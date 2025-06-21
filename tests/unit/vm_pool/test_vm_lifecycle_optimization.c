#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include "../../src/core/vm_lifecycle_optimized.h"
#include "../../src/vm.h"

// Test configuration
#define TEST_ITERATIONS 10000
#define STRESS_TEST_ITERATIONS 100000
#define CONCURRENT_THREADS 4
#define PERFORMANCE_THRESHOLD_IMPROVEMENT 200 // 200% improvement minimum

// Test state
static int tests_run = 0;
static int tests_passed = 0;
static int tests_failed = 0;

// Utility functions
static uint64_t get_timestamp_us(void) {
    struct timeval tv;
    gettimeofday(&tv, NULL);
    return (uint64_t)tv.tv_sec * 1000000 + tv.tv_usec;
}

static double measure_time_us(void (*test_func)(void)) {
    uint64_t start = get_timestamp_us();
    test_func();
    uint64_t end = get_timestamp_us();
    return (double)(end - start);
}

static void print_test_result(const char* test_name, int passed) {
    tests_run++;
    if (passed) {
        tests_passed++;
        printf("✓ %s\n", test_name);
    } else {
        tests_failed++;
        printf("✗ %s\n", test_name);
    }
}

static void print_performance_result(const char* test_name, double baseline_us, double optimized_us) {
    double improvement = (baseline_us / optimized_us - 1.0) * 100.0;
    int passed = improvement >= PERFORMANCE_THRESHOLD_IMPROVEMENT;
    
    printf("  Baseline: %.2f μs\n", baseline_us);
    printf("  Optimized: %.2f μs\n", optimized_us);
    printf("  Improvement: %.1f%%\n", improvement);
    
    print_test_result(test_name, passed);
}

// Test implementations
static void test_vm_pool_manager_creation() {
    printf("\nTesting VM Pool Manager Creation...\n");
    
    vm_pool_manager_t* manager = vm_pool_manager_create();
    int passed = (manager != NULL);
    
    if (manager) {
        vm_pool_manager_destroy(manager);
    }
    
    print_test_result("VM Pool Manager Creation", passed);
}

static void test_vm_pool_initialization() {
    printf("\nTesting VM Pool Initialization...\n");
    
    int result = vm_pool_manager_initialize(16, 128);
    int passed = (result == 0);
    
    if (passed) {
        // Test that we can acquire VMs
        ember_vm* vm1 = vm_acquire_from_pool();
        ember_vm* vm2 = vm_acquire_from_pool();
        
        passed = (vm1 != NULL && vm2 != NULL && vm1 != vm2);
        
        if (vm1) vm_release_to_pool(vm1);
        if (vm2) vm_release_to_pool(vm2);
        
        vm_pool_manager_shutdown();
    }
    
    print_test_result("VM Pool Initialization", passed);
}

static void test_vm_template_system() {
    printf("\nTesting VM Template System...\n");
    
    vm_template_t* template = vm_template_create();
    int passed = (template != NULL);
    
    if (template) {
        ember_vm* vm1 = vm_instantiate_from_template(template);
        ember_vm* vm2 = vm_instantiate_from_template(template);
        
        passed = (vm1 != NULL && vm2 != NULL);
        
        if (vm1) vm_destroy_optimized(vm1);
        if (vm2) vm_destroy_optimized(vm2);
        
        vm_template_destroy(template);
    }
    
    print_test_result("VM Template System", passed);
}

static void test_vm_state_management() {
    printf("\nTesting VM State Management...\n");
    
    ember_vm* vm = vm_create_optimized();
    int passed = (vm != NULL);
    
    if (vm) {
        // Capture initial state
        vm_state_snapshot_t* snapshot = vm_state_capture(vm);
        passed = (snapshot != NULL && snapshot->is_valid);
        
        if (snapshot) {
            // Modify VM state
            push(vm, ember_make_number(42));
            push(vm, ember_make_string("test"));
            
            // Restore to initial state
            int restore_result = vm_state_restore(vm, snapshot);
            passed = passed && (restore_result == 0);
            
            // Reset to clean state
            int reset_result = vm_reset_to_clean_state(vm);
            passed = passed && (reset_result == 0) && (vm->stack_top == 0);
            
            vm_state_snapshot_destroy(snapshot);
        }
        
        vm_destroy_optimized(vm);
    }
    
    print_test_result("VM State Management", passed);
}

static void test_object_pool_allocation() {
    printf("\nTesting Object Pool Allocation...\n");
    
    // Initialize if not already done
    if (!g_vm_pool_manager) {
        vm_pool_manager_initialize(16, 128);
    }
    
    int passed = 1;
    
    // Test various object sizes
    size_t test_sizes[] = {32, 128, 256, 1024, 4096};
    int num_sizes = sizeof(test_sizes) / sizeof(test_sizes[0]);
    
    for (int i = 0; i < num_sizes; i++) {
        void* obj = optimized_object_alloc(test_sizes[i], OBJ_STRING);
        if (!obj) {
            passed = 0;
            break;
        }
        
        // Write to the object to ensure it's valid
        memset(obj, 0x42, test_sizes[i]);
        
        optimized_object_free(obj, test_sizes[i], OBJ_STRING);
    }
    
    print_test_result("Object Pool Allocation", passed);
}

static void test_resource_pool_operations() {
    printf("\nTesting Resource Pool Operations...\n");
    
    int passed = 1;
    
    // Test stack pool
    ember_value* stack1 = vm_stack_pool_acquire();
    ember_value* stack2 = vm_stack_pool_acquire();
    
    if (!stack1 || !stack2 || stack1 == stack2) {
        passed = 0;
    } else {
        // Test that stacks are properly zeroed
        for (int i = 0; i < EMBER_STACK_MAX; i++) {
            if (stack1[i].type != EMBER_VAL_NIL || stack2[i].type != EMBER_VAL_NIL) {
                passed = 0;
                break;
            }
        }
        
        vm_stack_pool_release(stack1);
        vm_stack_pool_release(stack2);
    }
    
    // Test locals pool
    if (passed) {
        ember_value* locals1 = vm_locals_pool_acquire();
        ember_value* locals2 = vm_locals_pool_acquire();
        
        if (!locals1 || !locals2 || locals1 == locals2) {
            passed = 0;
        } else {
            vm_locals_pool_release(locals1);
            vm_locals_pool_release(locals2);
        }
    }
    
    print_test_result("Resource Pool Operations", passed);
}

// Performance benchmarks
static void baseline_vm_creation_test() {
    ember_vm* vm = ember_new_vm();
    ember_free_vm(vm);
}

static void optimized_vm_creation_test() {
    ember_vm* vm = vm_create_optimized();
    vm_destroy_optimized(vm);
}

static void vm_pool_acquire_release_test() {
    ember_vm* vm = vm_acquire_from_pool();
    vm_release_to_pool(vm);
}

static void test_vm_creation_performance() {
    printf("\nTesting VM Creation Performance...\n");
    
    // Initialize pool for optimized tests
    vm_pool_manager_initialize(32, 256);
    
    // Warmup
    for (int i = 0; i < 1000; i++) {
        baseline_vm_creation_test();
        optimized_vm_creation_test();
        vm_pool_acquire_release_test();
    }
    
    // Measure baseline VM creation
    double total_baseline = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        total_baseline += measure_time_us(baseline_vm_creation_test);
    }
    double avg_baseline = total_baseline / TEST_ITERATIONS;
    
    // Measure optimized VM creation
    double total_optimized = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        total_optimized += measure_time_us(optimized_vm_creation_test);
    }
    double avg_optimized = total_optimized / TEST_ITERATIONS;
    
    print_performance_result("VM Creation Performance", avg_baseline, avg_optimized);
    
    // Measure VM pool operations
    double total_pool = 0;
    for (int i = 0; i < TEST_ITERATIONS; i++) {
        total_pool += measure_time_us(vm_pool_acquire_release_test);
    }
    double avg_pool = total_pool / TEST_ITERATIONS;
    
    print_performance_result("VM Pool Operations", avg_baseline, avg_pool);
    
    vm_pool_manager_shutdown();
}

static void baseline_object_allocation_test() {
    void* obj = malloc(256);
    if (obj) {
        memset(obj, 0x42, 256);
        free(obj);
    }
}

static void optimized_object_allocation_test() {
    void* obj = optimized_object_alloc(256, OBJ_STRING);
    if (obj) {
        memset(obj, 0x42, 256);
        optimized_object_free(obj, 256, OBJ_STRING);
    }
}

static void test_object_allocation_performance() {
    printf("\nTesting Object Allocation Performance...\n");
    
    // Initialize pool
    vm_pool_manager_initialize(32, 256);
    object_pools_prewarm();
    
    // Warmup
    for (int i = 0; i < 1000; i++) {
        baseline_object_allocation_test();
        optimized_object_allocation_test();
    }
    
    // Measure baseline allocation
    double total_baseline = 0;
    for (int i = 0; i < TEST_ITERATIONS * 2; i++) {
        total_baseline += measure_time_us(baseline_object_allocation_test);
    }
    double avg_baseline = total_baseline / (TEST_ITERATIONS * 2);
    
    // Measure optimized allocation
    double total_optimized = 0;
    for (int i = 0; i < TEST_ITERATIONS * 2; i++) {
        total_optimized += measure_time_us(optimized_object_allocation_test);
    }
    double avg_optimized = total_optimized / (TEST_ITERATIONS * 2);
    
    print_performance_result("Object Allocation Performance", avg_baseline, avg_optimized);
    
    vm_pool_manager_shutdown();
}

static void test_memory_pool_integrity() {
    printf("\nTesting Memory Pool Integrity...\n");
    
    vm_pool_manager_initialize(16, 128);
    
    int passed = vm_pool_validate_integrity();
    
    // Stress test the pools
    if (passed) {
        ember_vm* vms[64];
        
        // Acquire many VMs
        for (int i = 0; i < 64; i++) {
            vms[i] = vm_acquire_from_pool();
            if (!vms[i]) {
                passed = 0;
                break;
            }
        }
        
        // Release them all
        for (int i = 0; i < 64; i++) {
            if (vms[i]) {
                vm_release_to_pool(vms[i]);
            }
        }
        
        // Validate integrity again
        passed = passed && vm_pool_validate_integrity();
    }
    
    vm_pool_manager_shutdown();
    
    print_test_result("Memory Pool Integrity", passed);
}

static void test_concurrent_vm_operations() {
    printf("\nTesting Concurrent VM Operations...\n");
    
    vm_pool_manager_initialize(32, 256);
    
    // Enable all optimizations
    vm_pool_configure(true, true, true, true);
    
    int passed = 1;
    volatile int error_count = 0;
    
    // Simple concurrent test - create multiple threads that acquire/release VMs
    #pragma omp parallel for num_threads(CONCURRENT_THREADS)
    for (int i = 0; i < TEST_ITERATIONS / CONCURRENT_THREADS; i++) {
        ember_vm* vm = vm_acquire_from_pool();
        if (!vm) {
            __sync_fetch_and_add(&error_count, 1);
            continue;
        }
        
        // Simulate some work
        push(vm, ember_make_number(i));
        ember_value val = pop(vm);
        if (val.type != EMBER_VAL_NUMBER || val.as.number_val != i) {
            __sync_fetch_and_add(&error_count, 1);
        }
        
        vm_release_to_pool(vm);
    }
    
    passed = (error_count == 0);
    
    if (!passed) {
        printf("  Errors detected: %d\n", error_count);
    }
    
    vm_pool_manager_shutdown();
    
    print_test_result("Concurrent VM Operations", passed);
}

static void test_stress_scenarios() {
    printf("\nTesting Stress Scenarios...\n");
    
    vm_pool_manager_initialize(64, 512);
    
    int passed = 1;
    uint64_t operations = 0;
    uint64_t errors = 0;
    
    printf("  Running %d stress operations...\n", STRESS_TEST_ITERATIONS);
    
    uint64_t start_time = get_timestamp_us();
    
    for (int i = 0; i < STRESS_TEST_ITERATIONS; i++) {
        ember_vm* vm = vm_acquire_from_pool();
        if (!vm) {
            errors++;
            continue;
        }
        
        // Simulate varying workloads
        switch (i % 4) {
            case 0:
                // Simple arithmetic
                push(vm, ember_make_number(10));
                push(vm, ember_make_number(20));
                push(vm, ember_make_number(30));
                pop(vm); pop(vm); pop(vm);
                break;
            case 1:
                // String operations
                push(vm, ember_make_string_gc(vm, "test"));
                pop(vm);
                break;
            case 2:
                // Array operations
                push(vm, ember_make_array(vm, 10));
                pop(vm);
                break;
            case 3:
                // Reset state
                vm_reset_to_clean_state(vm);
                break;
        }
        
        vm_release_to_pool(vm);
        operations++;
        
        if (i % 10000 == 0 && i > 0) {
            printf("    Progress: %d operations completed\n", i);
        }
    }
    
    uint64_t end_time = get_timestamp_us();
    double total_time_sec = (end_time - start_time) / 1000000.0;
    double ops_per_sec = operations / total_time_sec;
    
    printf("  Completed %lu operations in %.2f seconds\n", operations, total_time_sec);
    printf("  Throughput: %.0f operations/second\n", ops_per_sec);
    printf("  Error rate: %.2f%%\n", (double)errors / STRESS_TEST_ITERATIONS * 100.0);
    
    // Check if we met performance targets
    passed = (ops_per_sec >= 25000) && (errors < STRESS_TEST_ITERATIONS * 0.01);
    
    vm_pool_manager_shutdown();
    
    print_test_result("Stress Test Performance", passed);
}

static void test_background_management() {
    printf("\nTesting Background Management...\n");
    
    vm_pool_manager_initialize(32, 256);
    
    // Enable background features
    vm_pool_configure(true, true, true, true);
    
    int passed = 1;
    
    // Let background threads run for a short time
    sleep(2);
    
    // Check that pool is still functioning
    ember_vm* vm = vm_acquire_from_pool();
    passed = (vm != NULL);
    
    if (vm) {
        vm_release_to_pool(vm);
    }
    
    // Check pool integrity
    passed = passed && vm_pool_validate_integrity();
    
    vm_pool_manager_shutdown();
    
    print_test_result("Background Management", passed);
}

// Main test runner
int main(void) {
    printf("========================================\n");
    printf("VM Lifecycle Optimization Unit Tests\n");
    printf("========================================\n");
    
    // Basic functionality tests
    test_vm_pool_manager_creation();
    test_vm_pool_initialization();
    test_vm_template_system();
    test_vm_state_management();
    test_object_pool_allocation();
    test_resource_pool_operations();
    test_memory_pool_integrity();
    
    // Performance tests
    test_vm_creation_performance();
    test_object_allocation_performance();
    
    // Concurrency and stress tests
    test_concurrent_vm_operations();
    test_stress_scenarios();
    test_background_management();
    
    // Print summary
    printf("\n========================================\n");
    printf("Test Summary\n");
    printf("========================================\n");
    printf("Tests run: %d\n", tests_run);
    printf("Tests passed: %d\n", tests_passed);
    printf("Tests failed: %d\n", tests_failed);
    printf("Success rate: %.1f%%\n", (double)tests_passed / tests_run * 100.0);
    
    if (tests_failed == 0) {
        printf("\n🎉 All tests passed! VM lifecycle optimization is working correctly.\n");
        return 0;
    } else {
        printf("\n⚠️  %d test(s) failed. Please review the implementation.\n", tests_failed);
        return 1;
    }
}