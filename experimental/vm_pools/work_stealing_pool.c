#ifndef _GNU_SOURCE
#define _GNU_SOURCE
#endif
#include "work_stealing_pool.h"
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/sysinfo.h>
#include <sys/time.h>
#include <errno.h>
#include <sched.h>
#include <numa.h>
#include <math.h>
#include <stdatomic.h>

// Branch prediction hints
#ifdef __GNUC__
#define LIKELY(x)       __builtin_expect(!!(x), 1)
#define UNLIKELY(x)     __builtin_expect(!!(x), 0)
#else
#define LIKELY(x)       (x)
#define UNLIKELY(x)     (x)
#endif

// Memory barriers
#define MEMORY_BARRIER()    __sync_synchronize()
#define LOAD_BARRIER()      __asm__ __volatile__("" ::: "memory")
#define STORE_BARRIER()     __asm__ __volatile__("" ::: "memory")

// Atomic operations compatibility
#ifndef atomic_load_explicit
#define atomic_load_explicit(ptr, order) __atomic_load_n(ptr, order)
#define atomic_store_explicit(ptr, val, order) __atomic_store_n(ptr, val, order)
#define atomic_compare_exchange_weak_explicit(ptr, expected, desired, success_order, fail_order) \
    __atomic_compare_exchange_n(ptr, expected, desired, 1, success_order, fail_order)
#define atomic_fetch_add_explicit(ptr, val, order) __atomic_fetch_add(ptr, val, order)
#define memory_order_relaxed __ATOMIC_RELAXED
#define memory_order_acquire __ATOMIC_ACQUIRE
#define memory_order_release __ATOMIC_RELEASE
#define memory_order_acq_rel __ATOMIC_ACQ_REL
#define memory_order_seq_cst __ATOMIC_SEQ_CST
#endif

// Default configuration
const thread_pool_config_t DEFAULT_THREAD_POOL_CONFIG = {
    .min_threads = 2,
    .max_threads = 0,  // 0 means use CPU count * 2
    .initial_threads = 0,  // 0 means use CPU count
    .load_factor_expand = 0.8,
    .load_factor_shrink = 0.3,
    .resize_cooldown_ns = WORK_STEALING_RESIZE_COOLDOWN,
    .idle_timeout_ns = 60000000000ULL,  // 60 seconds
    .enable_work_stealing = true,
    .enable_numa_awareness = true,
    .enable_cpu_affinity = true,
    .deque_initial_capacity = WORK_STEALING_DEFAULT_DEQUE_SIZE
};

// Forward declarations
static void* worker_thread_main(void* arg);
static work_task_t* try_steal_work(work_stealing_pool_t* pool, worker_thread_t* worker);
static int distribute_task_to_worker(work_stealing_pool_t* pool, work_task_t* task);
static void update_worker_load_scores(work_stealing_pool_t* pool);
static int resize_deque(work_stealing_deque_t* deque, size_t new_capacity);
static void exponential_backoff(uint64_t* backoff_ns);

// ============================================================================
// UTILITY FUNCTIONS
// ============================================================================

uint64_t get_timestamp_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return (uint64_t)ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

int get_optimal_thread_count(void) {
    int cpu_count = get_nprocs();
    return cpu_count > 0 ? cpu_count : 4;
}

size_t get_cache_line_size(void) {
    long cache_line_size = sysconf(_SC_LEVEL1_DCACHE_LINESIZE);
    return cache_line_size > 0 ? (size_t)cache_line_size : WORK_STEALING_CACHE_LINE_SIZE;
}

static void exponential_backoff(uint64_t* backoff_ns) {
    if (*backoff_ns < WORK_STEALING_BACKOFF_MIN_NS) {
        *backoff_ns = WORK_STEALING_BACKOFF_MIN_NS;
    } else {
        *backoff_ns = (*backoff_ns * 2);
        if (*backoff_ns > WORK_STEALING_BACKOFF_MAX_NS) {
            *backoff_ns = WORK_STEALING_BACKOFF_MAX_NS;
        }
    }
    
    struct timespec sleep_time = {
        .tv_sec = *backoff_ns / 1000000000ULL,
        .tv_nsec = *backoff_ns % 1000000000ULL
    };
    nanosleep(&sleep_time, NULL);
}

// ============================================================================
// NUMA TOPOLOGY FUNCTIONS
// ============================================================================

int numa_topology_init(numa_topology_t* topology) {
    if (!topology) {
        return -1;
    }
    
    memset(topology, 0, sizeof(numa_topology_t));
    
    // Check if NUMA is available
    if (numa_available() == -1) {
        topology->numa_available = false;
        topology->node_count = 1;
        
        // Fallback: treat system as single NUMA node
        int cpu_count = get_nprocs();
        topology->cpu_to_node = calloc(cpu_count, sizeof(int));
        topology->node_cpus = malloc(sizeof(int*));
        topology->node_cpu_counts = malloc(sizeof(size_t));
        
        if (!topology->cpu_to_node || !topology->node_cpus || !topology->node_cpu_counts) {
            numa_topology_cleanup(topology);
            return -1;
        }
        
        topology->node_cpus[0] = calloc(cpu_count, sizeof(int));
        if (!topology->node_cpus[0]) {
            numa_topology_cleanup(topology);
            return -1;
        }
        
        for (int i = 0; i < cpu_count; i++) {
            topology->cpu_to_node[i] = 0;
            topology->node_cpus[0][i] = i;
        }
        topology->node_cpu_counts[0] = cpu_count;
        
        return 0;
    }
    
    topology->numa_available = true;
    topology->node_count = numa_max_node() + 1;
    
    int cpu_count = get_nprocs();
    topology->cpu_to_node = calloc(cpu_count, sizeof(int));
    topology->node_cpus = calloc(topology->node_count, sizeof(int*));
    topology->node_cpu_counts = calloc(topology->node_count, sizeof(size_t));
    
    if (!topology->cpu_to_node || !topology->node_cpus || !topology->node_cpu_counts) {
        numa_topology_cleanup(topology);
        return -1;
    }
    
    // Initialize CPU to node mapping
    for (int cpu = 0; cpu < cpu_count; cpu++) {
        topology->cpu_to_node[cpu] = numa_node_of_cpu(cpu);
        if (topology->cpu_to_node[cpu] < 0) {
            topology->cpu_to_node[cpu] = 0;  // Fallback to node 0
        }
    }
    
    // Count CPUs per node
    for (int cpu = 0; cpu < cpu_count; cpu++) {
        int node = topology->cpu_to_node[cpu];
        if (node >= 0 && node < topology->node_count) {
            topology->node_cpu_counts[node]++;
        }
    }
    
    // Allocate and populate node CPU lists
    for (int node = 0; node < topology->node_count; node++) {
        if (topology->node_cpu_counts[node] > 0) {
            topology->node_cpus[node] = calloc(topology->node_cpu_counts[node], sizeof(int));
            if (!topology->node_cpus[node]) {
                numa_topology_cleanup(topology);
                return -1;
            }
        }
    }
    
    // Populate node CPU lists
    size_t* node_indices = calloc(topology->node_count, sizeof(size_t));
    if (!node_indices) {
        numa_topology_cleanup(topology);
        return -1;
    }
    
    for (int cpu = 0; cpu < cpu_count; cpu++) {
        int node = topology->cpu_to_node[cpu];
        if (node >= 0 && node < topology->node_count && topology->node_cpus[node]) {
            topology->node_cpus[node][node_indices[node]++] = cpu;
        }
    }
    
    free(node_indices);
    return 0;
}

void numa_topology_cleanup(numa_topology_t* topology) {
    if (!topology) {
        return;
    }
    
    free(topology->cpu_to_node);
    
    if (topology->node_cpus) {
        for (int i = 0; i < topology->node_count; i++) {
            free(topology->node_cpus[i]);
        }
        free(topology->node_cpus);
    }
    
    free(topology->node_cpu_counts);
    memset(topology, 0, sizeof(numa_topology_t));
}

int numa_get_cpu_node(numa_topology_t* topology, int cpu) {
    if (!topology || !topology->cpu_to_node || cpu < 0 || cpu >= get_nprocs()) {
        return -1;
    }
    return topology->cpu_to_node[cpu];
}

int numa_get_node_cpus(numa_topology_t* topology, int node, int** cpus, size_t* count) {
    if (!topology || !cpus || !count || node < 0 || node >= topology->node_count) {
        return -1;
    }
    
    *cpus = topology->node_cpus[node];
    *count = topology->node_cpu_counts[node];
    return 0;
}

// ============================================================================
// CPU MONITORING FUNCTIONS
// ============================================================================

int cpu_monitor_init(cpu_monitor_t* monitor) {
    if (!monitor) {
        return -1;
    }
    
    memset(monitor, 0, sizeof(cpu_monitor_t));
    
    monitor->cpu_count = get_nprocs();
    monitor->cpu_usage = calloc(monitor->cpu_count, sizeof(double));
    monitor->idle_times = calloc(monitor->cpu_count, sizeof(uint64_t));
    monitor->total_times = calloc(monitor->cpu_count, sizeof(uint64_t));
    
    if (!monitor->cpu_usage || !monitor->idle_times || !monitor->total_times) {
        cpu_monitor_cleanup(monitor);
        return -1;
    }
    
    return cpu_monitor_update(monitor);
}

void cpu_monitor_cleanup(cpu_monitor_t* monitor) {
    if (!monitor) {
        return;
    }
    
    free(monitor->cpu_usage);
    free(monitor->idle_times);
    free(monitor->total_times);
    memset(monitor, 0, sizeof(cpu_monitor_t));
}

int cpu_monitor_update(cpu_monitor_t* monitor) {
    if (!monitor) {
        return -1;
    }
    
    FILE* stat_file = fopen("/proc/stat", "r");
    if (!stat_file) {
        return -1;
    }
    
    char line[256];
    int cpu_index = -1;
    
    while (fgets(line, sizeof(line), stat_file)) {
        if (strncmp(line, "cpu", 3) == 0) {
            if (line[3] == ' ') {
                // Overall CPU line, skip for now
                continue;
            } else if (line[3] >= '0' && line[3] <= '9') {
                // Individual CPU line
                cpu_index++;
                if (cpu_index >= monitor->cpu_count) {
                    break;
                }
                
                uint64_t user, nice, system, idle, iowait, irq, softirq, steal;
                int parsed = sscanf(line, "cpu%*d %lu %lu %lu %lu %lu %lu %lu %lu",
                                  &user, &nice, &system, &idle, &iowait, &irq, &softirq, &steal);
                
                if (parsed >= 4) {
                    uint64_t total = user + nice + system + idle + iowait + irq + softirq + steal;
                    uint64_t prev_idle = monitor->idle_times[cpu_index];
                    uint64_t prev_total = monitor->total_times[cpu_index];
                    
                    if (prev_total > 0) {
                        uint64_t total_diff = total - prev_total;
                        uint64_t idle_diff = idle - prev_idle;
                        
                        if (total_diff > 0) {
                            monitor->cpu_usage[cpu_index] = 100.0 * (1.0 - (double)idle_diff / total_diff);
                        }
                    }
                    
                    monitor->idle_times[cpu_index] = idle;
                    monitor->total_times[cpu_index] = total;
                }
            }
        }
    }
    
    fclose(stat_file);
    
    // Update system load average
    double loadavg[3];
    if (getloadavg(loadavg, 1) == 1) {
        monitor->system_load_avg = loadavg[0];
    }
    
    monitor->last_update = get_timestamp_ns();
    return 0;
}

double cpu_monitor_get_usage(cpu_monitor_t* monitor, int cpu) {
    if (!monitor || cpu < 0 || cpu >= monitor->cpu_count) {
        return 0.0;
    }
    return monitor->cpu_usage[cpu];
}

double cpu_monitor_get_system_load(cpu_monitor_t* monitor) {
    if (!monitor) {
        return 0.0;
    }
    return monitor->system_load_avg;
}

// ============================================================================
// WORK-STEALING DEQUE IMPLEMENTATION
// ============================================================================

work_stealing_deque_t* work_stealing_deque_create(size_t initial_capacity) {
    if (initial_capacity == 0) {
        initial_capacity = WORK_STEALING_DEFAULT_DEQUE_SIZE;
    }
    
    // Ensure capacity is power of 2
    size_t capacity = 1;
    while (capacity < initial_capacity) {
        capacity <<= 1;
    }
    
    work_stealing_deque_t* deque = aligned_alloc(get_cache_line_size(), 
                                                sizeof(work_stealing_deque_t));
    if (!deque) {
        return NULL;
    }
    
    deque->tasks = calloc(capacity, sizeof(work_task_t*));
    if (!deque->tasks) {
        free(deque);
        return NULL;
    }
    
    deque->capacity = capacity;
    deque->mask = capacity - 1;
    deque->top = 0;
    deque->bottom = 0;
    
    if (pthread_mutex_init(&deque->resize_mutex, NULL) != 0) {
        free(deque->tasks);
        free(deque);
        return NULL;
    }
    
    // Initialize statistics
    deque->tasks_pushed = 0;
    deque->tasks_popped = 0;
    deque->tasks_stolen = 0;
    deque->steal_attempts = 0;
    deque->failed_steals = 0;
    
    return deque;
}

void work_stealing_deque_destroy(work_stealing_deque_t* deque) {
    if (!deque) {
        return;
    }
    
    // Free any remaining tasks
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_relaxed);
    
    for (uint64_t i = top; i < bottom; i++) {
        volatile work_task_t** tasks = atomic_load_explicit(&deque->tasks, memory_order_acquire);
        work_task_t* task = (work_task_t*)tasks[i & deque->mask];
        free(task);
    }
    
    free(deque->tasks);
    pthread_mutex_destroy(&deque->resize_mutex);
    free(deque);
}

static int resize_deque(work_stealing_deque_t* deque, size_t new_capacity) {
    if (!deque || new_capacity <= deque->capacity || new_capacity > WORK_STEALING_MAX_DEQUE_SIZE) {
        return -1;
    }
    
    // Ensure new capacity is power of 2
    size_t capacity = deque->capacity;
    while (capacity < new_capacity) {
        capacity <<= 1;
    }
    
    work_task_t** new_tasks = calloc(capacity, sizeof(work_task_t*));
    if (!new_tasks) {
        return -1;
    }
    
    // Copy existing tasks
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_relaxed);
    
    for (uint64_t i = top; i < bottom; i++) {
        volatile work_task_t** tasks = atomic_load_explicit(&deque->tasks, memory_order_acquire);
        new_tasks[i & (capacity - 1)] = (work_task_t*)tasks[i & deque->mask];
    }
    
    work_task_t** old_tasks = (work_task_t**)deque->tasks;
    atomic_store_explicit((volatile work_task_t***)&deque->tasks, 
                         (volatile work_task_t**)new_tasks, memory_order_release);
    deque->capacity = capacity;
    deque->mask = capacity - 1;
    
    MEMORY_BARRIER();
    
    free(old_tasks);
    return 0;
}

int work_stealing_deque_push_bottom(work_stealing_deque_t* deque, work_task_t* task) {
    if (!deque || !task) {
        return -1;
    }
    
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_acquire);
    
    // Check if deque is full (leave one slot empty to distinguish full from empty)
    if (bottom - top >= deque->capacity - 1) {
        // Try to resize if not at maximum capacity
        if (pthread_mutex_trylock(&deque->resize_mutex) == 0) {
            if (deque->capacity < WORK_STEALING_MAX_DEQUE_SIZE) {
                resize_deque(deque, deque->capacity * 2);
            }
            pthread_mutex_unlock(&deque->resize_mutex);
        }
        
        // Check again after potential resize
        if (bottom - top >= deque->capacity - 1) {
            return -1;  // Deque is full
        }
    }
    
    volatile work_task_t** tasks = atomic_load_explicit(&deque->tasks, memory_order_acquire);
    tasks[bottom & deque->mask] = task;
    STORE_BARRIER();
    atomic_store_explicit(&deque->bottom, bottom + 1, memory_order_release);
    
    deque->tasks_pushed++;
    return 0;
}

work_task_t* work_stealing_deque_pop_bottom(work_stealing_deque_t* deque) {
    if (!deque) {
        return NULL;
    }
    
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    if (bottom == 0) {
        return NULL;
    }
    
    bottom--;
    atomic_store_explicit(&deque->bottom, bottom, memory_order_relaxed);
    MEMORY_BARRIER();
    
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_relaxed);
    
    if (top <= bottom) {
        // Non-empty deque
        volatile work_task_t** tasks = atomic_load_explicit(&deque->tasks, memory_order_acquire);
        work_task_t* task = (work_task_t*)tasks[bottom & deque->mask];
        
        if (top == bottom) {
            // Last element, need to compete with potential thieves
            uint64_t expected_top = top;
            if (!atomic_compare_exchange_weak_explicit(&deque->top, &expected_top, top + 1,
                                                     memory_order_seq_cst, memory_order_relaxed)) {
                // Lost the race to a thief
                task = NULL;
            }
            atomic_store_explicit(&deque->bottom, bottom + 1, memory_order_relaxed);
        }
        
        if (task) {
            deque->tasks_popped++;
        }
        return task;
    } else {
        // Deque was empty
        atomic_store_explicit(&deque->bottom, bottom + 1, memory_order_relaxed);
        return NULL;
    }
}

work_task_t* work_stealing_deque_steal_top(work_stealing_deque_t* deque) {
    if (!deque) {
        return NULL;
    }
    
    deque->steal_attempts++;
    
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_acquire);
    LOAD_BARRIER();
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_acquire);
    
    if (top >= bottom) {
        deque->failed_steals++;
        return NULL;  // Deque is empty
    }
    
    volatile work_task_t** tasks = atomic_load_explicit(&deque->tasks, memory_order_acquire);
    work_task_t* task = (work_task_t*)tasks[top & deque->mask];
    uint64_t expected_top = top;
    
    if (atomic_compare_exchange_weak_explicit(&deque->top, &expected_top, top + 1,
                                            memory_order_seq_cst, memory_order_relaxed)) {
        deque->tasks_stolen++;
        return task;
    } else {
        deque->failed_steals++;
        return NULL;  // Lost the race
    }
}

size_t work_stealing_deque_size(work_stealing_deque_t* deque) {
    if (!deque) {
        return 0;
    }
    
    uint64_t bottom = atomic_load_explicit(&deque->bottom, memory_order_relaxed);
    uint64_t top = atomic_load_explicit(&deque->top, memory_order_relaxed);
    
    return bottom > top ? bottom - top : 0;
}

bool work_stealing_deque_is_empty(work_stealing_deque_t* deque) {
    return work_stealing_deque_size(deque) == 0;
}

// ============================================================================
// WORKER THREAD IMPLEMENTATION
// ============================================================================

static void* worker_thread_main(void* arg) {
    worker_thread_t* worker = (worker_thread_t*)arg;
    work_stealing_pool_t* pool = worker->pool;
    uint64_t backoff_ns = 0;
    uint64_t last_activity = get_timestamp_ns();
    
    // Set thread name for debugging
    char thread_name[16];
    snprintf(thread_name, sizeof(thread_name), "ember-worker-%zu", worker->worker_id);
    pthread_setname_np(pthread_self(), thread_name);
    
    // Set CPU affinity if enabled
    if (pool->config.enable_cpu_affinity && worker->preferred_cpu >= 0) {
        CPU_ZERO(&worker->affinity_mask);
        CPU_SET(worker->preferred_cpu, &worker->affinity_mask);
        set_thread_affinity(pthread_self(), &worker->affinity_mask);
    }
    
    while (!worker->should_terminate && !pool->is_shutdown) {
        work_task_t* task = NULL;
        uint64_t task_start_time = 0;
        
        // Try to get task from own deque first
        task = work_stealing_deque_pop_bottom(worker->deque);
        
        if (!task && pool->config.enable_work_stealing) {
            // Try to steal work from other workers
            worker->state = WORKER_STATE_STEALING;
            task = try_steal_work(pool, worker);
            if (task) {
                worker->tasks_stolen_by_me++;
            }
        }
        
        if (!task) {
            // No work available, check if we should terminate due to idleness
            uint64_t current_time = get_timestamp_ns();
            uint64_t idle_duration = current_time - last_activity;
            
            if (idle_duration > pool->config.idle_timeout_ns && 
                pool->current_thread_count > pool->config.min_threads) {
                // Thread has been idle too long and we have more than minimum threads
                break;
            }
            
            worker->state = WORKER_STATE_IDLE;
            worker->total_idle_time += current_time - last_activity;
            
            // Exponential backoff to reduce CPU usage when idle
            exponential_backoff(&backoff_ns);
            continue;
        }
        
        // Reset backoff on successful work acquisition
        backoff_ns = 0;
        last_activity = get_timestamp_ns();
        
        // Execute the task
        worker->state = WORKER_STATE_WORKING;
        task_start_time = get_timestamp_ns();
        
        if (task->function) {
            task->function(task->data);
        }
        
        uint64_t task_end_time = get_timestamp_ns();
        uint64_t task_duration = task_end_time - task_start_time;
        
        // Update worker statistics
        worker->tasks_executed++;
        worker->total_execution_time += task_duration;
        worker->last_activity = task_end_time;
        
        // Update average task duration using exponential moving average
        if (worker->avg_task_duration == 0.0) {
            worker->avg_task_duration = task_duration / 1000000.0;  // Convert to milliseconds
        } else {
            worker->avg_task_duration = 0.9 * worker->avg_task_duration + 
                                      0.1 * (task_duration / 1000000.0);
        }
        
        // Update pool statistics
        pthread_mutex_lock(&pool->global_mutex);
        pool->stats.total_tasks_completed++;
        
        uint64_t wait_time = task_start_time - task->submit_time;
        if (pool->stats.avg_task_wait_time == 0.0) {
            pool->stats.avg_task_wait_time = wait_time / 1000000.0;
        } else {
            pool->stats.avg_task_wait_time = 0.95 * pool->stats.avg_task_wait_time +
                                           0.05 * (wait_time / 1000000.0);
        }
        
        if (pool->stats.avg_task_execution_time == 0.0) {
            pool->stats.avg_task_execution_time = task_duration / 1000000.0;
        } else {
            pool->stats.avg_task_execution_time = 0.95 * pool->stats.avg_task_execution_time +
                                                0.05 * (task_duration / 1000000.0);
        }
        pthread_mutex_unlock(&pool->global_mutex);
        
        // Clean up task
        free(task);
    }
    
    worker->state = WORKER_STATE_TERMINATING;
    return NULL;
}

static work_task_t* try_steal_work(work_stealing_pool_t* pool, worker_thread_t* worker) {
    if (!pool || !worker) {
        return NULL;
    }
    
    // Try to steal from other workers in the same NUMA node first
    for (int attempt = 0; attempt < WORK_STEALING_STEAL_RETRIES; attempt++) {
        // Start with workers in the same NUMA node
        for (size_t i = 0; i < pool->current_thread_count; i++) {
            if (i == worker->worker_id) {
                continue;  // Skip self
            }
            
            worker_thread_t* target = &pool->workers[i];
            
            // Prefer stealing from workers in the same NUMA node
            if (pool->config.enable_numa_awareness && 
                target->numa_node != worker->numa_node) {
                continue;
            }
            
            work_task_t* stolen_task = work_stealing_deque_steal_top(target->deque);
            if (stolen_task) {
                target->tasks_stolen_from_me++;
                pool->stats.successful_steals++;
                return stolen_task;
            }
        }
        
        // If no work found in same NUMA node, try other nodes
        if (pool->config.enable_numa_awareness) {
            for (size_t i = 0; i < pool->current_thread_count; i++) {
                if (i == worker->worker_id) {
                    continue;
                }
                
                worker_thread_t* target = &pool->workers[i];
                
                // Only try different NUMA nodes now
                if (target->numa_node == worker->numa_node) {
                    continue;
                }
                
                work_task_t* stolen_task = work_stealing_deque_steal_top(target->deque);
                if (stolen_task) {
                    target->tasks_stolen_from_me++;
                    pool->stats.successful_steals++;
                    return stolen_task;
                }
            }
        }
        
        // Brief pause before next steal attempt
        struct timespec pause = { .tv_sec = 0, .tv_nsec = 1000 };  // 1 microsecond
        nanosleep(&pause, NULL);
    }
    
    pool->stats.total_steal_operations++;
    return NULL;
}

// ============================================================================
// THREAD POOL CORE IMPLEMENTATION
// ============================================================================

work_stealing_pool_t* work_stealing_pool_create(const thread_pool_config_t* config) {
    work_stealing_pool_t* pool = aligned_alloc(get_cache_line_size(), 
                                              sizeof(work_stealing_pool_t));
    if (!pool) {
        return NULL;
    }
    
    memset(pool, 0, sizeof(work_stealing_pool_t));
    
    // Copy configuration or use defaults
    if (config) {
        pool->config = *config;
    } else {
        pool->config = DEFAULT_THREAD_POOL_CONFIG;
    }
    
    // Set default values if not specified
    if (pool->config.max_threads == 0) {
        pool->config.max_threads = get_optimal_thread_count() * 2;
    }
    if (pool->config.initial_threads == 0) {
        pool->config.initial_threads = get_optimal_thread_count();
    }
    
    // Validate configuration
    if (pool->config.initial_threads < pool->config.min_threads) {
        pool->config.initial_threads = pool->config.min_threads;
    }
    if (pool->config.initial_threads > pool->config.max_threads) {
        pool->config.initial_threads = pool->config.max_threads;
    }
    
    pool->creation_time = get_timestamp_ns();
    pool->is_shutdown = false;
    pool->is_paused = false;
    pool->current_thread_count = 0;
    pool->target_thread_count = pool->config.initial_threads;
    
    // Initialize mutexes and condition variables
    if (pthread_mutex_init(&pool->global_mutex, NULL) != 0 ||
        pthread_cond_init(&pool->pool_condition, NULL) != 0) {
        free(pool);
        return NULL;
    }
    
    // Initialize NUMA topology
    if (pool->config.enable_numa_awareness) {
        if (numa_topology_init(&pool->numa_topology) != 0) {
            pool->config.enable_numa_awareness = false;
        }
    }
    
    // Initialize CPU monitor
    if (cpu_monitor_init(&pool->cpu_monitor) != 0) {
        // CPU monitoring is optional, continue without it
    }
    
    // Create global queue for initial task distribution
    pool->global_queue = work_stealing_deque_create(pool->config.deque_initial_capacity * 2);
    if (!pool->global_queue) {
        work_stealing_pool_destroy(pool);
        return NULL;
    }
    
    // Allocate worker arrays
    pool->workers = calloc(pool->config.max_threads, sizeof(worker_thread_t));
    pool->worker_load_scores = calloc(pool->config.max_threads, sizeof(uint64_t));
    
    if (!pool->workers || !pool->worker_load_scores) {
        work_stealing_pool_destroy(pool);
        return NULL;
    }
    
    // Initialize back-pressure control
    pool->backpressure.max_pending_tasks = pool->config.max_threads * 100;  // 100 tasks per thread
    pool->backpressure.high_water_mark = pool->backpressure.max_pending_tasks * 0.8;
    pool->backpressure.low_water_mark = pool->backpressure.max_pending_tasks * 0.3;
    pool->backpressure.rejection_probability = 0.0;
    pool->backpressure.adaptive_rejection = true;
    
    return pool;
}

void work_stealing_pool_destroy(work_stealing_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    // Shutdown the pool if not already done
    if (!pool->is_shutdown) {
        work_stealing_pool_shutdown(pool, true);
    }
    
    // Cleanup workers
    if (pool->workers) {
        for (size_t i = 0; i < pool->current_thread_count; i++) {
            worker_thread_t* worker = &pool->workers[i];
            if (worker->deque) {
                work_stealing_deque_destroy(worker->deque);
            }
            pthread_mutex_destroy(&worker->worker_mutex);
            pthread_cond_destroy(&worker->work_available);
        }
        free(pool->workers);
    }
    
    free(pool->worker_load_scores);
    
    // Cleanup global queue
    if (pool->global_queue) {
        work_stealing_deque_destroy(pool->global_queue);
    }
    
    // Cleanup NUMA topology
    numa_topology_cleanup(&pool->numa_topology);
    
    // Cleanup CPU monitor
    cpu_monitor_cleanup(&pool->cpu_monitor);
    
    // Cleanup synchronization objects
    pthread_mutex_destroy(&pool->global_mutex);
    pthread_cond_destroy(&pool->pool_condition);
    
    free(pool);
}

int work_stealing_pool_start(work_stealing_pool_t* pool) {
    if (!pool || pool->is_shutdown) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    
    // Create initial worker threads
    for (size_t i = 0; i < pool->config.initial_threads; i++) {
        worker_thread_t* worker = &pool->workers[i];
        
        worker->worker_id = i;
        worker->pool = pool;
        worker->state = WORKER_STATE_IDLE;
        worker->should_terminate = false;
        worker->numa_node = 0;  // Default to node 0
        worker->preferred_cpu = -1;
        
        // Set NUMA node and preferred CPU
        if (pool->config.enable_numa_awareness && pool->numa_topology.numa_available) {
            worker->numa_node = i % pool->numa_topology.node_count;
            
            int* node_cpus;
            size_t cpu_count;
            if (numa_get_node_cpus(&pool->numa_topology, worker->numa_node, 
                                  &node_cpus, &cpu_count) == 0 && cpu_count > 0) {
                worker->preferred_cpu = node_cpus[i % cpu_count];
            }
        } else {
            worker->preferred_cpu = i % get_nprocs();
        }
        
        // Create worker deque
        worker->deque = work_stealing_deque_create(pool->config.deque_initial_capacity);
        if (!worker->deque) {
            pthread_mutex_unlock(&pool->global_mutex);
            return -1;
        }
        
        // Initialize synchronization objects
        if (pthread_mutex_init(&worker->worker_mutex, NULL) != 0 ||
            pthread_cond_init(&worker->work_available, NULL) != 0) {
            work_stealing_deque_destroy(worker->deque);
            pthread_mutex_unlock(&pool->global_mutex);
            return -1;
        }
        
        // Create the worker thread
        if (pthread_create(&worker->thread_id, NULL, worker_thread_main, worker) != 0) {
            work_stealing_deque_destroy(worker->deque);
            pthread_mutex_destroy(&worker->worker_mutex);
            pthread_cond_destroy(&worker->work_available);
            pthread_mutex_unlock(&pool->global_mutex);
            return -1;
        }
        
        pool->current_thread_count++;
    }
    
    pthread_mutex_unlock(&pool->global_mutex);
    return 0;
}

int work_stealing_pool_shutdown(work_stealing_pool_t* pool, bool wait_for_completion) {
    if (!pool) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    pool->is_shutdown = true;
    
    // Signal all workers to terminate
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        pool->workers[i].should_terminate = true;
        pthread_cond_signal(&pool->workers[i].work_available);
    }
    
    pthread_mutex_unlock(&pool->global_mutex);
    
    if (wait_for_completion) {
        // Wait for all worker threads to finish
        for (size_t i = 0; i < pool->current_thread_count; i++) {
            pthread_join(pool->workers[i].thread_id, NULL);
        }
    }
    
    return 0;
}

// ============================================================================
// TASK SUBMISSION AND DISTRIBUTION
// ============================================================================

work_task_t* work_stealing_pool_create_task(task_function_t function, void* data,
                                           task_priority_t priority, task_type_t type) {
    if (!function) {
        return NULL;
    }
    
    work_task_t* task = malloc(sizeof(work_task_t));
    if (!task) {
        return NULL;
    }
    
    task->function = function;
    task->data = data;
    task->priority = priority;
    task->type = type;
    task->submit_time = get_timestamp_ns();
    task->deadline = 0;  // No deadline by default
    task->cpu_affinity_mask = 0;  // No affinity preference by default
    task->estimated_duration_ns = 0;  // Unknown duration
    task->next = NULL;
    
    return task;
}

static int distribute_task_to_worker(work_stealing_pool_t* pool, work_task_t* task) {
    if (!pool || !task || pool->current_thread_count == 0) {
        return -1;
    }
    
    size_t best_worker_id = 0;
    uint64_t best_load_score = UINT64_MAX;
    
    // Find the least loaded worker
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        worker_thread_t* worker = &pool->workers[i];
        
        // Skip workers that are terminating
        if (worker->should_terminate || worker->state == WORKER_STATE_TERMINATING) {
            continue;
        }
        
        // Calculate load score (lower is better)
        uint64_t load_score = work_stealing_deque_size(worker->deque) * 1000 +
                             pool->worker_load_scores[i];
        
        // Prefer workers in the same NUMA node if CPU affinity is specified
        if (task->cpu_affinity_mask != 0 && pool->config.enable_numa_awareness) {
            int task_numa_node = -1;
            // Find the NUMA node for the preferred CPU
            for (int cpu = 0; cpu < 32 && task_numa_node == -1; cpu++) {
                if (task->cpu_affinity_mask & (1U << cpu)) {
                    task_numa_node = numa_get_cpu_node(&pool->numa_topology, cpu);
                    break;
                }
            }
            
            if (task_numa_node >= 0 && worker->numa_node != task_numa_node) {
                load_score += 10000;  // Penalty for different NUMA node
            }
        }
        
        if (load_score < best_load_score) {
            best_load_score = load_score;
            best_worker_id = i;
        }
    }
    
    // Submit task to the selected worker
    worker_thread_t* target_worker = &pool->workers[best_worker_id];
    
    if (work_stealing_deque_push_bottom(target_worker->deque, task) == 0) {
        // Update load score
        pool->worker_load_scores[best_worker_id] += 100;
        
        // Wake up the worker if it's idle
        pthread_mutex_lock(&target_worker->worker_mutex);
        if (target_worker->state == WORKER_STATE_IDLE) {
            pthread_cond_signal(&target_worker->work_available);
        }
        pthread_mutex_unlock(&target_worker->worker_mutex);
        
        return 0;
    }
    
    return -1;
}

int work_stealing_pool_submit_task(work_stealing_pool_t* pool, work_task_t* task) {
    if (!pool || !task || pool->is_shutdown || pool->is_paused) {
        return -1;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    
    // Check back-pressure
    uint64_t pending_tasks = work_stealing_pool_get_pending_tasks(pool);
    if (pending_tasks >= pool->backpressure.max_pending_tasks) {
        pool->stats.total_tasks_rejected++;
        pthread_mutex_unlock(&pool->global_mutex);
        return -1;
    }
    
    // Apply adaptive rejection if enabled
    if (pool->backpressure.adaptive_rejection && pending_tasks >= pool->backpressure.high_water_mark) {
        double load_factor = (double)pending_tasks / pool->backpressure.max_pending_tasks;
        double rejection_prob = (load_factor - 0.8) / 0.2;  // Linear increase from 80% to 100%
        
        if (rejection_prob > 0.0) {
            double random_val = (double)rand() / RAND_MAX;
            if (random_val < rejection_prob) {
                pool->stats.total_tasks_rejected++;
                pthread_mutex_unlock(&pool->global_mutex);
                return -1;
            }
        }
    }
    
    pool->stats.total_tasks_submitted++;
    
    // Try to distribute directly to a worker
    if (distribute_task_to_worker(pool, task) == 0) {
        pthread_mutex_unlock(&pool->global_mutex);
        return 0;
    }
    
    // Fall back to global queue
    if (work_stealing_deque_push_bottom(pool->global_queue, task) == 0) {
        pthread_mutex_unlock(&pool->global_mutex);
        return 0;
    }
    
    // All queues are full
    pool->stats.total_tasks_rejected++;
    pthread_mutex_unlock(&pool->global_mutex);
    return -1;
}

// ============================================================================
// POOL MANAGEMENT AND OPTIMIZATION
// ============================================================================

static void update_worker_load_scores(work_stealing_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        worker_thread_t* worker = &pool->workers[i];
        
        // Decay load score over time
        pool->worker_load_scores[i] = pool->worker_load_scores[i] * 0.95;
        
        // Add current queue size
        pool->worker_load_scores[i] += work_stealing_deque_size(worker->deque) * 50;
        
        // Add penalty for high error rate (if tracked)
        // This would require additional error tracking in the worker
    }
}

int work_stealing_pool_adaptive_resize(work_stealing_pool_t* pool) {
    if (!pool || pool->is_shutdown) {
        return -1;
    }
    
    uint64_t current_time = get_timestamp_ns();
    
    // Check cooldown period
    if (current_time - pool->last_resize_time < pool->config.resize_cooldown_ns) {
        return 0;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    
    // Calculate current load metrics
    uint64_t total_pending = work_stealing_pool_get_pending_tasks(pool);
    double load_factor = (double)total_pending / pool->current_thread_count;
    
    // Update CPU utilization
    cpu_monitor_update(&pool->cpu_monitor);
    double avg_cpu_usage = 0.0;
    for (int i = 0; i < pool->cpu_monitor.cpu_count; i++) {
        avg_cpu_usage += pool->cpu_monitor.cpu_usage[i];
    }
    avg_cpu_usage /= pool->cpu_monitor.cpu_count;
    
    bool should_expand = false;
    bool should_shrink = false;
    
    // Decide whether to expand
    if (pool->current_thread_count < pool->config.max_threads) {
        // High load factor or high CPU usage suggests need for more threads
        if (load_factor > pool->config.load_factor_expand * 10 ||  // 10 tasks per thread
            (avg_cpu_usage > 80.0 && total_pending > pool->current_thread_count * 2)) {
            should_expand = true;
        }
    }
    
    // Decide whether to shrink
    if (pool->current_thread_count > pool->config.min_threads && !should_expand) {
        // Low load factor and low CPU usage suggests we can reduce threads
        if (load_factor < pool->config.load_factor_shrink * 10 &&
            avg_cpu_usage < 30.0 && total_pending < pool->current_thread_count) {
            should_shrink = true;
        }
    }
    
    if (should_expand) {
        size_t new_thread_count = pool->current_thread_count + 1;
        if (new_thread_count <= pool->config.max_threads) {
            // Add new worker - simplified version
            pool->target_thread_count = new_thread_count;
            pool->stats.pool_expansions++;
            pool->last_resize_time = current_time;
        }
    } else if (should_shrink) {
        size_t new_thread_count = pool->current_thread_count - 1;
        if (new_thread_count >= pool->config.min_threads) {
            // Signal least active worker to terminate
            pool->target_thread_count = new_thread_count;
            pool->stats.pool_contractions++;
            pool->last_resize_time = current_time;
        }
    }
    
    pool->stats.current_load_factor = load_factor;
    pool->stats.current_thread_count = pool->current_thread_count;
    
    if (pool->current_thread_count > pool->stats.peak_thread_count) {
        pool->stats.peak_thread_count = pool->current_thread_count;
    }
    
    pthread_mutex_unlock(&pool->global_mutex);
    return 0;
}

int work_stealing_pool_load_balance(work_stealing_pool_t* pool) {
    if (!pool || !pool->load_balancing_enabled) {
        return 0;
    }
    
    uint64_t current_time = get_timestamp_ns();
    if (current_time - pool->last_load_balance < WORK_STEALING_LOAD_BALANCE_INTERVAL) {
        return 0;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    
    update_worker_load_scores(pool);
    
    // Distribute tasks from global queue if any
    while (!work_stealing_deque_is_empty(pool->global_queue)) {
        work_task_t* task = work_stealing_deque_pop_bottom(pool->global_queue);
        if (task) {
            if (distribute_task_to_worker(pool, task) != 0) {
                // Put it back if distribution failed
                work_stealing_deque_push_bottom(pool->global_queue, task);
                break;
            }
        }
    }
    
    pool->last_load_balance = current_time;
    pthread_mutex_unlock(&pool->global_mutex);
    
    return 0;
}

// ============================================================================
// MONITORING AND STATISTICS
// ============================================================================

size_t work_stealing_pool_get_pending_tasks(work_stealing_pool_t* pool) {
    if (!pool) {
        return 0;
    }
    
    size_t total = work_stealing_deque_size(pool->global_queue);
    
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        total += work_stealing_deque_size(pool->workers[i].deque);
    }
    
    return total;
}

double work_stealing_pool_get_load_factor(work_stealing_pool_t* pool) {
    if (!pool || pool->current_thread_count == 0) {
        return 0.0;
    }
    
    size_t pending_tasks = work_stealing_pool_get_pending_tasks(pool);
    return (double)pending_tasks / pool->current_thread_count;
}

void work_stealing_pool_get_stats(work_stealing_pool_t* pool, pool_statistics_t* stats) {
    if (!pool || !stats) {
        return;
    }
    
    pthread_mutex_lock(&pool->global_mutex);
    *stats = pool->stats;
    stats->last_stats_update = get_timestamp_ns();
    pthread_mutex_unlock(&pool->global_mutex);
}

void work_stealing_pool_print_stats(work_stealing_pool_t* pool) {
    if (!pool) {
        return;
    }
    
    pool_statistics_t stats;
    work_stealing_pool_get_stats(pool, &stats);
    
    printf("\n=== Work-Stealing Thread Pool Statistics ===\n");
    printf("Pool Configuration:\n");
    printf("  Min threads: %zu\n", pool->config.min_threads);
    printf("  Max threads: %zu\n", pool->config.max_threads);
    printf("  Current threads: %zu\n", stats.current_thread_count);
    printf("  Peak threads: %zu\n", stats.peak_thread_count);
    printf("  Target threads: %zu\n", pool->target_thread_count);
    
    printf("\nTask Statistics:\n");
    printf("  Total submitted: %lu\n", stats.total_tasks_submitted);
    printf("  Total completed: %lu\n", stats.total_tasks_completed);
    printf("  Total rejected: %lu\n", stats.total_tasks_rejected);
    printf("  Pending tasks: %zu\n", work_stealing_pool_get_pending_tasks(pool));
    
    printf("\nWork Stealing Statistics:\n");
    printf("  Total steal operations: %lu\n", stats.total_steal_operations);
    printf("  Successful steals: %lu\n", stats.successful_steals);
    if (stats.total_steal_operations > 0) {
        printf("  Steal success rate: %.2f%%\n", 
               (double)stats.successful_steals / stats.total_steal_operations * 100.0);
    }
    
    printf("\nPerformance Metrics:\n");
    printf("  Current load factor: %.2f\n", stats.current_load_factor);
    printf("  Average task wait time: %.3f ms\n", stats.avg_task_wait_time);
    printf("  Average task execution time: %.3f ms\n", stats.avg_task_execution_time);
    
    printf("\nPool Management:\n");
    printf("  Pool expansions: %lu\n", stats.pool_expansions);
    printf("  Pool contractions: %lu\n", stats.pool_contractions);
    
    printf("\nWorker Statistics:\n");
    uint64_t total_tasks_executed = 0;
    uint64_t total_tasks_stolen_by_workers = 0;
    uint64_t total_tasks_stolen_from_workers = 0;
    
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        worker_thread_t* worker = &pool->workers[i];
        total_tasks_executed += worker->tasks_executed;
        total_tasks_stolen_by_workers += worker->tasks_stolen_by_me;
        total_tasks_stolen_from_workers += worker->tasks_stolen_from_me;
        
        printf("  Worker %zu: %lu tasks, %.2f ms avg, NUMA node %d, CPU %d\n",
               i, worker->tasks_executed, worker->avg_task_duration,
               worker->numa_node, worker->preferred_cpu);
    }
    
    printf("  Total tasks executed by workers: %lu\n", total_tasks_executed);
    printf("  Total tasks stolen by workers: %lu\n", total_tasks_stolen_by_workers);
    printf("  Total tasks stolen from workers: %lu\n", total_tasks_stolen_from_workers);
    
    printf("\nDeque Statistics:\n");
    for (size_t i = 0; i < pool->current_thread_count; i++) {
        work_stealing_deque_t* deque = pool->workers[i].deque;
        printf("  Worker %zu deque: pushed=%lu, popped=%lu, stolen=%lu, size=%zu\n",
               i, deque->tasks_pushed, deque->tasks_popped, deque->tasks_stolen,
               work_stealing_deque_size(deque));
    }
    
    printf("  Global queue size: %zu\n", work_stealing_deque_size(pool->global_queue));
    printf("==============================================\n");
}

int set_thread_affinity(pthread_t thread, const cpu_set_t* cpuset) {
    return pthread_setaffinity_np(thread, sizeof(cpu_set_t), cpuset);
}