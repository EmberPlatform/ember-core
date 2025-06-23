#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <string.h>
#include "ember.h"
// #include "../../src/core/vm_optimized.h" // File doesn't exist
#include "../../src/core/arena_allocator.h"
#include "../../src/core/string_intern_optimized.h"

// Test program to validate and benchmark the new optimizations

void test_arena_allocator() {
    printf("\n=== Testing Arena Allocator ===\n");
    
    arena_allocator* arena = arena_create();
    if (!arena) {
        printf("Failed to create arena allocator\n");
        return;
    }
    
    clock_t start = clock();
    
    // Test allocation patterns
    void* ptrs[1000];
    for (int i = 0; i < 1000; i++) {
        size_t size = (i % 100) + 1;  // Variable sizes 1-100 bytes
        ptrs[i] = arena_alloc(arena, size);
        if (!ptrs[i]) {
            printf("Arena allocation failed at iteration %d\n", i);
            break;
        }
        
        // Write some data to test memory validity
        memset(ptrs[i], i % 256, size);
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("Arena allocations completed in %.3f ms\n", time_taken);
    arena_print_stats(arena);
    
    // Test reset functionality
    arena_reset(arena);
    printf("Arena reset - memory can be reused\n");
    arena_print_stats(arena);
    
    arena_destroy(arena);
    printf("Arena allocator test completed\n");
}

void test_string_interning() {
    printf("\n=== Testing Optimized String Interning ===\n");
    
    string_intern_table* table = string_intern_create();
    if (!table) {
        printf("Failed to create string intern table\n");
        return;
    }
    
    clock_t start = clock();
    
    // Test with common strings that should be deduplicated
    const char* common_strings[] = {
        "hello", "world", "test", "string", "optimization",
        "performance", "benchmark", "ember", "vm", "cache"
    };
    int num_common = sizeof(common_strings) / sizeof(common_strings[0]);

    UNUSED(num_common);
    
    const char* interned[10000];
    int duplicates = 0;

    UNUSED(duplicates);
    
    // Intern many strings with repetition
    for (int i = 0; i < 10000; i++) {
        const char* original = common_strings[i % num_common];

        UNUSED(original);
        interned[i] = string_intern_cstr(table, original);
        
        // Check if interning worked (same pointer for same string)
        if (i > 0) {
            for (int j = 0; j < i; j++) {
                if (strcmp(interned[i], interned[j]) == 0 && interned[i] == interned[j]) {
                    duplicates++;
                    break;
                }
            }
        }
    }
    
    clock_t end = clock();
    double time_taken = ((double)(end - start)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("String interning completed in %.3f ms\n", time_taken);
    printf("Duplicate detections: %d\n", duplicates);
    string_intern_print_stats(table);
    
    string_intern_destroy(table);
    printf("String interning test completed\n");
}

void test_vm_optimizations() {
    printf("\n=== Testing VM Optimizations ===\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return;
    }
    
    // Initialize optimizations
    vm_optimization_init(vm);
    vm_set_optimization_level(3); // Maximum optimizations
    
    // Test simple arithmetic that benefits from optimization
    const char* test_code = 
        "result = 0\n"
        "i = 0\n"
        "while i < 10000 {\n"
        "    result = result + i * 2\n"  // Strength reduction: i*2 -> i+i
        "    result = result + 0\n"      // Identity: +0 -> nop
        "    result = result * 1\n"      // Identity: *1 -> nop 
        "    result = result + (5 + 10)\n" // Constant folding: 5+10 -> 15
        "    i = i + 1\n"
        "}\n"
        "print(\"Optimized result:\", result)\n";

    UNUSED(test_code);
    
    printf("Running optimization test with standard VM...\n");
    clock_t start1 = clock();
    int result1 = ember_eval(vm, test_code);

    UNUSED(result1);
    clock_t end1 = clock();
    double time1 = ((double)(end1 - start1)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("Standard VM execution time: %.3f ms\n", time1);
    if (result1 != 0) {
        printf("Standard VM execution failed\n");
    }
    
    printf("\nRunning optimization test with optimized VM...\n");
    ember_vm_clear_error(vm);
    clock_t start2 = clock();
    int result2 = ember_run_optimized(vm);

    UNUSED(result2);
    clock_t end2 = clock();
    double time2 = ((double)(end2 - start2)) / CLOCKS_PER_SEC * 1000.0;
    
    printf("Optimized VM execution time: %.3f ms\n", time2);
    if (result2 != 0) {
        printf("Optimized VM execution failed\n");
    }
    
    // Print performance statistics
    vm_print_performance_stats();
    
    if (time1 > 0 && time2 > 0) {
        double speedup = time1 / time2;
        printf("Speedup: %.2fx\n", speedup);
        
        if (speedup > 1.2) {
            printf("✓ Significant optimization achieved!\n");
        } else if (speedup > 1.05) {
            printf("~ Modest optimization achieved\n");
        } else {
            printf("⚠ No significant optimization detected\n");
        }
    }
    
    vm_optimization_cleanup();
    ember_free_vm(vm);
    printf("VM optimization test completed\n");
}

void benchmark_memory_operations() {
    printf("\n=== Benchmarking Memory Operations ===\n");
    
    // Compare standard malloc/free vs arena allocation
    const int iterations = 10000;
 UNUSED(iterations);
    const int allocation_sizes[] = {16, 32, 64, 128, 256};
    const int num_sizes = sizeof(allocation_sizes) / sizeof(allocation_sizes[0]);
 UNUSED(num_sizes);
    
    printf("Testing %d allocations with standard malloc/free...\n", iterations);
    clock_t start1 = clock();
    
    void* ptrs[iterations];
    for (int i = 0; i < iterations; i++) {
        size_t size = allocation_sizes[i % num_sizes];
        ptrs[i] = malloc(size);
        if (ptrs[i]) {
            memset(ptrs[i], i % 256, size); // Touch memory
        }
    }
    
    for (int i = 0; i < iterations; i++) {
        free(ptrs[i]);
    }
    
    clock_t end1 = clock();
    double malloc_time = ((double)(end1 - start1)) / CLOCKS_PER_SEC * 1000.0;
    printf("Standard malloc/free time: %.3f ms\n", malloc_time);
    
    printf("Testing %d allocations with arena allocator...\n", iterations);
    arena_allocator* arena = arena_create();
    
    clock_t start2 = clock();
    
    for (int i = 0; i < iterations; i++) {
        size_t size = allocation_sizes[i % num_sizes];
        void* ptr = arena_alloc(arena, size);
        if (ptr) {
            memset(ptr, i % 256, size); // Touch memory
        }
    }
    
    clock_t end2 = clock();
    double arena_time = ((double)(end2 - start2)) / CLOCKS_PER_SEC * 1000.0;
    printf("Arena allocation time: %.3f ms\n", arena_time);
    
    if (malloc_time > 0 && arena_time > 0) {
        double speedup = malloc_time / arena_time;
        printf("Arena speedup: %.2fx\n", speedup);
    }
    
    arena_destroy(arena);
    printf("Memory operation benchmark completed\n");
}

int main() {
    printf("Ember VM Performance Optimization Test Suite\n");
    printf("============================================\n");
    
    // Run all tests
    test_arena_allocator();
    test_string_interning();
    benchmark_memory_operations();
    test_vm_optimizations();
    
    printf("\n=== Optimization Test Suite Complete ===\n");
    printf("Summary of implemented optimizations:\n");
    printf("• Arena allocator for fast memory management\n");
    printf("• Optimized string interning with Robin Hood hashing\n");
    printf("• Branch prediction hints in VM dispatch\n");
    printf("• Strength reduction in arithmetic operations\n");
    printf("• Identity operation elimination\n");
    printf("• Optimized stack operations with reduced bounds checking\n");
    printf("• Performance monitoring and statistics\n");
    
    return 0;
}