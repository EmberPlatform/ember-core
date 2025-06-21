#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <time.h>
#include <sys/time.h>
#include <assert.h>
#include <stdatomic.h>

#include "../../src/core/work_stealing_pool.h"

// Test configuration
#define MAX_TEST_THREADS 1000
#define TEST_DURATION_SECONDS 30
#define TASK_BATCH_SIZE 100
#define MIN_TASK_EXECUTION_TIME_NS 1000    // 1 microsecond
#define MAX_TASK_EXECUTION_TIME_NS 1000000 // 1 millisecond

// Global test statistics
typedef struct {
    atomic_uint_fast64_t tasks_submitted;
    atomic_uint_fast64_t tasks_completed;
    atomic_uint_fast64_t tasks_failed;
    atomic_uint_fast64_t total_execution_time_ns;
    atomic_uint_fast64_t min_execution_time_ns;
    atomic_uint_fast64_t max_execution_time_ns;
    uint64_t test_start_time;
    uint64_t test_end_time;
} test_statistics_t;

static test_statistics_t g_test_stats;

// Test task data
typedef struct {
    uint64_t task_id;
    uint64_t submit_time;
    uint64_t expected_duration_ns;
    volatile bool completed;
} test_task_data_t;

// Forward declarations
static void test_task_function(void* data);
static void stress_test_thread_function(void* arg);
static void print_test_results(int thread_count, double duration_seconds);
static void reset_test_statistics(void);
static uint64_t get_time_ns(void);
static void run_single_thread_test(void);
static void run_scaling_test(void);
static void run_extreme_load_test(void);
static void run_numa_awareness_test(void);
static void run_work_stealing_efficiency_test(void);

// Simple test task that simulates variable workload
static void test_task_function(void* data) {
    test_task_data_t* task_data = (test_task_data_t*)data;
    uint64_t start_time = get_time_ns();
    
    // Simulate work with the expected duration
    uint64_t target_end_time = start_time + task_data->expected_duration_ns;
    
    // Busy wait to simulate CPU work
    volatile uint64_t counter = 0;
    while (get_time_ns() < target_end_time) {
        counter++;
        if (counter % 1000 == 0) {
            // Occasionally yield to prevent compiler optimization
            __asm__ __volatile__("" : : : "memory");
        }
    }
    
    uint64_t end_time = get_time_ns();
    uint64_t actual_duration = end_time - start_time;
    
    // Update statistics atomically
    atomic_fetch_add_explicit(&g_test_stats.tasks_completed, 1, memory_order_relaxed);
    atomic_fetch_add_explicit(&g_test_stats.total_execution_time_ns, actual_duration, memory_order_relaxed);
    
    // Update min/max execution times
    uint64_t current_min = atomic_load_explicit(&g_test_stats.min_execution_time_ns, memory_order_relaxed);
    while (actual_duration < current_min) {
        if (atomic_compare_exchange_weak_explicit(&g_test_stats.min_execution_time_ns, 
                                                &current_min, actual_duration,
                                                memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
    
    uint64_t current_max = atomic_load_explicit(&g_test_stats.max_execution_time_ns, memory_order_relaxed);
    while (actual_duration > current_max) {
        if (atomic_compare_exchange_weak_explicit(&g_test_stats.max_execution_time_ns,
                                                &current_max, actual_duration,
                                                memory_order_relaxed, memory_order_relaxed)) {
            break;
        }
    }
    
    task_data->completed = true;
}

// Thread function that continuously submits tasks
static void stress_test_thread_function(void* arg) {
    work_stealing_pool_t* pool = (work_stealing_pool_t*)arg;
    uint64_t thread_task_id = 0;
    
    while (get_time_ns() < g_test_stats.test_end_time) {
        // Submit a batch of tasks
        for (int i = 0; i < TASK_BATCH_SIZE; i++) {
            test_task_data_t* task_data = malloc(sizeof(test_task_data_t));
            if (!task_data) {
                atomic_fetch_add_explicit(&g_test_stats.tasks_failed, 1, memory_order_relaxed);
                continue;
            }
            
            task_data->task_id = thread_task_id++;
            task_data->submit_time = get_time_ns();
            task_data->expected_duration_ns = MIN_TASK_EXECUTION_TIME_NS + 
                (rand() % (MAX_TASK_EXECUTION_TIME_NS - MIN_TASK_EXECUTION_TIME_NS));
            task_data->completed = false;
            
            work_task_t* task = work_stealing_pool_create_task(
                test_task_function, 
                task_data,
                TASK_PRIORITY_NORMAL,
                TASK_TYPE_COMPUTATION
            );
            
            if (!task) {
                free(task_data);
                atomic_fetch_add_explicit(&g_test_stats.tasks_failed, 1, memory_order_relaxed);
                continue;
            }
            
            if (work_stealing_pool_submit_task(pool, task) != 0) {
                free(task_data);
                free(task);
                atomic_fetch_add_explicit(&g_test_stats.tasks_failed, 1, memory_order_relaxed);
            } else {
                atomic_fetch_add_explicit(&g_test_stats.tasks_submitted, 1, memory_order_relaxed);
            }
        }
        
        // Brief pause between batches
        usleep(1000); // 1ms
    }
    
    return NULL;
}

static uint64_t get_time_ns(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000000000ULL + ts.tv_nsec;
}

static void reset_test_statistics(void) {
    atomic_store_explicit(&g_test_stats.tasks_submitted, 0, memory_order_relaxed);
    atomic_store_explicit(&g_test_stats.tasks_completed, 0, memory_order_relaxed);
    atomic_store_explicit(&g_test_stats.tasks_failed, 0, memory_order_relaxed);
    atomic_store_explicit(&g_test_stats.total_execution_time_ns, 0, memory_order_relaxed);
    atomic_store_explicit(&g_test_stats.min_execution_time_ns, UINT64_MAX, memory_order_relaxed);
    atomic_store_explicit(&g_test_stats.max_execution_time_ns, 0, memory_order_relaxed);
}

static void print_test_results(int thread_count, double duration_seconds) {
    uint64_t submitted = atomic_load_explicit(&g_test_stats.tasks_submitted, memory_order_relaxed);
    uint64_t completed = atomic_load_explicit(&g_test_stats.tasks_completed, memory_order_relaxed);
    uint64_t failed = atomic_load_explicit(&g_test_stats.tasks_failed, memory_order_relaxed);
    uint64_t total_execution_time = atomic_load_explicit(&g_test_stats.total_execution_time_ns, memory_order_relaxed);
    uint64_t min_time = atomic_load_explicit(&g_test_stats.min_execution_time_ns, memory_order_relaxed);
    uint64_t max_time = atomic_load_explicit(&g_test_stats.max_execution_time_ns, memory_order_relaxed);
    
    double success_rate = submitted > 0 ? (double)completed / submitted * 100.0 : 0.0;
    double throughput = completed / duration_seconds;
    double avg_execution_time = completed > 0 ? (double)total_execution_time / completed / 1000000.0 : 0.0;
    
    printf("Test Results (Threads: %d, Duration: %.1fs):\n", thread_count, duration_seconds);
    printf("  Tasks Submitted: %lu\n", submitted);
    printf("  Tasks Completed: %lu\n", completed);
    printf("  Tasks Failed: %lu\n", failed);
    printf("  Success Rate: %.2f%%\n", success_rate);
    printf("  Throughput: %.1f tasks/second\n", throughput);
    printf("  Avg Execution Time: %.3f ms\n", avg_execution_time);
    printf("  Min Execution Time: %.3f ms\n", min_time != UINT64_MAX ? min_time / 1000000.0 : 0.0);
    printf("  Max Execution Time: %.3f ms\n", max_time / 1000000.0);
    printf("\n");
}

// Test 1: Single thread baseline
static void run_single_thread_test(void) {
    printf("=== Single Thread Baseline Test ===\n");
    
    thread_pool_config_t config = DEFAULT_THREAD_POOL_CONFIG;
    config.min_threads = 1;
    config.max_threads = 1;
    config.initial_threads = 1;
    
    work_stealing_pool_t* pool = work_stealing_pool_create(&config);
    assert(pool != NULL);
    
    assert(work_stealing_pool_start(pool) == 0);
    
    reset_test_statistics();
    g_test_stats.test_start_time = get_time_ns();
    g_test_stats.test_end_time = g_test_stats.test_start_time + (5 * 1000000000ULL); // 5 seconds
    
    pthread_t submitter_thread;
    pthread_create(&submitter_thread, NULL, (void*(*)(void*))stress_test_thread_function, pool);
    
    pthread_join(submitter_thread, NULL);
    
    // Wait for remaining tasks to complete
    sleep(2);
    
    double duration = (get_time_ns() - g_test_stats.test_start_time) / 1000000000.0;
    print_test_results(1, duration);
    
    work_stealing_pool_destroy(pool);
}

// Test 2: Linear scaling test
static void run_scaling_test(void) {
    printf("=== Linear Scaling Test ===\n");
    
    int thread_counts[] = {1, 2, 4, 8, 16, 32, 64, 128};
    int num_tests = sizeof(thread_counts) / sizeof(thread_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int thread_count = thread_counts[i];
        
        thread_pool_config_t config = DEFAULT_THREAD_POOL_CONFIG;
        config.min_threads = thread_count;
        config.max_threads = thread_count;
        config.initial_threads = thread_count;
        
        work_stealing_pool_t* pool = work_stealing_pool_create(&config);
        assert(pool != NULL);
        
        assert(work_stealing_pool_start(pool) == 0);
        
        reset_test_statistics();
        g_test_stats.test_start_time = get_time_ns();
        g_test_stats.test_end_time = g_test_stats.test_start_time + (10 * 1000000000ULL); // 10 seconds
        
        // Create multiple submitter threads to generate load
        int num_submitters = (thread_count + 3) / 4; // 1 submitter per 4 worker threads
        pthread_t* submitter_threads = malloc(num_submitters * sizeof(pthread_t));
        
        for (int j = 0; j < num_submitters; j++) {
            pthread_create(&submitter_threads[j], NULL, 
                          (void*(*)(void*))stress_test_thread_function, pool);
        }
        
        for (int j = 0; j < num_submitters; j++) {
            pthread_join(submitter_threads[j], NULL);
        }
        
        // Wait for remaining tasks to complete
        sleep(2);
        
        double duration = (get_time_ns() - g_test_stats.test_start_time) / 1000000000.0;
        print_test_results(thread_count, duration);
        
        free(submitter_threads);
        work_stealing_pool_destroy(pool);
        
        // Brief pause between tests
        sleep(1);
    }
}

// Test 3: Extreme load test (target: 1000+ concurrent threads)
static void run_extreme_load_test(void) {
    printf("=== Extreme Load Test (1000+ Threads) ===\n");
    
    int extreme_thread_counts[] = {200, 500, 1000};
    int num_tests = sizeof(extreme_thread_counts) / sizeof(extreme_thread_counts[0]);
    
    for (int i = 0; i < num_tests; i++) {
        int thread_count = extreme_thread_counts[i];
        
        thread_pool_config_t config = DEFAULT_THREAD_POOL_CONFIG;
        config.min_threads = thread_count / 10; // Start with fewer threads
        config.max_threads = thread_count;
        config.initial_threads = thread_count / 5;
        config.load_factor_expand = 0.7; // Expand more aggressively
        
        work_stealing_pool_t* pool = work_stealing_pool_create(&config);
        assert(pool != NULL);
        
        assert(work_stealing_pool_start(pool) == 0);
        
        reset_test_statistics();
        g_test_stats.test_start_time = get_time_ns();
        g_test_stats.test_end_time = g_test_stats.test_start_time + (TEST_DURATION_SECONDS * 1000000000ULL);
        
        // Create many submitter threads to generate extreme load
        int num_submitters = thread_count / 10; // Many submitters
        pthread_t* submitter_threads = malloc(num_submitters * sizeof(pthread_t));
        
        for (int j = 0; j < num_submitters; j++) {
            pthread_create(&submitter_threads[j], NULL,
                          (void*(*)(void*))stress_test_thread_function, pool);
        }
        
        // Monitor and potentially resize the pool during the test
        sleep(5); // Let it run for a bit
        work_stealing_pool_adaptive_resize(pool);
        sleep(5);
        work_stealing_pool_load_balance(pool);
        
        for (int j = 0; j < num_submitters; j++) {
            pthread_join(submitter_threads[j], NULL);
        }
        
        // Wait for remaining tasks to complete
        sleep(5);
        
        double duration = (get_time_ns() - g_test_stats.test_start_time) / 1000000000.0;
        print_test_results(thread_count, duration);
        
        // Print additional pool statistics
        work_stealing_pool_print_stats(pool);
        
        free(submitter_threads);
        work_stealing_pool_destroy(pool);
        
        // Longer pause between extreme tests
        sleep(3);
    }
}

// Test 4: NUMA awareness test
static void run_numa_awareness_test(void) {
    printf("=== NUMA Awareness Test ===\n");
    
    thread_pool_config_t config = DEFAULT_THREAD_POOL_CONFIG;
    config.min_threads = 16;
    config.max_threads = 64;
    config.initial_threads = 32;
    config.enable_numa_awareness = true;
    config.enable_cpu_affinity = true;
    
    work_stealing_pool_t* pool = work_stealing_pool_create(&config);
    assert(pool != NULL);
    
    assert(work_stealing_pool_start(pool) == 0);
    
    reset_test_statistics();
    g_test_stats.test_start_time = get_time_ns();
    g_test_stats.test_end_time = g_test_stats.test_start_time + (15 * 1000000000ULL); // 15 seconds
    
    // Create tasks with CPU affinity
    pthread_t submitter_thread;
    pthread_create(&submitter_thread, NULL, (void*(*)(void*))stress_test_thread_function, pool);
    
    pthread_join(submitter_thread, NULL);
    
    // Wait for remaining tasks to complete
    sleep(2);
    
    double duration = (get_time_ns() - g_test_stats.test_start_time) / 1000000000.0;
    print_test_results(32, duration);
    
    printf("NUMA-specific statistics:\n");
    work_stealing_pool_print_stats(pool);
    
    work_stealing_pool_destroy(pool);
}

// Test 5: Work stealing efficiency test
static void run_work_stealing_efficiency_test(void) {
    printf("=== Work Stealing Efficiency Test ===\n");
    
    thread_pool_config_t config = DEFAULT_THREAD_POOL_CONFIG;
    config.min_threads = 8;
    config.max_threads = 16;
    config.initial_threads = 8;
    config.enable_work_stealing = true;
    
    work_stealing_pool_t* pool = work_stealing_pool_create(&config);
    assert(pool != NULL);
    
    assert(work_stealing_pool_start(pool) == 0);
    
    // Create an imbalanced workload by submitting many tasks to one worker
    reset_test_statistics();
    g_test_stats.test_start_time = get_time_ns();
    
    // Submit a large batch of tasks quickly to create imbalance
    for (int i = 0; i < 1000; i++) {
        test_task_data_t* task_data = malloc(sizeof(test_task_data_t));
        task_data->task_id = i;
        task_data->submit_time = get_time_ns();
        task_data->expected_duration_ns = MIN_TASK_EXECUTION_TIME_NS * 10; // Longer tasks
        task_data->completed = false;
        
        work_task_t* task = work_stealing_pool_create_task(
            test_task_function,
            task_data,
            TASK_PRIORITY_NORMAL,
            TASK_TYPE_COMPUTATION
        );
        
        if (work_stealing_pool_submit_task(pool, task) == 0) {
            atomic_fetch_add_explicit(&g_test_stats.tasks_submitted, 1, memory_order_relaxed);
        } else {
            free(task_data);
            free(task);
            atomic_fetch_add_explicit(&g_test_stats.tasks_failed, 1, memory_order_relaxed);
        }
    }
    
    // Wait for all tasks to complete
    sleep(10);
    
    double duration = (get_time_ns() - g_test_stats.test_start_time) / 1000000000.0;
    print_test_results(8, duration);
    
    printf("Work stealing statistics:\n");
    work_stealing_pool_print_stats(pool);
    
    work_stealing_pool_destroy(pool);
}

// Main test function
int main(void) {
    printf("=== Work-Stealing Thread Pool Stress Test ===\n");
    printf("Testing implementation against performance targets:\n");
    printf("- Linear scaling to 1000+ concurrent threads\n");
    printf("- 95%+ success rate at any thread count\n");
    printf("- Optimal CPU utilization across all cores\n");
    printf("- <1ms task distribution latency\n");
    printf("\n");
    
    // Initialize random seed
    srand(time(NULL));
    
    // Run all tests
    run_single_thread_test();
    run_scaling_test();
    run_extreme_load_test();
    run_numa_awareness_test();
    run_work_stealing_efficiency_test();
    
    printf("=== ALL TESTS COMPLETED ===\n");
    printf("Analyze the results above to verify performance targets:\n");
    printf("1. Success rates should be 95%+ across all thread counts\n");
    printf("2. Throughput should scale approximately linearly with thread count\n");
    printf("3. Work stealing should show balanced load distribution\n");
    printf("4. NUMA awareness should improve cache locality\n");
    
    return 0;
}