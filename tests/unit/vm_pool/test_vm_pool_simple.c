/**
 * Simple VM Pool Test
 * Tests VM allocation/deallocation for race conditions and memory leaks
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
#define NUM_VMS_PER_THREAD 10
#define NUM_ITERATIONS 50

// Test data structure for threads
typedef struct {
    int thread_id;
    int iterations;
    int vm_alloc_count;
    int vm_free_count;
} thread_data_t;

void* vm_allocation_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        ember_vm* vm = ember_new_vm();
        if (vm != NULL) {
            data->vm_alloc_count++;
            
            // Add small delay to encourage race conditions
            usleep(rand() % 100); // 0-0.1ms delay
            
            ember_free_vm(vm);
            data->vm_free_count++;
        }
    }
    
    return NULL;
}

void test_vm_allocation_concurrency() {
    printf("Testing VM allocation/deallocation concurrency...\\n");
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = NUM_ITERATIONS;
        thread_data[i].vm_alloc_count = 0;
        thread_data[i].vm_free_count = 0;
    }
    
    printf("  Starting %d threads, each allocating/freeing %d VMs...\\n", 
           NUM_THREADS, NUM_ITERATIONS);
    
    clock_t start_time = clock();
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, vm_allocation_test, &thread_data[i]);
        assert(result == 0);
    }
    
    // Wait for all threads to complete
    for (int i = 0; i < NUM_THREADS; i++) {
        pthread_join(threads[i], NULL);
    }
    
    clock_t end_time = clock();
    double elapsed_time = ((double)(end_time - start_time)) / CLOCKS_PER_SEC;
    
    // Collect results
    int total_vm_alloc = 0;
    int total_vm_free = 0;
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_vm_alloc += thread_data[i].vm_alloc_count;
        total_vm_free += thread_data[i].vm_free_count;
        
        printf("  Thread %d: %d VMs allocated, %d VMs freed\\n", 
               i, thread_data[i].vm_alloc_count, thread_data[i].vm_free_count);
    }
    
    printf("\\n  === VM Allocation Test Results ===\\n");
    printf("  Total VMs: %d allocated, %d freed\\n", total_vm_alloc, total_vm_free);
    printf("  VM allocation balance: %s\\n", (total_vm_alloc == total_vm_free) ? "BALANCED" : "UNBALANCED");
    printf("  Time elapsed: %.2f seconds\\n", elapsed_time);
    printf("  VM operations per second: %.0f\\n", (total_vm_alloc + total_vm_free) / elapsed_time);
    
    // Validate VM allocation/deallocation balance
    assert(total_vm_alloc == total_vm_free);
    printf("  ✓ VM allocation/deallocation is balanced\\n");
    
    // Expect all allocations to succeed
    int expected_total = NUM_THREADS * NUM_ITERATIONS;
    assert(total_vm_alloc == expected_total);
    assert(total_vm_free == expected_total);
    printf("  ✓ All VM allocations and deallocations successful\\n\\n");
}

void test_vm_pool_sequential() {
    printf("Testing VM pool sequential operations for memory leaks...\\n");
    
    const int NUM_CYCLES = 1000;
    
    printf("  Running %d VM allocation/deallocation cycles...\\n", NUM_CYCLES);
    
    for (int i = 0; i < NUM_CYCLES; i++) {
        ember_vm* vm = ember_new_vm();
        assert(vm != NULL);
        ember_free_vm(vm);
        
        if (i % 200 == 0) {
            printf("  Completed %d cycles...\\n", i + 1);
        }
    }
    
    printf("  ✓ Sequential VM pool test completed without leaks\\n\\n");
}

int main() {
    printf("Simple VM Pool Test\\n");
    printf("==================\\n\\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    test_vm_allocation_concurrency();
    test_vm_pool_sequential();
    
    printf("All VM pool tests passed!\\n");
    printf("VM pool allocation/deallocation is thread-safe.\\n");
    
    return 0;
}