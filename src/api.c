#define _GNU_SOURCE
#include "ember.h"
#include "vm.h"
#include "runtime/value/value.h"
#include "frontend/parser/parser.h"
#include "frontend/lexer/lexer.h"
#include "runtime/runtime.h"
#include "runtime/package/package.h"
#include <stddef.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <limits.h>
#include <errno.h>
#include <stdint.h>
#include <time.h>
#include <stdbool.h>

// Clean public API - only embedding interface functions remain

// ============================================================================
// SECURE VM POOL API IMPLEMENTATION
// ============================================================================

// Global state for secure VM pool (simplified for security testing)
static ember_vm** secure_vm_pool = NULL;
static size_t secure_pool_size = 0;
static size_t secure_pool_capacity = 0;
static vm_pool_config_t secure_pool_config = {0};
static bool secure_pool_initialized = false;
static uint64_t allocation_count = 0;
static uint64_t last_rate_limit_reset = 0;

// Security validation constants
#define MIN_POOL_SIZE 1
#define MAX_POOL_SIZE 1000
#define MIN_CHUNK_SIZE 1
#define MAX_CHUNK_SIZE 100
#define MIN_THREAD_CACHE 1
#define MAX_THREAD_CACHE 100
#define MIN_VMS_PER_THREAD 1
#define MAX_VMS_PER_THREAD 100
#define MIN_RATE_LIMIT_WINDOW 100
#define MAX_RATE_LIMIT_WINDOW 86400000
#define MIN_RATE_LIMIT_ALLOCS 1
#define MAX_RATE_LIMIT_ALLOCS 10000

// Default secure configuration
static const vm_pool_config_t DEFAULT_SECURE_CONFIG = {
    .initial_size = 16,
    .chunk_size = 8,
    .thread_cache_size = 4,
    .max_vms_per_thread = 10,
    .rate_limit_window_ms = 1000,
    .rate_limit_max_allocs = 50
};

// Get current timestamp in milliseconds
static uint64_t get_current_time_ms(void) {
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;  // Return 0 on error (security: don't leak system details)
    }
    return (uint64_t)ts.tv_sec * 1000 + ts.tv_nsec / 1000000;
}

// Secure configuration validation
static int validate_pool_config(const vm_pool_config_t* config) {
    if (!config) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    // Check all parameters are within safe bounds
    if (config->initial_size < MIN_POOL_SIZE || config->initial_size > MAX_POOL_SIZE) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->chunk_size < MIN_CHUNK_SIZE || config->chunk_size > MAX_CHUNK_SIZE) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->thread_cache_size < MIN_THREAD_CACHE || config->thread_cache_size > MAX_THREAD_CACHE) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->max_vms_per_thread < MIN_VMS_PER_THREAD || config->max_vms_per_thread > MAX_VMS_PER_THREAD) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->rate_limit_window_ms < MIN_RATE_LIMIT_WINDOW || config->rate_limit_window_ms > MAX_RATE_LIMIT_WINDOW) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    if (config->rate_limit_max_allocs < MIN_RATE_LIMIT_ALLOCS || config->rate_limit_max_allocs > MAX_RATE_LIMIT_ALLOCS) {
        return EMBER_ERROR_INVALID_PARAMETER;
    }
    
    // Check for integer overflow potential
    if (config->initial_size > UINT32_MAX / sizeof(ember_vm*)) {
        return EMBER_ERROR_SECURITY_VIOLATION;
    }
    
    return EMBER_SUCCESS;
}

// Apply secure defaults only for zero values (preserving invalid values for validation)
static void apply_secure_defaults(vm_pool_config_t* config) {
    if (!config) return;
    
    // Apply defaults only for zero values - let validation catch invalid ones
    if (config->initial_size == 0) {
        config->initial_size = DEFAULT_SECURE_CONFIG.initial_size;
    }
    
    if (config->chunk_size == 0) {
        config->chunk_size = DEFAULT_SECURE_CONFIG.chunk_size;
    }
    
    if (config->thread_cache_size == 0) {
        config->thread_cache_size = DEFAULT_SECURE_CONFIG.thread_cache_size;
    }
    
    if (config->max_vms_per_thread == 0) {
        config->max_vms_per_thread = DEFAULT_SECURE_CONFIG.max_vms_per_thread;
    }
    
    if (config->rate_limit_window_ms == 0) {
        config->rate_limit_window_ms = DEFAULT_SECURE_CONFIG.rate_limit_window_ms;
    }
    
    if (config->rate_limit_max_allocs == 0) {
        config->rate_limit_max_allocs = DEFAULT_SECURE_CONFIG.rate_limit_max_allocs;
    }
}

// Rate limiting check
static int check_rate_limit(void) {
    uint64_t current_time = get_current_time_ms();
    
    // Reset allocation count if window has passed
    if (current_time - last_rate_limit_reset >= secure_pool_config.rate_limit_window_ms) {
        allocation_count = 0;
        last_rate_limit_reset = current_time;
    }
    
    // Check if rate limit exceeded
    if (allocation_count >= secure_pool_config.rate_limit_max_allocs) {
        return EMBER_ERROR_RESOURCE_EXHAUSTED;
    }
    
    allocation_count++;
    return EMBER_SUCCESS;
}

// Secure VM pool initialization
int ember_pool_init(const vm_pool_config_t* config) {
    // NULL config is allowed - use secure defaults
    vm_pool_config_t effective_config;
    
    if (config == NULL) {
        // Use default secure configuration
        effective_config = DEFAULT_SECURE_CONFIG;
    } else {
        // Copy and validate provided configuration
        effective_config = *config;
        apply_secure_defaults(&effective_config);
        
        int validation_result = validate_pool_config(&effective_config);
        if (validation_result != EMBER_SUCCESS) {
            return validation_result;
        }
    }
    
    // Clean up existing pool if already initialized
    if (secure_pool_initialized) {
        ember_pool_cleanup();
    }
    
    // Allocate pool array with size validation
    size_t pool_size = effective_config.initial_size;
    
    // Check for potential integer overflow
    if (pool_size > SIZE_MAX / sizeof(ember_vm*)) {
        return EMBER_ERROR_SECURITY_VIOLATION;
    }
    
    secure_vm_pool = malloc(pool_size * sizeof(ember_vm*));
    if (!secure_vm_pool) {
        return EMBER_ERROR_MEMORY_ALLOCATION;
    }
    
    // Initialize all VMs to NULL for safe cleanup
    for (size_t i = 0; i < pool_size; i++) {
        secure_vm_pool[i] = NULL;
    }
    
    // Set pool state
    secure_pool_capacity = pool_size;
    secure_pool_size = 0;
    secure_pool_config = effective_config;
    secure_pool_initialized = true;
    allocation_count = 0;
    last_rate_limit_reset = get_current_time_ms();
    
    return EMBER_SUCCESS;
}

// Secure VM pool cleanup
void ember_pool_cleanup(void) {
    if (!secure_pool_initialized || !secure_vm_pool) {
        return;  // Safe to call multiple times
    }
    
    // Clean up all VMs in the pool
    for (size_t i = 0; i < secure_pool_capacity; i++) {
        if (secure_vm_pool[i] != NULL) {
            ember_free_vm(secure_vm_pool[i]);
            secure_vm_pool[i] = NULL;
        }
    }
    
    // Free the pool array
    free(secure_vm_pool);
    secure_vm_pool = NULL;
    secure_pool_capacity = 0;
    secure_pool_size = 0;
    secure_pool_initialized = false;
    allocation_count = 0;
    last_rate_limit_reset = 0;
}

// Secure VM acquisition with rate limiting
ember_vm* ember_pool_get_vm(void) {
    // Check if pool is initialized
    if (!secure_pool_initialized) {
        return NULL;  // Graceful failure - no error leakage
    }
    
    // Apply rate limiting
    int rate_check = check_rate_limit();
    if (rate_check != EMBER_SUCCESS) {
        return NULL;  // Rate limited - no error details leaked
    }
    
    // Try to get VM from pool first
    for (size_t i = 0; i < secure_pool_capacity; i++) {
        if (secure_vm_pool[i] != NULL) {
            ember_vm* vm = secure_vm_pool[i];
            secure_vm_pool[i] = NULL;
            secure_pool_size--;
            
            // Reset VM state for security
            if (vm) {
                vm->ip = 0;
                vm->stack_top = 0;
                vm->exception_pending = 0;
                // Clear current exception for security
                vm->current_exception = ember_make_nil();
            }
            
            return vm;
        }
    }
    
    // No available VM in pool - create new one if under limits
    if (secure_pool_size < secure_pool_config.max_vms_per_thread) {
        ember_vm* new_vm = ember_new_vm();
        if (new_vm) {
            secure_pool_size++;
        }
        return new_vm;
    }
    
    return NULL;  // Pool exhausted
}

// Secure VM release with validation
void ember_pool_release_vm(ember_vm* vm) {
    // NULL VM is safe to release - no error
    if (!vm) {
        return;
    }
    
    // Check if pool is initialized
    if (!secure_pool_initialized || !secure_vm_pool) {
        // Pool not initialized - just free the VM
        ember_free_vm(vm);
        return;
    }
    
    // Reset VM state for security before returning to pool
    vm->ip = 0;
    vm->stack_top = 0;
    vm->exception_pending = 0;
    vm->current_exception = ember_make_nil();
    
    // Try to return VM to pool
    for (size_t i = 0; i < secure_pool_capacity; i++) {
        if (secure_vm_pool[i] == NULL) {
            secure_vm_pool[i] = vm;
            return;
        }
    }
    
    // Pool is full - free the VM
    ember_free_vm(vm);
    if (secure_pool_size > 0) {
        secure_pool_size--;
    }
}

// Internal helper function for VM-aware module path resolution
static char* ember_resolve_module_path_vm(ember_vm* vm, const char* module_name) {
    if (!module_name || strlen(module_name) == 0) {
        return NULL;
    }
    
    // Validate module name for security
    if (ember_package_validate_name(module_name) != 0) {
        fprintf(stderr, "[RESOLVE] Invalid module name: %s\n", module_name);
        return NULL;
    }
    
    char* resolved_path = malloc(EMBER_MAX_PATH_LEN);
    if (!resolved_path) {
        fprintf(stderr, "[RESOLVE] Failed to allocate memory for path resolution\n");
        return NULL;
    }
    
    // Search order:
    // 1. VM-specific module paths (if VM provided)
    // 2. Current directory (./<module_name>.ember)
    // 3. Current directory module subdirectory (./<module_name>/package.ember)
    // 4. User packages directory (~/.ember/packages/<module_name>/package.ember)
    // 5. System packages directory
    // 6. Standard library directory
    
    // 1. VM-specific module paths
    if (vm && vm->module_path_count > 0) {
        for (int i = 0; i < vm->module_path_count; i++) {
            if (vm->module_paths[i]) {
                // Try direct .ember file
                snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/%s.ember", vm->module_paths[i], module_name);
                if (access(resolved_path, R_OK) == 0) {
                    printf("[RESOLVE] Found module in custom path: %s\n", resolved_path);
                    return resolved_path;
                }
                
                // Try package.ember in subdirectory
                snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/%s/package.ember", vm->module_paths[i], module_name);
                if (access(resolved_path, R_OK) == 0) {
                    printf("[RESOLVE] Found module package in custom path: %s\n", resolved_path);
                    return resolved_path;
                }
            }
        }
    }
    
    // Fall back to standard resolution
    free(resolved_path);
    return ember_resolve_module_path(module_name);
}

// Module system implementation
int ember_import_module(ember_vm* vm, const char* module_name) {
    if (!vm || !module_name) {
        return -1;
    }
    
    // Check for circular dependency by looking for modules currently being loaded
    // We use the is_loaded flag - 0 means not started, 1 means loaded, -1 means currently loading
    for (int i = 0; i < vm->module_count; i++) {
        if (strcmp(vm->modules[i].name, module_name) == 0) {
            if (vm->modules[i].is_loaded == -1) {
                fprintf(stderr, "[MODULE] Circular dependency detected: %s\n", module_name);
                return -1;
            } else if (vm->modules[i].is_loaded == 1) {
                return 0; // Already loaded successfully
            }
            break; // Found but not loaded, continue to load
        }
    }
    
    // Note: Circular dependency check is performed above
    
    // Resolve module path using VM-specific search paths
    char* module_path = ember_resolve_module_path_vm(vm, module_name);
    if (!module_path) {
        fprintf(stderr, "[MODULE] Failed to resolve path for module: %s\n", module_name);
        return -1;
    }
    
    // Check if we have space for a new module
    if (vm->module_count >= EMBER_MAX_MODULES) {
        fprintf(stderr, "[MODULE] Maximum module limit reached (%d)\n", EMBER_MAX_MODULES);
        free(module_path);
        return -1;
    }
    
    // Find or create module entry
    ember_module* module = NULL;
    int module_index = -1;
    
    // Look for existing module entry
    for (int i = 0; i < vm->module_count; i++) {
        if (strcmp(vm->modules[i].name, module_name) == 0) {
            module = &vm->modules[i];
            module_index = i;
            break;
        }
    }
    
    // Create new module entry if not found
    if (!module) {
        module_index = vm->module_count++;
        module = &vm->modules[module_index];
        
        // Initialize module structure
        module->name = malloc(strlen(module_name) + 1);
        if (!module->name) {
            vm->module_count--;
            free(module_path);
            return -1;
        }
        strcpy(module->name, module_name);
        
        module->path = module_path;
        module->chunk = NULL;
        module->is_loaded = 0;
        module->export_count = 0;
    } else {
        // Update path if different
        if (module->path && strcmp(module->path, module_path) != 0) {
            free(module->path);
            module->path = module_path;
        } else if (!module->path) {
            module->path = module_path;
        } else {
            free(module_path); // Path is the same, free the duplicate
        }
    }
    
    // Mark module as currently loading to detect circular dependencies
    module->is_loaded = -1;
    
    // Check if file exists and is readable
    if (access(module->path, R_OK) != 0) {
        module->is_loaded = 0; // Reset loading state on error
        fprintf(stderr, "[MODULE] Module file not found or not readable: %s\n", module->path);
        return -1;
    }
    
    // Load and parse the module file
    FILE* file = fopen(module->path, "r");
    if (!file) {
        module->is_loaded = 0; // Reset loading state on error
        fprintf(stderr, "[MODULE] Failed to open module file: %s\n", module->path);
        return -1;
    }
    
    // Read file contents
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    char* source = malloc(file_size + 1);
    if (!source) {
        fclose(file);
        module->is_loaded = 0; // Reset loading state on error
        fprintf(stderr, "[MODULE] Failed to allocate memory for module source\n");
        return -1;
    }
    
    size_t bytes_read = fread(source, 1, file_size, file);
    source[bytes_read] = '\0';
    fclose(file);
    
    // Save current VM state
    ember_chunk* saved_chunk = vm->chunk;
    int saved_local_count = vm->local_count;
    
    // Create chunk for module
    module->chunk = malloc(sizeof(ember_chunk));
    if (!module->chunk) {
        free(source);
        module->is_loaded = 0; // Reset loading state on error
        fprintf(stderr, "[MODULE] Failed to allocate memory for module chunk\n");
        return -1;
    }
    
    init_chunk(module->chunk);
    track_function_chunk(vm, module->chunk);
    
    // Compile and execute module in isolated context
    vm->chunk = module->chunk;
    vm->local_count = 0;
    
    int result = ember_eval(vm, source);
    
    // Restore VM state
    vm->chunk = saved_chunk;
    vm->local_count = saved_local_count;
    
    free(source);
    
    if (result == 0) {
        module->is_loaded = 1;
        printf("[MODULE] Successfully loaded module: %s\n", module_name);
        return 0;
    } else {
        module->is_loaded = 0; // Reset to not loaded on failure
        fprintf(stderr, "[MODULE] Failed to execute module: %s\n", module_name);
        return -1;
    }
}

int ember_install_library(const char* library_name, const char* source_path) {
    if (!library_name || !source_path) {
        fprintf(stderr, "[LIBRARY] Invalid parameters for library installation\n");
        return -1;
    }
    
    // Validate library name for security
    if (ember_package_validate_name(library_name) != 0) {
        fprintf(stderr, "[LIBRARY] Invalid library name: %s\n", library_name);
        return -1;
    }
    
    // Initialize package system if not already done
    if (!ember_package_system_init()) {
        fprintf(stderr, "[LIBRARY] Failed to initialize package system\n");
        return -1;
    }
    
    // Check if source path exists and is readable
    if (access(source_path, R_OK) != 0) {
        fprintf(stderr, "[LIBRARY] Source path not found or not readable: %s\n", source_path);
        return -1;
    }
    
    // Create package structure
    EmberPackage package;
    memset(&package, 0, sizeof(EmberPackage));
    
    // Set package details
    strncpy(package.name, library_name, EMBER_PACKAGE_MAX_NAME_LEN - 1);
    package.name[EMBER_PACKAGE_MAX_NAME_LEN - 1] = '\0';
    
    strncpy(package.version, "installed", EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    package.version[EMBER_PACKAGE_MAX_VERSION_LEN - 1] = '\0';
    
    // Determine installation directory
    char install_dir[EMBER_PACKAGE_MAX_PATH_LEN];
    char* home = getenv("HOME");
    if (home) {
        snprintf(install_dir, sizeof(install_dir), "%s/.ember/packages/%s", home, library_name);
    } else {
        snprintf(install_dir, sizeof(install_dir), "/tmp/.ember/packages/%s", library_name);
    }
    
    strncpy(package.local_path, install_dir, EMBER_PACKAGE_MAX_PATH_LEN - 1);
    package.local_path[EMBER_PACKAGE_MAX_PATH_LEN - 1] = '\0';
    
    // Create installation directory
    if (ember_package_create_directory_recursive(install_dir) != 0) {
        fprintf(stderr, "[LIBRARY] Failed to create installation directory: %s\n", install_dir);
        return -1;
    }
    
    // Determine if source is a file or directory
    struct stat source_stat;
    if (stat(source_path, &source_stat) != 0) {
        fprintf(stderr, "[LIBRARY] Failed to stat source path: %s\n", source_path);
        return -1;
    }
    
    if (S_ISREG(source_stat.st_mode)) {
        // Source is a file - copy it to the package directory
        char dest_path[EMBER_PACKAGE_MAX_PATH_LEN];
        
        // If source is a .ember file, copy it as the main package file
        const char* src_ext = strrchr(source_path, '.');
        if (src_ext && strcmp(src_ext, ".ember") == 0) {
            int result = snprintf(dest_path, sizeof(dest_path), "%s/package.ember", install_dir);
            if (result >= (int)sizeof(dest_path)) {
                fprintf(stderr, "[LIBRARY] Destination path too long: %s/package.ember\n", install_dir);
                return -1;
            }
        } else {
            // Copy with original filename
            const char* filename = strrchr(source_path, '/');
            filename = filename ? filename + 1 : source_path;
            int result = snprintf(dest_path, sizeof(dest_path), "%s/%s", install_dir, filename);
            if (result >= (int)sizeof(dest_path)) {
                fprintf(stderr, "[LIBRARY] Destination path too long: %s/%s\n", install_dir, filename);
                return -1;
            }
        }
        
        // Copy file
        FILE* src_file = fopen(source_path, "rb");
        if (!src_file) {
            fprintf(stderr, "[LIBRARY] Failed to open source file: %s\n", source_path);
            return -1;
        }
        
        FILE* dest_file = fopen(dest_path, "wb");
        if (!dest_file) {
            fclose(src_file);
            fprintf(stderr, "[LIBRARY] Failed to create destination file: %s\n", dest_path);
            return -1;
        }
        
        // Copy file contents
        char buffer[4096];
        size_t bytes;
        while ((bytes = fread(buffer, 1, sizeof(buffer), src_file)) > 0) {
            if (fwrite(buffer, 1, bytes, dest_file) != bytes) {
                fclose(src_file);
                fclose(dest_file);
                fprintf(stderr, "[LIBRARY] Failed to write to destination file\n");
                return -1;
            }
        }
        
        fclose(src_file);
        fclose(dest_file);
        
        printf("[LIBRARY] Copied file %s to %s\n", source_path, dest_path);
        
    } else if (S_ISDIR(source_stat.st_mode)) {
        // Source is a directory - copy entire directory structure
        // This is a simplified implementation - in practice, you'd want recursive copy
        char cmd[1024];
        snprintf(cmd, sizeof(cmd), "cp -r %s/* %s/", source_path, install_dir);
        
        // SECURITY NOTE: This is not ideal - in production, implement proper recursive copy
        int result = system(cmd);
        if (result != 0) {
            fprintf(stderr, "[LIBRARY] Failed to copy directory contents\n");
            return -1;
        }
        
        printf("[LIBRARY] Copied directory %s to %s\n", source_path, install_dir);
    } else {
        fprintf(stderr, "[LIBRARY] Source path is neither a file nor a directory: %s\n", source_path);
        return -1;
    }
    
    // Create package manifest if it doesn't exist
    char manifest_path[EMBER_PACKAGE_MAX_PATH_LEN];
    int manifest_result = snprintf(manifest_path, sizeof(manifest_path), "%s/package.toml", install_dir);
    if (manifest_result >= (int)sizeof(manifest_path)) {
        fprintf(stderr, "[LIBRARY] Manifest path too long: %s/package.toml\n", install_dir);
        return -1;
    }
    
    if (access(manifest_path, F_OK) != 0) {
        FILE* manifest_file = fopen(manifest_path, "w");
        if (manifest_file) {
            fprintf(manifest_file, "# Ember Package Manifest\n");
            fprintf(manifest_file, "name = \"%s\"\n", library_name);
            fprintf(manifest_file, "version = \"installed\"\n");
            fprintf(manifest_file, "description = \"Locally installed library\"\n");
            fprintf(manifest_file, "\n[dependencies]\n");
            fclose(manifest_file);
            printf("[LIBRARY] Created package manifest: %s\n", manifest_path);
        }
    }
    
    // Validate package structure
    if (!ember_package_validate_structure(install_dir)) {
        fprintf(stderr, "[LIBRARY] Package validation failed after installation\n");
        return -1;
    }
    
    // Register package in global registry
    package.verified = true;
    package.loaded = false;
    
    EmberPackageRegistry* registry = ember_package_get_global_registry();
    if (registry) {
        ember_package_registry_add(registry, &package);
        printf("[LIBRARY] Registered package in global registry\n");
    }
    
    printf("[LIBRARY] Successfully installed library: %s\n", library_name);
    return 0;
}

char* ember_resolve_module_path(const char* module_name) {
    if (!module_name || strlen(module_name) == 0) {
        return NULL;
    }
    
    // Validate module name for security
    if (ember_package_validate_name(module_name) != 0) {
        fprintf(stderr, "[RESOLVE] Invalid module name: %s\n", module_name);
        return NULL;
    }
    
    char* resolved_path = malloc(EMBER_MAX_PATH_LEN);
    if (!resolved_path) {
        fprintf(stderr, "[RESOLVE] Failed to allocate memory for path resolution\n");
        return NULL;
    }
    
    // Search order:
    // 1. Current directory (./<module_name>.ember)
    // 2. Current directory module subdirectory (./<module_name>/package.ember)
    // 3. User packages directory (~/.ember/packages/<module_name>/package.ember)
    // 4. System packages directory (/usr/lib/ember/<module_name>/package.ember or platform equivalent)
    // 5. Standard library directory (EMBER_SYSTEM_LIB_PATH/<module_name>.ember)
    
    // 1. Current directory - direct file
    snprintf(resolved_path, EMBER_MAX_PATH_LEN, "./%s.ember", module_name);
    if (access(resolved_path, R_OK) == 0) {
        printf("[RESOLVE] Found module in current directory: %s\n", resolved_path);
        return resolved_path;
    }
    
    // 2. Current directory - module subdirectory
    snprintf(resolved_path, EMBER_MAX_PATH_LEN, "./%s/package.ember", module_name);
    if (access(resolved_path, R_OK) == 0) {
        printf("[RESOLVE] Found module in current directory subdirectory: %s\n", resolved_path);
        return resolved_path;
    }
    
    // 3. User packages directory
    const char* home = getenv("HOME");
    if (home) {
        snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/.ember/packages/%s/package.ember", home, module_name);
        if (access(resolved_path, R_OK) == 0) {
            printf("[RESOLVE] Found module in user packages: %s\n", resolved_path);
            return resolved_path;
        }
        
        // Also check for direct .ember file in user packages
        snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/.ember/packages/%s.ember", home, module_name);
        if (access(resolved_path, R_OK) == 0) {
            printf("[RESOLVE] Found module file in user packages: %s\n", resolved_path);
            return resolved_path;
        }
    }
    
    // 4. System packages directory
    snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/%s/package.ember", EMBER_SYSTEM_LIB_PATH, module_name);
    if (access(resolved_path, R_OK) == 0) {
        printf("[RESOLVE] Found module in system packages: %s\n", resolved_path);
        return resolved_path;
    }
    
    // Also check for direct .ember file in system packages
    snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/%s.ember", EMBER_SYSTEM_LIB_PATH, module_name);
    if (access(resolved_path, R_OK) == 0) {
        printf("[RESOLVE] Found module file in system packages: %s\n", resolved_path);
        return resolved_path;
    }
    
    // 5. Standard library directory
    snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/stdlib/%s.ember", EMBER_SYSTEM_LIB_PATH, module_name);
    if (access(resolved_path, R_OK) == 0) {
        printf("[RESOLVE] Found module in standard library: %s\n", resolved_path);
        return resolved_path;
    }
    
    // Check for built-in modules in the current executable's directory
    // This is useful for bundled distributions
    char exec_path[EMBER_MAX_PATH_LEN];
    ssize_t len = readlink("/proc/self/exe", exec_path, sizeof(exec_path) - 1);
    if (len != -1) {
        exec_path[len] = '\0';
        char* last_slash = strrchr(exec_path, '/');
        if (last_slash) {
            *last_slash = '\0';
            int result1 = snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/lib/ember/%s.ember", exec_path, module_name);
            if (result1 >= EMBER_MAX_PATH_LEN) {
                printf("[RESOLVE] Path too long for module: %s/lib/ember/%s.ember\n", exec_path, module_name);
            } else if (access(resolved_path, R_OK) == 0) {
                printf("[RESOLVE] Found module in executable directory: %s\n", resolved_path);
                return resolved_path;
            }
            
            int result2 = snprintf(resolved_path, EMBER_MAX_PATH_LEN, "%s/lib/ember/%s/package.ember", exec_path, module_name);
            if (result2 >= EMBER_MAX_PATH_LEN) {
                printf("[RESOLVE] Path too long for package: %s/lib/ember/%s/package.ember\n", exec_path, module_name);
            } else if (access(resolved_path, R_OK) == 0) {
                printf("[RESOLVE] Found module package in executable directory: %s\n", resolved_path);
                return resolved_path;
            }
        }
    }
    
    // Module not found in any standard location
    fprintf(stderr, "[RESOLVE] Module not found: %s\n", module_name);
    fprintf(stderr, "[RESOLVE] Searched in:\n");
    fprintf(stderr, "  - Current directory: ./%s.ember\n", module_name);
    fprintf(stderr, "  - Current directory: ./%s/package.ember\n", module_name);
    if (home) {
        fprintf(stderr, "  - User packages: %s/.ember/packages/%s/package.ember\n", home, module_name);
    }
    fprintf(stderr, "  - System packages: %s/%s/package.ember\n", EMBER_SYSTEM_LIB_PATH, module_name);
    fprintf(stderr, "  - Standard library: %s/stdlib/%s.ember\n", EMBER_SYSTEM_LIB_PATH, module_name);
    
    free(resolved_path);
    return NULL;
}

void ember_add_module_path(ember_vm* vm, const char* path) {
    if (!vm || !path || strlen(path) == 0) {
        fprintf(stderr, "[MODULE_PATH] Invalid parameters for adding module path\n");
        return;
    }
    
    // Validate path for security - prevent path traversal
    if (strstr(path, "..") != NULL) {
        fprintf(stderr, "[MODULE_PATH] Path contains directory traversal: %s\n", path);
        return;
    }
    
    // Check if we have space for more paths
    if (vm->module_path_count >= 8) { // Based on the VM structure
        fprintf(stderr, "[MODULE_PATH] Maximum module path limit reached (8)\n");
        return;
    }
    
    // Check if path already exists
    for (int i = 0; i < vm->module_path_count; i++) {
        if (vm->module_paths[i] && strcmp(vm->module_paths[i], path) == 0) {
            printf("[MODULE_PATH] Path already exists in module paths: %s\n", path);
            return;
        }
    }
    
    // Validate that path exists and is a directory
    struct stat path_stat;
    if (stat(path, &path_stat) != 0) {
        fprintf(stderr, "[MODULE_PATH] Path does not exist: %s\n", path);
        return;
    }
    
    if (!S_ISDIR(path_stat.st_mode)) {
        fprintf(stderr, "[MODULE_PATH] Path is not a directory: %s\n", path);
        return;
    }
    
    // Check if path is readable
    if (access(path, R_OK) != 0) {
        fprintf(stderr, "[MODULE_PATH] Path is not readable: %s\n", path);
        return;
    }
    
    // Allocate memory for the path string
    char* path_copy = malloc(strlen(path) + 1);
    if (!path_copy) {
        fprintf(stderr, "[MODULE_PATH] Failed to allocate memory for path: %s\n", path);
        return;
    }
    
    strcpy(path_copy, path);
    
    // Add path to the VM's module paths
    vm->module_paths[vm->module_path_count] = path_copy;
    vm->module_path_count++;
    
    printf("[MODULE_PATH] Added module path: %s\n", path);
    printf("[MODULE_PATH] Total module paths: %d\n", vm->module_path_count);
    
    // Print all current module paths for debugging
    printf("[MODULE_PATH] Current module search paths:\n");
    for (int i = 0; i < vm->module_path_count; i++) {
        printf("  %d: %s\n", i + 1, vm->module_paths[i]);
    }
}

int ember_call(ember_vm* vm, const char* func_name, int argc, ember_value* argv) {
    // Validate input parameters
    if (!vm) {
        fprintf(stderr, "[CALL] VM is null\n");
        return -1;
    }
    
    if (!func_name || strlen(func_name) == 0) {
        fprintf(stderr, "[CALL] Function name is null or empty\n");
        return -1;
    }
    
    if (argc < 0) {
        fprintf(stderr, "[CALL] Invalid argument count: %d\n", argc);
        return -1;
    }
    
    if (argc > 0 && !argv) {
        fprintf(stderr, "[CALL] Arguments array is null but argc > 0\n");
        return -1;
    }
    
    // Find function in globals
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->globals[i].key, func_name) == 0) {
            ember_value func_val = vm->globals[i].value;
            
            if (func_val.type == EMBER_VAL_NATIVE) {
                // Call native function directly
                ember_value result = func_val.as.native_val(vm, argc, argv);
                // Push result onto stack for consistency
                push(vm, result);
                return 0;
            } else if (func_val.type == EMBER_VAL_FUNCTION) {
                // Call user-defined function
                if (!func_val.as.func_val.chunk) {
                    fprintf(stderr, "[CALL] Function '%s' has no bytecode chunk\n", func_name);
                    return -1;
                }
                
                // Security check: prevent stack-based buffer overflow
                if (argc > EMBER_MAX_ARGS) {
                    fprintf(stderr, "[CALL] Too many arguments: %d (max: %d)\n", argc, EMBER_MAX_ARGS);
                    return -1;
                }
                
                // Save current VM state (like in VM OP_CALL)
                ember_chunk* saved_chunk = vm->chunk;
                uint8_t* saved_ip = vm->ip;
                int saved_local_count = vm->local_count;
                
                // Set up parameters as local variables (like in VM OP_CALL)
                // Arguments are added to the current local scope
                for (int i = 0; i < argc && vm->local_count < EMBER_MAX_LOCALS; i++) {
                    vm->locals[vm->local_count++] = argv[i];
                }
                
                // Switch to function chunk
                vm->chunk = func_val.as.func_val.chunk;
                vm->ip = func_val.as.func_val.chunk->code;
                
                // Run function
                int func_result = ember_run(vm);
                
                // Restore VM state (like in VM OP_CALL)
                vm->chunk = saved_chunk;
                vm->ip = saved_ip;
                vm->local_count = saved_local_count;
                
                // Return value should already be on stack from OP_RETURN
                
                return func_result;
            }
        }
    }
    // Function not found
    fprintf(stderr, "[CALL] Function '%s' not found in global scope\n", func_name);
    return -1;
}

void ember_print_value(ember_value value) {
    print_value(value);
}

// All native functions have been moved to their respective modules:
// - Runtime built-ins: src/runtime/builtins.c  
// - Math functions: src/stdlib/math_native.c
// - String functions: src/stdlib/string_native.c
// - File I/O functions: src/stdlib/io_native.c
// This API file now contains only the clean public embedding interface