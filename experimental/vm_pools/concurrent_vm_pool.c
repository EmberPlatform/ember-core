#include "concurrent_vm_pool.h"
#include "work_stealing_pool.h"
#include "../memory/memory_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <sys/inotify.h>
#include <sys/stat.h>
#include <openssl/sha.h>
#include <math.h>

// Default VM pool configuration
const concurrent_vm_pool_config_t DEFAULT_VM_POOL_CONFIG = {
    .initial_vm_count = 0,              // 0 = use CPU count
    .max_vm_count = 0,                  // 0 = use CPU count * 4
    .vm_pool_growth_factor = 2,
    .vm_idle_timeout_ns = 300000000000ULL,  // 5 minutes
    .enable_vm_reuse = true,
    .enable_bytecode_caching = true,
    .enable_hot_reload = false,
    .max_script_cache_size = 1000,
    .script_cache_ttl_ns = 3600000000000ULL  // 1 hour
};

// Cache hash table size (prime number for better distribution)
#define CACHE_TABLE_SIZE 1009

// Global pool instance for mod_ember integration
static concurrent_vm_pool_t* g_global_vm_pool = NULL;
static pthread_mutex_t g_global_pool_mutex = PTHREAD_MUTEX_INITIALIZER;

// Forward declarations
static void vm_task_executor(void* data);
static int create_vm_instance(concurrent_vm_pool_t* pool, vm_pool_entry_t* entry);
static void cleanup_vm_instance(vm_pool_entry_t* entry);
static void* hot_reload_monitor_thread(void* arg);
static void invalidate_script_cache(concurrent_vm_pool_t* pool, const char* script_path);
static uint32_t cache_hash_function(const char* key);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

uint32_t hash_string(const char* str) {
    if (!str) return 0;
    
    uint32_t hash = 5381;
    int c;
    while ((c = *str++)) {
        hash = ((hash << 5) + hash) + c;
    }
    return hash;
}

static uint32_t cache_hash_function(const char* key) {
    return hash_string(key) % CACHE_TABLE_SIZE;
}

const char* vm_state_to_string(vm_state_t state) {
    switch (state) {
        case VM_STATE_IDLE: return "IDLE";
        case VM_STATE_EXECUTING: return "EXECUTING";
        case VM_STATE_ERROR: return "ERROR";
        case VM_STATE_CLEANUP: return "CLEANUP";
        default: return "UNKNOWN";
    }
}

int calculate_optimal_vm_count(void) {
    int cpu_count = get_optimal_thread_count();
    // For I/O bound workloads like HTTP request processing,
    // we can have more VMs than CPU cores
    return cpu_count * 2;
}

__attribute__((unused)) static char* calculate_source_hash(const char* source_code) {
    if (!source_code) return NULL;
    
    unsigned char hash[SHA256_DIGEST_LENGTH];
    SHA256_CTX sha256;
    SHA256_Init(&sha256);
    SHA256_Update(&sha256, source_code, strlen(source_code));
    SHA256_Final(hash, &sha256);
    
    char* hex_hash = malloc(SHA256_DIGEST_LENGTH * 2 + 1);
    if (!hex_hash) return NULL;
    
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        snprintf(hex_hash + (i * 2), 3, "%02x", hash[i]);
    }
    hex_hash[SHA256_DIGEST_LENGTH * 2] = '\0';
    
    return hex_hash;
}

// ============================================================================
// VM POOL ENTRY MANAGEMENT
// ============================================================================

static int create_vm_instance(concurrent_vm_pool_t* pool, vm_pool_entry_t* entry) {
    if (!pool || !entry) {
        return -1;
    }
    
    // Initialize mutex
    if (pthread_mutex_init(&entry->vm_mutex, NULL) != 0) {
        return -1;
    }
    
    // Create new Ember VM
    entry->vm = ember_new_vm();
    if (!entry->vm) {
        pthread_mutex_destroy(&entry->vm_mutex);
        return -1;
    }
    
    // Enable lazy loading for better performance
    ember_enable_lazy_loading(entry->vm, 1);
    
    // Register standard library functions
    // This would need to be implemented based on Ember's API
    // ember_register_stdlib(entry->vm);
    
    // Initialize entry state
    entry->state = VM_STATE_IDLE;
    entry->last_used = get_timestamp_ns();
    entry->request_count = 0;
    entry->error_count = 0;
    entry->avg_execution_time = 0.0;
    entry->total_execution_time = 0;
    entry->numa_node = 0;  // Default to node 0
    entry->is_persistent = pool->config.enable_vm_reuse;
    entry->cache_hits = 0;
    entry->cache_misses = 0;
    entry->compilation_time_total = 0;
    entry->compilation_count = 0;
    
    return 0;
}

static void cleanup_vm_instance(vm_pool_entry_t* entry) {
    if (!entry) {
        return;
    }
    
    if (entry->vm) {
        ember_free_vm(entry->vm);
        entry->vm = NULL;
    }
    
    pthread_mutex_destroy(&entry->vm_mutex);
    memset(entry, 0, sizeof(vm_pool_entry_t));
}

// ============================================================================
// BYTECODE CACHE IMPLEMENTATION
// ============================================================================

int concurrent_vm_pool_cache_bytecode(concurrent_vm_pool_t* pool,
                                     const char* script_path,
                                     const char* source_hash,
                                     const uint8_t* bytecode,
                                     size_t bytecode_size) {
    if (!pool || !script_path || !bytecode || bytecode_size == 0) {
        return -1;
    }
    
    if (!pool->config.enable_bytecode_caching) {
        return 0;  // Caching disabled
    }
    
    pthread_rwlock_wrlock(&pool->cache_rwlock);
    
    // Check if we're at capacity
    if (pool->cached_scripts >= pool->config.max_script_cache_size) {
        // TODO: Implement LRU eviction
        pthread_rwlock_unlock(&pool->cache_rwlock);
        return -1;
    }
    
    uint32_t hash = cache_hash_function(script_path);
    
    // Check if entry already exists
    bytecode_cache_entry_t* existing = pool->bytecode_cache[hash];
    while (existing) {
        if (strcmp(existing->script_path, script_path) == 0) {
            // Update existing entry with atomic operations to prevent race conditions
            char* new_hash = source_hash ? strdup(source_hash) : NULL;
            uint8_t* new_bytecode = malloc(bytecode_size);
            if (new_bytecode) {
                memcpy(new_bytecode, bytecode, bytecode_size);
                
                // Save old pointers
                char* old_hash = existing->source_hash;
                uint8_t* old_bytecode = existing->bytecode;
                
                // Atomic update
                existing->source_hash = new_hash;
                existing->bytecode = new_bytecode;
                existing->bytecode_size = bytecode_size;
                existing->compile_time = time(NULL);
                existing->access_count = 0;
                existing->last_access = get_timestamp_ns();
                
                // Free old data after update
                free(old_hash);
                free(old_bytecode);
            } else {
                free(new_hash);
            }
            
            pthread_rwlock_unlock(&pool->cache_rwlock);
            return 0;
        }
        existing = existing->next;
    }
    
    // Create new entry
    bytecode_cache_entry_t* entry = malloc(sizeof(bytecode_cache_entry_t));
    if (!entry) {
        pthread_rwlock_unlock(&pool->cache_rwlock);
        return -1;
    }
    
    entry->script_path = strdup(script_path);
    entry->source_hash = source_hash ? strdup(source_hash) : NULL;
    entry->bytecode = malloc(bytecode_size);
    entry->bytecode_size = bytecode_size;
    entry->compile_time = time(NULL);
    entry->file_mtime = 0;  // TODO: Get actual file mtime
    entry->access_count = 0;
    entry->last_access = get_timestamp_ns();
    
    if (!entry->script_path || !entry->bytecode) {
        free(entry->script_path);
        free(entry->source_hash);
        free(entry->bytecode);
        free(entry);
        pthread_rwlock_unlock(&pool->cache_rwlock);
        return -1;
    }
    
    memcpy(entry->bytecode, bytecode, bytecode_size);
    
    // Add to hash table
    entry->next = pool->bytecode_cache[hash];
    pool->bytecode_cache[hash] = entry;
    pool->cached_scripts++;
    
    pthread_rwlock_unlock(&pool->cache_rwlock);
    return 0;
}

bytecode_cache_entry_t* concurrent_vm_pool_lookup_bytecode(concurrent_vm_pool_t* pool,
                                                          const char* script_path) {
    if (!pool || !script_path || !pool->config.enable_bytecode_caching) {
        return NULL;
    }
    
    pthread_rwlock_rdlock(&pool->cache_rwlock);
    
    uint32_t hash = cache_hash_function(script_path);
    bytecode_cache_entry_t* entry = pool->bytecode_cache[hash];
    
    while (entry) {
        if (strcmp(entry->script_path, script_path) == 0) {
            // Update access statistics
            entry->access_count++;
            entry->last_access = get_timestamp_ns();
            pool->script_cache_hits++;
            
            pthread_rwlock_unlock(&pool->cache_rwlock);
            return entry;
        }
        entry = entry->next;
    }
    
    pool->script_cache_misses++;
    pthread_rwlock_unlock(&pool->cache_rwlock);
    return NULL;
}

void concurrent_vm_pool_invalidate_cache(concurrent_vm_pool_t* pool,
                                        const char* script_path) {
    if (!pool || !script_path) {
        return;
    }
    
    pthread_rwlock_wrlock(&pool->cache_rwlock);
    
    uint32_t hash = cache_hash_function(script_path);
    bytecode_cache_entry_t** current = &pool->bytecode_cache[hash];
    
    while (*current) {
        if (strcmp((*current)->script_path, script_path) == 0) {
            bytecode_cache_entry_t* to_remove = *current;
            *current = (*current)->next;
            
            free(to_remove->script_path);
            free(to_remove->source_hash);
            free(to_remove->bytecode);
            free(to_remove);
            
            pool->cached_scripts--;
            break;
        } else {
            current = &(*current)->next;
        }
    }
    
    pthread_rwlock_unlock(&pool->cache_rwlock);
}

void concurrent_vm_pool_clear_cache(concurrent_vm_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pthread_rwlock_wrlock(&pool->cache_rwlock);
    
    for (size_t i = 0; i < CACHE_TABLE_SIZE; i++) {
        bytecode_cache_entry_t* entry = pool->bytecode_cache[i];
        while (entry) {
            bytecode_cache_entry_t* next = entry->next;
            
            free(entry->script_path);
            free(entry->source_hash);
            free(entry->bytecode);
            free(entry);
            
            entry = next;
        }
        pool->bytecode_cache[i] = NULL;
    }
    
    pool->cached_scripts = 0;
    pthread_rwlock_unlock(&pool->cache_rwlock);
}

// ============================================================================
// TASK CONTEXT MANAGEMENT
// ============================================================================

vm_task_context_t* vm_task_context_create(const char* script_path,
                                         const char* source_code,
                                         void* request) {
    vm_task_context_t* context = malloc(sizeof(vm_task_context_t));
    if (!context) {
        return NULL;
    }
    
    memset(context, 0, sizeof(vm_task_context_t));
    
    context->script_path = script_path ? strdup(script_path) : NULL;
    if (source_code) {
        context->source_length = strlen(source_code);
        context->source_code = malloc(context->source_length + 1);
        if (context->source_code) {
            strcpy(context->source_code, source_code);
        }
    }
    
    context->request = request;  // Note: We don't copy, just reference
    context->task_id = get_timestamp_ns();  // Use timestamp as unique ID
    context->execution_result = -1;  // Initialize as not executed
    
    return context;
}

void vm_task_context_destroy(vm_task_context_t* context) {
    if (!context) {
        return;
    }
    
    free(context->script_path);
    free(context->source_code);
    free(context->error_message);
    free(context);
}

// ============================================================================
// TASK EXECUTION
// ============================================================================

static void vm_task_executor(void* data) {
    vm_task_context_t* context = (vm_task_context_t*)data;
    if (!context || !context->vm) {
        return;
    }
    
    uint64_t start_time = get_timestamp_ns();
    context->execution_result = 0;
    
    // Compile and execute script
    if (context->source_code) {
        int result = ember_eval(context->vm, context->source_code);
        if (result != 0) {
            context->execution_result = result;
            
            ember_error* error = ember_vm_get_error(context->vm);
            if (error && error->message[0] != '\0') {
                context->error_message = strdup(error->message);
            }
        }
    }
    
    context->execution_time_ns = get_timestamp_ns() - start_time;
    
    // Call completion callback if provided
    if (context->completion_callback) {
        context->completion_callback(context, context->callback_user_data);
    }
}

// ============================================================================
// VM POOL IMPLEMENTATION
// ============================================================================

concurrent_vm_pool_t* concurrent_vm_pool_create(const concurrent_vm_pool_config_t* config,
                                               const thread_pool_config_t* thread_config) {
    concurrent_vm_pool_t* pool = malloc(sizeof(concurrent_vm_pool_t));
    if (!pool) {
        return NULL;
    }
    
    memset(pool, 0, sizeof(concurrent_vm_pool_t));
    
    // Copy configuration or use defaults
    if (config) {
        pool->config = *config;
    } else {
        pool->config = DEFAULT_VM_POOL_CONFIG;
    }
    
    // Set default values if not specified
    if (pool->config.initial_vm_count == 0) {
        pool->config.initial_vm_count = calculate_optimal_vm_count();
    }
    if (pool->config.max_vm_count == 0) {
        pool->config.max_vm_count = calculate_optimal_vm_count() * 4;
    }
    
    pool->creation_time = get_timestamp_ns();
    pool->is_shutting_down = false;
    
    // Initialize synchronization objects
    if (pthread_mutex_init(&pool->vm_pool_mutex, NULL) != 0 ||
        pthread_cond_init(&pool->pool_condition, NULL) != 0 ||
        pthread_rwlock_init(&pool->cache_rwlock, NULL) != 0) {
        free(pool);
        return NULL;
    }
    
    // Create work stealing thread pool
    pool->thread_pool = work_stealing_pool_create(thread_config);
    if (!pool->thread_pool) {
        concurrent_vm_pool_destroy(pool);
        return NULL;
    }
    
    // Initialize VM pool
    pool->vm_pool_capacity = pool->config.max_vm_count;
    pool->vm_entries = calloc(pool->vm_pool_capacity, sizeof(vm_pool_entry_t));
    if (!pool->vm_entries) {
        concurrent_vm_pool_destroy(pool);
        return NULL;
    }
    
    // Initialize bytecode cache
    if (pool->config.enable_bytecode_caching) {
        pool->cache_table_size = CACHE_TABLE_SIZE;
        pool->bytecode_cache = calloc(CACHE_TABLE_SIZE, sizeof(bytecode_cache_entry_t*));
        if (!pool->bytecode_cache) {
            concurrent_vm_pool_destroy(pool);
            return NULL;
        }
    }
    
    return pool;
}

void concurrent_vm_pool_destroy(concurrent_vm_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Shutdown the pool
    if (!pool->is_shutting_down) {
        concurrent_vm_pool_shutdown(pool, true);
    }
    
    // Disable hot reload
    concurrent_vm_pool_disable_hot_reload(pool);
    
    // Cleanup VM pool
    if (pool->vm_entries) {
        for (size_t i = 0; i < pool->vm_pool_size; i++) {
            cleanup_vm_instance(&pool->vm_entries[i]);
        }
        free(pool->vm_entries);
    }
    
    // Cleanup bytecode cache
    concurrent_vm_pool_clear_cache(pool);
    free(pool->bytecode_cache);
    
    // Cleanup thread pool
    if (pool->thread_pool) {
        work_stealing_pool_destroy(pool->thread_pool);
    }
    
    // Cleanup synchronization objects
    pthread_mutex_destroy(&pool->vm_pool_mutex);
    pthread_cond_destroy(&pool->pool_condition);
    pthread_rwlock_destroy(&pool->cache_rwlock);
    
    free(pool);
}

int concurrent_vm_pool_start(concurrent_vm_pool_t* pool) {
    if (!pool || pool->is_shutting_down) {
        return -1;
    }
    
    // Start the work stealing thread pool
    if (work_stealing_pool_start(pool->thread_pool) != 0) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->vm_pool_mutex);
    
    // Create initial VM instances
    for (size_t i = 0; i < pool->config.initial_vm_count; i++) {
        if (create_vm_instance(pool, &pool->vm_entries[i]) == 0) {
            pool->vm_pool_size++;
            pool->available_vms++;
        }
    }
    
    pthread_mutex_unlock(&pool->vm_pool_mutex);
    
    return pool->vm_pool_size > 0 ? 0 : -1;
}

int concurrent_vm_pool_shutdown(concurrent_vm_pool_t* pool, bool wait_for_completion) {
    if (!pool) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->vm_pool_mutex);
    pool->is_shutting_down = true;
    pthread_cond_broadcast(&pool->pool_condition);
    pthread_mutex_unlock(&pool->vm_pool_mutex);
    
    // Shutdown the thread pool
    if (pool->thread_pool) {
        work_stealing_pool_shutdown(pool->thread_pool, wait_for_completion);
    }
    
    return 0;
}

ember_vm* concurrent_vm_pool_acquire_vm(concurrent_vm_pool_t* pool) {
    if (!pool || pool->is_shutting_down) {
        return NULL;
    }
    
    pthread_mutex_lock(&pool->vm_pool_mutex);
    
    // Find available VM
    for (size_t i = 0; i < pool->vm_pool_size; i++) {
        vm_pool_entry_t* entry = &pool->vm_entries[i];
        
        if (pthread_mutex_trylock(&entry->vm_mutex) == 0) {
            if (entry->state == VM_STATE_IDLE && entry->vm) {
                entry->state = VM_STATE_EXECUTING;
                entry->last_used = get_timestamp_ns();
                pool->available_vms--;
                pool->vm_acquisitions++;
                
                pthread_mutex_unlock(&pool->vm_pool_mutex);
                return entry->vm;
            }
            pthread_mutex_unlock(&entry->vm_mutex);
        }
    }
    
    // No available VMs, try to expand pool if possible
    if (pool->vm_pool_size < pool->vm_pool_capacity) {
        vm_pool_entry_t* entry = &pool->vm_entries[pool->vm_pool_size];
        if (create_vm_instance(pool, entry) == 0) {
            pthread_mutex_lock(&entry->vm_mutex);
            entry->state = VM_STATE_EXECUTING;
            entry->last_used = get_timestamp_ns();
            
            pool->vm_pool_size++;
            pool->vm_acquisitions++;
            pool->vm_pool_expansions++;
            
            pthread_mutex_unlock(&pool->vm_pool_mutex);
            return entry->vm;
        }
    }
    
    pool->vm_acquisition_failures++;
    pthread_mutex_unlock(&pool->vm_pool_mutex);
    return NULL;
}

void concurrent_vm_pool_release_vm(concurrent_vm_pool_t* pool, ember_vm* vm) {
    if (!pool || !vm) {
        return;
    }
    
    pthread_mutex_lock(&pool->vm_pool_mutex);
    
    // Find the VM entry
    for (size_t i = 0; i < pool->vm_pool_size; i++) {
        vm_pool_entry_t* entry = &pool->vm_entries[i];
        
        if (entry->vm == vm) {
            entry->state = VM_STATE_IDLE;
            entry->request_count++;
            
            // Clear VM state for next request
            ember_vm_clear_error(vm);
            
            pool->available_vms++;
            pthread_mutex_unlock(&entry->vm_mutex);
            break;
        }
    }
    
    pthread_mutex_unlock(&pool->vm_pool_mutex);
}

int concurrent_vm_pool_submit_request(concurrent_vm_pool_t* pool,
                                     void* request,
                                     void (*completion_callback)(vm_task_context_t*, void*),
                                     void* user_data) {
    (void)completion_callback; // Unused parameter
    (void)user_data; // Unused parameter
    if (!pool || !request) {
        return -1;
    }
    
    // TODO: This function needs to be refactored to not depend on request structure
    // For now, return error as this is web-integration code that shouldn't be in ember-core
    return -1;
    
    /*
    FILE* file = fopen(request->script_path, "r");
    if (!file) {
        return -1;
    }
    
    fseek(file, 0, SEEK_END);
    source_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    source_code = malloc(source_size + 1);
    if (!source_code) {
        fclose(file);
        return -1;
    }
    
    fread(source_code, 1, source_size, file);
    source_code[source_size] = '\0';
    fclose(file);
    
    int result = concurrent_vm_pool_submit_script_execution(pool, "script_path",
                                                           source_code, request,
                                                           completion_callback, user_data);
    
    free(source_code);
    return result;
    */
}

int concurrent_vm_pool_submit_script_execution(concurrent_vm_pool_t* pool,
                                              const char* script_path,
                                              const char* source_code,
                                              void* request,
                                              void (*completion_callback)(vm_task_context_t*, void*),
                                              void* user_data) {
    if (!pool || !source_code) {
        return -1;
    }
    
    // Create task context
    vm_task_context_t* context = vm_task_context_create(script_path, source_code, request);
    if (!context) {
        return -1;
    }
    
    context->completion_callback = completion_callback;
    context->callback_user_data = user_data;
    
    // Acquire VM
    context->vm = concurrent_vm_pool_acquire_vm(pool);
    if (!context->vm) {
        vm_task_context_destroy(context);
        return -1;
    }
    
    // Create work task
    work_task_t* task = work_stealing_pool_create_task(vm_task_executor, context,
                                                      TASK_PRIORITY_NORMAL,
                                                      TASK_TYPE_VM_EXECUTION);
    if (!task) {
        concurrent_vm_pool_release_vm(pool, context->vm);
        vm_task_context_destroy(context);
        return -1;
    }
    
    // Submit task to thread pool
    if (work_stealing_pool_submit_task(pool->thread_pool, task) != 0) {
        concurrent_vm_pool_release_vm(pool, context->vm);
        vm_task_context_destroy(context);
        free(task);
        return -1;
    }
    
    return 0;
}

// ============================================================================
// HOT RELOAD IMPLEMENTATION
// ============================================================================

static void* hot_reload_monitor_thread(void* arg) {
    concurrent_vm_pool_t* pool = (concurrent_vm_pool_t*)arg;
    if (!pool) {
        return NULL;
    }
    
    char buffer[4096];
    struct inotify_event* event;
    
    while (!pool->is_shutting_down) {
        ssize_t length = read(pool->inotify_fd, buffer, sizeof(buffer));
        if (length < 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        
        for (char* ptr = buffer; ptr < buffer + length; ) {
            event = (struct inotify_event*)ptr;
            
            if (event->mask & (IN_MODIFY | IN_MOVED_TO | IN_CREATE)) {
                if (event->len > 0) {
                    char* filename = event->name;
                    if (strstr(filename, ".ember")) {
                        // Invalidate cache for this script
                        invalidate_script_cache(pool, filename);
                    }
                }
            }
            
            ptr += sizeof(struct inotify_event) + event->len;
        }
    }
    
    return NULL;
}

static void invalidate_script_cache(concurrent_vm_pool_t* pool, const char* script_path) {
    if (!pool || !script_path) {
        return;
    }
    
    concurrent_vm_pool_invalidate_cache(pool, script_path);
    
    // Could also log the invalidation
    fprintf(stderr, "[Hot Reload] Invalidated cache for: %s\n", script_path);
}

int concurrent_vm_pool_enable_hot_reload(concurrent_vm_pool_t* pool,
                                        const char* watch_directory) {
    if (!pool || !watch_directory || pool->hot_reload_enabled) {
        return -1;
    }
    
    pool->inotify_fd = inotify_init1(IN_NONBLOCK);
    if (pool->inotify_fd == -1) {
        return -1;
    }
    
    pool->inotify_wd = inotify_add_watch(pool->inotify_fd, watch_directory,
                                        IN_MODIFY | IN_MOVED_TO | IN_CREATE);
    if (pool->inotify_wd == -1) {
        close(pool->inotify_fd);
        return -1;
    }
    
    if (pthread_create(&pool->hot_reload_thread, NULL, hot_reload_monitor_thread, pool) != 0) {
        inotify_rm_watch(pool->inotify_fd, pool->inotify_wd);
        close(pool->inotify_fd);
        return -1;
    }
    
    pool->hot_reload_enabled = true;
    return 0;
}

void concurrent_vm_pool_disable_hot_reload(concurrent_vm_pool_t* pool) {
    if (!pool || !pool->hot_reload_enabled) {
        return;
    }
    
    pool->hot_reload_enabled = false;
    
    if (pool->inotify_wd != -1) {
        inotify_rm_watch(pool->inotify_fd, pool->inotify_wd);
        pool->inotify_wd = -1;
    }
    
    if (pool->inotify_fd != -1) {
        close(pool->inotify_fd);
        pool->inotify_fd = -1;
    }
    
    // Wait for hot reload thread to finish
    pthread_join(pool->hot_reload_thread, NULL);
}

// ============================================================================
// STATISTICS AND MONITORING
// ============================================================================

void concurrent_vm_pool_get_stats(concurrent_vm_pool_t* pool,
                                 pool_statistics_t* thread_stats,
                                 struct vm_pool_stats* vm_stats) {
    if (!pool) {
        return;
    }
    
    // Get thread pool statistics
    if (thread_stats) {
        work_stealing_pool_get_stats(pool->thread_pool, thread_stats);
    }
    
    // Get VM pool statistics
    if (vm_stats) {
        pthread_mutex_lock(&pool->vm_pool_mutex);
        
        vm_stats->vm_pool_size = pool->vm_pool_size;
        vm_stats->available_vms = pool->available_vms;
        vm_stats->active_vms = pool->vm_pool_size - pool->available_vms;
        vm_stats->total_requests = pool->total_requests;
        vm_stats->total_execution_time = pool->total_execution_time;
        vm_stats->total_compilation_time = pool->total_compilation_time;
        vm_stats->script_cache_hits = pool->script_cache_hits;
        vm_stats->script_cache_misses = pool->script_cache_misses;
        vm_stats->cached_scripts = pool->cached_scripts;
        vm_stats->vm_acquisitions = pool->vm_acquisitions;
        vm_stats->vm_acquisition_failures = pool->vm_acquisition_failures;
        
        // Calculate derived statistics
        if (pool->total_requests > 0) {
            vm_stats->avg_request_time = (double)pool->total_execution_time / 
                                        pool->total_requests / 1000000.0;  // Convert to ms
        } else {
            vm_stats->avg_request_time = 0.0;
        }
        
        uint64_t total_compilation_ops = pool->script_cache_hits + pool->script_cache_misses;
        if (total_compilation_ops > 0) {
            vm_stats->avg_compilation_time = (double)pool->total_compilation_time /
                                           total_compilation_ops / 1000000.0;  // Convert to ms
            vm_stats->cache_hit_rate = (double)pool->script_cache_hits / 
                                      total_compilation_ops * 100.0;
        } else {
            vm_stats->avg_compilation_time = 0.0;
            vm_stats->cache_hit_rate = 0.0;
        }
        
        vm_stats->vm_utilization = pool->vm_pool_size > 0 ? 
                                  (double)(pool->vm_pool_size - pool->available_vms) /
                                  pool->vm_pool_size * 100.0 : 0.0;
        
        pthread_mutex_unlock(&pool->vm_pool_mutex);
    }
}

void concurrent_vm_pool_print_stats(concurrent_vm_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pool_statistics_t thread_stats;
    struct vm_pool_stats vm_stats;
    
    concurrent_vm_pool_get_stats(pool, &thread_stats, &vm_stats);
    
    printf("\n=== Concurrent VM Pool Statistics ===\n");
    
    printf("VM Pool Status:\n");
    printf("  VM pool size: %zu\n", vm_stats.vm_pool_size);
    printf("  Available VMs: %zu\n", vm_stats.available_vms);
    printf("  Active VMs: %zu\n", vm_stats.active_vms);
    printf("  VM utilization: %.2f%%\n", vm_stats.vm_utilization);
    
    printf("\nRequest Processing:\n");
    printf("  Total requests: %lu\n", vm_stats.total_requests);
    printf("  VM acquisitions: %lu\n", vm_stats.vm_acquisitions);
    printf("  VM acquisition failures: %lu\n", vm_stats.vm_acquisition_failures);
    printf("  Average request time: %.3f ms\n", vm_stats.avg_request_time);
    
    printf("\nBytecode Caching:\n");
    printf("  Cached scripts: %zu\n", vm_stats.cached_scripts);
    printf("  Cache hits: %lu\n", vm_stats.script_cache_hits);
    printf("  Cache misses: %lu\n", vm_stats.script_cache_misses);
    printf("  Cache hit rate: %.2f%%\n", vm_stats.cache_hit_rate);
    printf("  Average compilation time: %.3f ms\n", vm_stats.avg_compilation_time);
    
    printf("\nHot Reload:\n");
    printf("  Hot reload enabled: %s\n", pool->hot_reload_enabled ? "Yes" : "No");
    
    printf("\n");
    
    // Print thread pool statistics
    work_stealing_pool_print_stats(pool->thread_pool);
}

size_t concurrent_vm_pool_get_pending_requests(concurrent_vm_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    
    return work_stealing_pool_get_pending_tasks(pool->thread_pool);
}

double concurrent_vm_pool_get_vm_utilization(concurrent_vm_pool_t* pool) {
    if (!pool) {
        return 0.0;
    }
    
    pthread_mutex_lock(&pool->vm_pool_mutex);
    double utilization = pool->vm_pool_size > 0 ? 
                        (double)(pool->vm_pool_size - pool->available_vms) /
                        pool->vm_pool_size * 100.0 : 0.0;
    pthread_mutex_unlock(&pool->vm_pool_mutex);
    
    return utilization;
}

// ============================================================================
// GLOBAL INTEGRATION FOR MOD_EMBER
// ============================================================================

int mod_ember_init_concurrent_pool(const char* document_root,
                                  const concurrent_vm_pool_config_t* vm_config,
                                  const thread_pool_config_t* thread_config) {
    (void)document_root; // Unused parameter
    pthread_mutex_lock(&g_global_pool_mutex);
    
    if (g_global_vm_pool) {
        pthread_mutex_unlock(&g_global_pool_mutex);
        return 0;  // Already initialized
    }
    
    g_global_vm_pool = concurrent_vm_pool_create(vm_config, thread_config);
    if (!g_global_vm_pool) {
        pthread_mutex_unlock(&g_global_pool_mutex);
        return -1;
    }
    
    if (concurrent_vm_pool_start(g_global_vm_pool) != 0) {
        concurrent_vm_pool_destroy(g_global_vm_pool);
        g_global_vm_pool = NULL;
        pthread_mutex_unlock(&g_global_pool_mutex);
        return -1;
    }
    
    pthread_mutex_unlock(&g_global_pool_mutex);
    return 0;
}

void mod_ember_cleanup_concurrent_pool(void) {
    pthread_mutex_lock(&g_global_pool_mutex);
    
    if (g_global_vm_pool) {
        concurrent_vm_pool_destroy(g_global_vm_pool);
        g_global_vm_pool = NULL;
    }
    
    pthread_mutex_unlock(&g_global_pool_mutex);
}

int mod_ember_submit_concurrent_request(const char* script_path,
                                       void* request,
                                       void (*completion_callback)(vm_task_context_t*, void*),
                                       void* user_data) {
    (void)script_path; // Unused parameter
    pthread_mutex_lock(&g_global_pool_mutex);
    
    if (!g_global_vm_pool) {
        pthread_mutex_unlock(&g_global_pool_mutex);
        return -1;
    }
    
    concurrent_vm_pool_t* pool = g_global_vm_pool;
    pthread_mutex_unlock(&g_global_pool_mutex);
    
    return concurrent_vm_pool_submit_request(pool, request, completion_callback, user_data);
}