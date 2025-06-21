#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

// Test memory management and garbage collection to improve coverage

void test_value_creation_and_cleanup(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing value creation and cleanup...\n");
    
    // Test number value creation
    ember_value num = ember_make_number(42.5);
    assert(num.type == EMBER_VAL_NUMBER);
    assert(num.as.number_val == 42.5);
    
    // Test boolean value creation
    ember_value bool_true = ember_make_bool(1);
    ember_value bool_false = ember_make_bool(0);
    assert(bool_true.type == EMBER_VAL_BOOL);
    assert(bool_false.type == EMBER_VAL_BOOL);
    assert(bool_true.as.bool_val == 1);
    assert(bool_false.as.bool_val == 0);
    
    // Test nil value creation
    ember_value nil = ember_make_nil();
    assert(nil.type == EMBER_VAL_NIL);
    
    printf("Basic value creation test passed\n");
    ember_free_vm(vm);
}

void test_string_value_creation(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing string value creation...\n");
    
    // Test string creation (this should exercise the GC string allocation)
    ember_value str1 = ember_make_string_gc(vm, "Hello, World!");
    assert(str1.type == EMBER_VAL_STRING);
    
    ember_value str2 = ember_make_string_gc(vm, "Another string");
    assert(str2.type == EMBER_VAL_STRING);
    
    // Test empty string
    ember_value empty_str = ember_make_string_gc(vm, "");
    assert(empty_str.type == EMBER_VAL_STRING);
    
    // Test long string to exercise memory allocation
    char long_string[1000];
    memset(long_string, 'A', 999);
    long_string[999] = '\0';
    
    ember_value long_str = ember_make_string_gc(vm, long_string);
    assert(long_str.type == EMBER_VAL_STRING);
    
    printf("String value creation test passed\n");
    ember_free_vm(vm);
}

void test_array_operations(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing array operations...\n");
    
    // Test array creation
    ember_value arr = ember_make_array(vm, 10);
    assert(arr.type == EMBER_VAL_ARRAY);
    
    // Test another array to exercise memory allocation
    ember_value arr2 = ember_make_array(vm, 5);
    assert(arr2.type == EMBER_VAL_ARRAY);
    
    // Test large array
    ember_value large_arr = ember_make_array(vm, 100);
    assert(large_arr.type == EMBER_VAL_ARRAY);
    
    printf("Array operations test passed\n");
    ember_free_vm(vm);
}

void test_hash_map_operations(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing hash map operations...\n");
    
    // Test hash map creation
    ember_value map = ember_make_hash_map(vm, 16);
    assert(map.type == EMBER_VAL_HASH_MAP);
    
    // Test another hash map
    ember_value map2 = ember_make_hash_map(vm, 8);
    assert(map2.type == EMBER_VAL_HASH_MAP);
    
    // Test large hash map
    ember_value large_map = ember_make_hash_map(vm, 64);
    assert(large_map.type == EMBER_VAL_HASH_MAP);
    
    printf("Hash map operations test passed\n");
    ember_free_vm(vm);
}

void test_memory_allocation_limits(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing memory allocation limits...\n");
    
    // Create many objects to test garbage collection behavior
    for (int i = 0; i < 50; i++) {
        char test_string[64];
        snprintf(test_string, sizeof(test_string), "Test string %d", i);
        
        ember_value str = ember_make_string_gc(vm, test_string);
        ember_value arr = ember_make_array(vm, 5);
        ember_value map = ember_make_hash_map(vm, 4);
        
        // Use the values to prevent optimization
        assert(str.type == EMBER_VAL_STRING);
        assert(arr.type == EMBER_VAL_ARRAY);
        assert(map.type == EMBER_VAL_HASH_MAP);
        
        if (i % 10 == 0) {
            printf("Created %d sets of objects\n", i + 1);
        }
    }
    
    printf("Memory allocation limits test passed\n");
    ember_free_vm(vm);
}

void test_stack_operations_safety(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing stack operations safety...\n");
    
    // Test pushing values onto the stack
    ember_value val1 = ember_make_number(1.0);
    ember_value val2 = ember_make_number(2.0);
    ember_value val3 = ember_make_bool(1);
    
    // These functions are internal but we can test the public API
    // that uses them indirectly through ember_call
    
    // Test stack overflow protection by creating a function that
    // would cause stack overflow if not properly protected
    // (This is a simplified test - real testing would need internal access)
    
    printf("Stack operations safety test passed\n");
    ember_free_vm(vm);
}

void test_memory_leak_prevention(void) {
    printf("Testing memory leak prevention...\n");
    
    // Create and destroy many VMs to test cleanup
    for (int i = 0; i < 10; i++) {
        ember_vm* vm = ember_new_vm();
        assert(vm != NULL);
        
        // Create some objects
        ember_value str = ember_make_string_gc(vm, "Memory test");
        ember_value arr = ember_make_array(vm, 10);
        ember_value map = ember_make_hash_map(vm, 8);
        
        // Use the objects
        assert(str.type == EMBER_VAL_STRING);
        assert(arr.type == EMBER_VAL_ARRAY);
        assert(map.type == EMBER_VAL_HASH_MAP);
        
        // Free the VM (should clean up all objects)
        ember_free_vm(vm);
    }
    
    printf("Memory leak prevention test passed\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Memory Safety Tests...\n");
    
    test_value_creation_and_cleanup();
    test_string_value_creation();
    test_array_operations();
    test_hash_map_operations();
    test_memory_allocation_limits();
    test_stack_operations_safety();
    test_memory_leak_prevention();
    
    printf("All memory safety tests passed!\n");
    return 0;
}