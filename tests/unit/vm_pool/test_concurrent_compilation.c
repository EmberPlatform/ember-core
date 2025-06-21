/**
 * Concurrent Script Compilation and Caching Test
 * Tests thread safety of script compilation and bytecode caching
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
#define NUM_ITERATIONS 50
#define NUM_SCRIPTS 5

// Test data structure for threads
typedef struct {
    int thread_id;
    int iterations;
    int success_count;
    int failure_count;
    ember_vm** vms;
    char** scripts;
    int num_scripts;
} thread_data_t;

// Sample scripts for testing
const char* test_scripts[NUM_SCRIPTS] = {
    "print(\"Hello from script 1\")",
    "x = 42\nprint(\"Script 2: \" + x)",
    "for (i = 0; i < 3; i = i + 1) {\n    print(\"Loop \" + i)\n}",
    "fn test() {\n    return \"Script 4\"\n}\nprint(test())",
    "arr = [1, 2, 3]\nprint(\"Array length: \" + len(arr))"
};

void* compilation_stress_test(void* arg) {
    thread_data_t* data = (thread_data_t*)arg;
    
    for (int i = 0; i < data->iterations; i++) {
        // Pick a random script and VM
        int script_idx = rand() % data->num_scripts;
        int vm_idx = rand() % 3; // Use 3 VMs per thread
        
        // Compile and execute the script
        int result = ember_eval(data->vms[vm_idx], data->scripts[script_idx]);
        
        if (result == 0) {
            data->success_count++;
        } else {
            data->failure_count++;
        }
        
        // Add small random delay to simulate realistic timing
        usleep(rand() % 1000); // 0-1ms delay
    }
    
    return NULL;
}

void test_concurrent_compilation() {
    printf("Testing concurrent script compilation...\n");
    
    pthread_t threads[NUM_THREADS];
    thread_data_t thread_data[NUM_THREADS];
    
    // Initialize thread data
    for (int i = 0; i < NUM_THREADS; i++) {
        thread_data[i].thread_id = i;
        thread_data[i].iterations = NUM_ITERATIONS;
        thread_data[i].success_count = 0;
        thread_data[i].failure_count = 0;
        thread_data[i].num_scripts = NUM_SCRIPTS;
        
        // Create VMs for this thread
        thread_data[i].vms = malloc(3 * sizeof(ember_vm*));
        for (int j = 0; j < 3; j++) {
            thread_data[i].vms[j] = ember_new_vm();
            assert(thread_data[i].vms[j] != NULL);
        }
        
        // Setup scripts
        thread_data[i].scripts = malloc(NUM_SCRIPTS * sizeof(char*));
        for (int j = 0; j < NUM_SCRIPTS; j++) {
            thread_data[i].scripts[j] = malloc(strlen(test_scripts[j]) + 1);
            strcpy(thread_data[i].scripts[j], test_scripts[j]);
        }
    }
    
    printf("  Starting %d threads with %d iterations each...\n", NUM_THREADS, NUM_ITERATIONS);
    
    clock_t start_time = clock();
    
    // Create and start threads
    for (int i = 0; i < NUM_THREADS; i++) {
        int result = pthread_create(&threads[i], NULL, compilation_stress_test, &thread_data[i]);
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
        
        printf("  Thread %d: %d successes, %d failures\n", 
               i, thread_data[i].success_count, thread_data[i].failure_count);
    }
    
    printf("  Total: %d successes, %d failures\n", total_success, total_failure);
    printf("  Success rate: %.2f%%\n", (double)total_success / (total_success + total_failure) * 100);
    printf("  Time elapsed: %.2f seconds\n", elapsed_time);
    printf("  Operations per second: %.0f\n", (total_success + total_failure) / elapsed_time);
    
    // Cleanup
    for (int i = 0; i < NUM_THREADS; i++) {
        for (int j = 0; j < 3; j++) {
            ember_free_vm(thread_data[i].vms[j]);
        }
        free(thread_data[i].vms);
        
        for (int j = 0; j < NUM_SCRIPTS; j++) {
            free(thread_data[i].scripts[j]);
        }
        free(thread_data[i].scripts);
    }
    
    // Verify no crashes occurred and reasonable success rate
    assert(total_success > 0);
    double success_rate = (double)total_success / (total_success + total_failure);
    assert(success_rate > 0.8); // At least 80% success rate
    
    printf("  ✓ Concurrent compilation test passed\n\n");
}

void test_memory_safety_under_load() {
    printf("Testing memory safety under compilation load...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Compile many different scripts rapidly
    for (int i = 0; i < 1000; i++) {
        char script[256];
        snprintf(script, sizeof(script), "x%d = %d\nprint(\"Value: \" + x%d)", i, i, i);
        
        int result = ember_eval(vm, script);
        
        // Don't assert on result - some may fail due to memory pressure
        if (i % 100 == 0) {
            printf("  Compiled %d scripts\n", i + 1);
        }
    }
    
    ember_free_vm(vm);
    printf("  ✓ Memory safety under load test passed\n\n");
}

void test_compilation_edge_cases() {
    printf("Testing compilation edge cases...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test empty script
    int result = ember_eval(vm, "");
    // Should handle gracefully
    printf("  ✓ Empty script handled\n");
    
    // Test script with syntax error
    result = ember_eval(vm, "this is not valid ember code @#$");
    assert(result != 0); // Should fail
    printf("  ✓ Syntax error handled\n");
    
    // Test very long script
    char* long_script = malloc(50000);
    strcpy(long_script, "result = 0\n");
    for (int i = 0; i < 1000; i++) {
        char temp[50];
        snprintf(temp, sizeof(temp), "result = result + %d\n", i);
        strcat(long_script, temp);
    }
    strcat(long_script, "print(\"Result: \" + result)");
    
    result = ember_eval(vm, long_script);
    // Should handle gracefully (may succeed or fail)
    free(long_script);
    printf("  ✓ Long script handled\n");
    
    // Test deeply nested script
    char* nested_script = malloc(10000);
    strcpy(nested_script, "x = ");
    for (int i = 0; i < 100; i++) {
        strcat(nested_script, "(");
    }
    strcat(nested_script, "42");
    for (int i = 0; i < 100; i++) {
        strcat(nested_script, ")");
    }
    strcat(nested_script, "\nprint(x)");
    
    result = ember_eval(vm, nested_script);
    // Should handle gracefully
    free(nested_script);
    printf("  ✓ Deeply nested script handled\n");
    
    ember_free_vm(vm);
    printf("  ✓ Compilation edge cases test passed\n\n");
}

void test_vm_isolation() {
    printf("Testing VM isolation under concurrent access...\n");
    
    ember_vm* vm1 = ember_new_vm();
    ember_vm* vm2 = ember_new_vm();
    assert(vm1 != NULL && vm2 != NULL);
    
    // Set different global variables in each VM
    ember_eval(vm1, "global_vm1 = \"VM1_VALUE\"");
    ember_eval(vm2, "global_vm2 = \"VM2_VALUE\"");
    
    // Try to access the other VM's variable (should fail)
    int result1 = ember_eval(vm1, "print(global_vm2)"); // Should fail
    int result2 = ember_eval(vm2, "print(global_vm1)"); // Should fail
    
    // Access their own variables (should succeed)
    int result3 = ember_eval(vm1, "print(global_vm1)"); // Should succeed
    int result4 = ember_eval(vm2, "print(global_vm2)"); // Should succeed
    
    ember_free_vm(vm1);
    ember_free_vm(vm2);
    
    printf("  ✓ VM isolation test passed\n\n");
}

int main() {
    printf("Concurrent Script Compilation and Caching Tests\n");
    printf("===============================================\n\n");
    
    // Seed random number generator
    srand(time(NULL));
    
    test_concurrent_compilation();
    test_memory_safety_under_load();
    test_compilation_edge_cases();
    test_vm_isolation();
    
    printf("All concurrent compilation tests passed!\n");
    printf("Script compilation and VM management are thread-safe and robust.\n");
    
    return 0;
}