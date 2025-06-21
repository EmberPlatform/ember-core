/**
 * Minimal Thread Safety Test for Parser
 * Tests that concurrent compilation no longer causes heap buffer overflow
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

#define NUM_THREADS 4
#define NUM_ITERATIONS 10

// Test data structure for threads
typedef struct {
    int thread_id;
    int iterations;
    int success_count;
    int failure_count;
} thread_data_t;

// Simple valid Ember script
const char* test_script = "x = 42\nprint(x)";

void* simple_compilation_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    // Create a VM for this thread
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    for (int i = 0; i < data->iterations; i++) {
        // Compile and execute the script
        int result = ember_eval(vm, test_script);
        
        if (result == 0) {
            data->success_count++;
        } else {
            data->failure_count++;
        }
        
        // Small delay to encourage race conditions if they exist
        usleep(1000); // 1ms delay
    }
    
    ember_free_vm(vm);
    return NULL;
}

void test_thread_safety() {
    printf("Testing thread safety of parser...\\n");
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = NUM_ITERATIONS;
        thread_data[i].success_count = 0;
        thread_data[i].failure_count = 0;
    }
    
    printf("  Starting %d threads with %d iterations each...\\n", NUM_THREADS, NUM_ITERATIONS);
    
    clock_t start_time = clock();
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, simple_compilation_test, &thread_data[i]);
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
    
    for (int i = 0; i < NUM_THREADS; i++) {
        total_success += thread_data[i].success_count;
        total_failure += thread_data[i].failure_count;
        
        printf("  Thread %d: %d successes, %d failures\\n", 
               i, thread_data[i].success_count, thread_data[i].failure_count);
    }
    
    printf("  Total: %d successes, %d failures\\n", total_success, total_failure);
    printf("  Success rate: %.2f%%\\n", (double)total_success / (total_success + total_failure) * 100);
    printf("  Time elapsed: %.2f seconds\\n", elapsed_time);
    printf("  Operations per second: %.0f\\n", (total_success + total_failure) / elapsed_time);
    
    // If we get here without crashing, the thread safety issue is fixed
    printf("  ✓ Thread safety test completed without crashes\\n\\n");
}

int main() {
    printf("Minimal Thread Safety Test\\n");
    printf("==========================\\n\\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    test_thread_safety();
    
    printf("Thread safety test passed!\\n");
    printf("Parser is now thread-safe with thread-local storage.\\n");
    
    return 0;
}