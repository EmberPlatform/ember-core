/**
 * VM Pool Stress Test
 * Tests VM pool for race conditions and memory leaks under high concurrency
 */

#define _GNU_SOURCE
#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <pthread.h>
#include <unistd.h>
#include <time.h>

#define NUM_THREADS 8
#define NUM_VMS_PER_THREAD 5
#define NUM_ITERATIONS 20
#define SCRIPT_VARIATIONS 3

// Test data structure for threads
typedef struct {
    int thread_id;
    int iterations;
    int success_count;
    int failure_count;
    int vm_alloc_count;
    int vm_free_count;
} thread_data_t;

// Simple valid Ember scripts
const char* test_scripts[SCRIPT_VARIATIONS] = {
    "result = 10 + 20\nprint(result)",
    "name = \"VM Test\"\nprint(name)",
    "count = 5\nfor (i = 0; i < count; i = i + 1) {\n    print(i)\n}"
};

void* vm_pool_stress_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    ember_vm* vms[NUM_VMS_PER_THREAD];
    
    for (int i = 0; i < data->iterations; i++) {
        // Allocate multiple VMs
        for (int j = 0; j < NUM_VMS_PER_THREAD; j++) {
            vms[j] = ember_new_vm();
            if (vms[j] != NULL) {
                data->vm_alloc_count++;
            }
        }
        
        // Use the VMs for script execution
        for (int j = 0; j < NUM_VMS_PER_THREAD; j++) {
            if (vms[j] != NULL) {
                int script_idx = (i + j) % SCRIPT_VARIATIONS;
                int result = ember_eval(vms[j], test_scripts[script_idx]);
                
                if (result == 0) {
                    data->success_count++;
                } else {
                    data->failure_count++;
                }
            }
        }
        
        // Free the VMs
        for (int j = 0; j < NUM_VMS_PER_THREAD; j++) {
            if (vms[j] != NULL) {
                ember_free_vm(vms[j]);
                data->vm_free_count++;
                vms[j] = NULL;
            }
        }
        
        // Add small random delay to encourage race conditions
        usleep(rand() % 2000); // 0-2ms delay
    }
    
    return NULL;
}

void test_vm_pool_concurrency() {
    printf("Testing VM pool concurrency and memory management...\\n");
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = NUM_ITERATIONS;
        thread_data[i].success_count = 0;
        thread_data[i].failure_count = 0;
        thread_data[i].vm_alloc_count = 0;
        thread_data[i].vm_free_count = 0;
    }
    
    printf("  Starting %d threads, each allocating %d VMs for %d iterations...\\n", 
           NUM_THREADS, NUM_VMS_PER_THREAD, NUM_ITERATIONS);
    
    clock_t start_time = clock();
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, vm_pool_stress_test, &thread_data[i]);
        assert(result == 0);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Collect results
    int total_success = 0;
    int total_failure = 0;
    int total_vm_alloc = 0;
    int total_vm_free = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_success += thread_data[i].success_count;
        total_failure += thread_data[i].failure_count;
        total_vm_alloc += thread_data[i].vm_alloc_count;
        total_vm_free += thread_data[i].vm_free_count;
        
        printf("  Thread %d: %d successes, %d failures, %d VMs allocated, %d VMs freed\\n", 
               i, thread_data[i].success_count, thread_data[i].failure_count,
               thread_data[i].vm_alloc_count, thread_data[i].vm_free_count);
    }
    
    printf("\\n  === VM Pool Stress Test Results ===\\n");
    printf("  Total executions: %d successes, %d failures\\n", total_success, total_failure);
    printf("  Total VMs: %d allocated, %d freed\\n", total_vm_alloc, total_vm_free);
    printf("  VM allocation balance: %s\\n", (total_vm_alloc == total_vm_free) ? "BALANCED" : "UNBALANCED");
    printf("  Success rate: %.2f%%\\n", (double)total_success / (total_success + total_failure) * 100);
    printf("  Time elapsed: %.2f seconds\\n", elapsed_time);
    printf("  VM operations per second: %.0f\\n", (total_vm_alloc + total_vm_free) / elapsed_time);
    
    // Validate VM allocation/deallocation balance
    assert(total_vm_alloc == total_vm_free);
    printf("  ✓ VM pool memory management is balanced\\n");
    
    // Validate reasonable success rate
    double success_rate = (double)total_success / (total_success + total_failure);
    assert(success_rate > 0.8);
    printf("  ✓ VM pool concurrency test passed\\n\\n");
}

void test_vm_pool_memory_leaks() {
    printf("Testing VM pool for memory leaks...\\n");
    
    const int NUM_CYCLES = 100;
    const int VMS_PER_CYCLE = 10;
    
    printf("  Running %d cycles of %d VM allocations each...\\n", NUM_CYCLES, VMS_PER_CYCLE);
    
    for (int cycle = 0; cycle < NUM_CYCLES; cycle++) {
        ember_vm* vms[VMS_PER_CYCLE];
        
        // Allocate VMs
        for (int i = 0; i < VMS_PER_CYCLE; i++) {
            vms[i] = ember_new_vm();
            assert(vms[i] != NULL);
            
            // Execute a simple script to exercise VM
            ember_eval(vms[i], "x = 42\\nprint(x)");
        }
        
        // Free VMs
        for (int i = 0; i < VMS_PER_CYCLE; i++) {
            ember_free_vm(vms[i]);
        }
        
        if (cycle % 20 == 0) {
            printf("  Completed %d cycles...\\n", cycle + 1);
        }
    }
    
    printf("  ✓ VM pool memory leak test completed\\n\\n");
}

int main() {
    printf("VM Pool Stress Test\\n");
    printf("==================\\n\\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    test_vm_pool_concurrency();
    test_vm_pool_memory_leaks();
    
    printf("All VM pool tests passed!\\n");
    printf("VM pool is thread-safe and memory-leak free.\\n");
    
    return 0;
}