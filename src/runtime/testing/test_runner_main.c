#include "testing_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// External test suite declarations
extern void run_collections_tests(test_runner* runner);
extern void run_regex_tests(test_runner* runner);

// Basic VM functionality tests
TEST_DEFINE(vm_creation) {
    TEST_ASSERT(vm != NULL, "VM should be created successfully");
    TEST_ASSERT(vm->stack_top == 0, "VM stack should be empty initially");
}

TEST_DEFINE(value_creation) {
    ember_value num = ember_make_number(42.5);
    TEST_ASSERT(num.type == EMBER_VAL_NUMBER, "Number value should have correct type");
    TEST_ASSERT_NUM_EQ(42.5, num.as.number_val, 0.001, "Number value should have correct value");
    
    ember_value bool_true = ember_make_bool(1);
    ember_value bool_false = ember_make_bool(0);
    TEST_ASSERT(bool_true.type == EMBER_VAL_BOOL, "Boolean value should have correct type");
    TEST_ASSERT(bool_true.as.bool_val == 1, "True boolean should have value 1");
    TEST_ASSERT(bool_false.as.bool_val == 0, "False boolean should have value 0");
    
    ember_value nil = ember_make_nil();
    TEST_ASSERT(nil.type == EMBER_VAL_NIL, "Nil value should have correct type");
    
    ember_value str = ember_make_string_gc(vm, "hello world");
    TEST_ASSERT(str.type == EMBER_VAL_STRING, "String value should have correct type");
    TEST_ASSERT_STR_EQ("hello world", AS_CSTRING(str), "String should have correct content");
}

TEST_DEFINE(value_equality) {
    ember_value num1 = ember_make_number(123);
    ember_value num2 = ember_make_number(123);
    ember_value num3 = ember_make_number(456);
    
    TEST_ASSERT_EQ(num1, num2, "Equal numbers should be equal");
    TEST_ASSERT_NEQ(num1, num3, "Different numbers should not be equal");
    
    ember_value str1 = ember_make_string_gc(vm, "test");
    ember_value str2 = ember_make_string_gc(vm, "test");
    ember_value str3 = ember_make_string_gc(vm, "different");
    
    TEST_ASSERT_EQ(str1, str2, "Equal strings should be equal");
    TEST_ASSERT_NEQ(str1, str3, "Different strings should not be equal");
    
    ember_value nil1 = ember_make_nil();
    ember_value nil2 = ember_make_nil();
    
    TEST_ASSERT_EQ(nil1, nil2, "Nil values should be equal");
}

TEST_DEFINE(array_operations) {
    ember_value array = ember_make_array(vm, 10);
    TEST_ASSERT(array.type == EMBER_VAL_ARRAY, "Array should have correct type");
    
    ember_array* arr = AS_ARRAY(array);
    TEST_ASSERT(arr->length == 0, "New array should be empty");
    TEST_ASSERT(arr->capacity == 10, "Array should have specified capacity");
    
    // Test array push
    ember_value elem1 = ember_make_number(1);
    ember_value elem2 = ember_make_string_gc(vm, "two");
    ember_value elem3 = ember_make_number(3);
    
    array_push(arr, elem1);
    array_push(arr, elem2);
    array_push(arr, elem3);
    
    TEST_ASSERT(arr->length == 3, "Array should have 3 elements after pushes");
    TEST_ASSERT_EQ(elem1, arr->elements[0], "First element should match");
    TEST_ASSERT_EQ(elem2, arr->elements[1], "Second element should match");
    TEST_ASSERT_EQ(elem3, arr->elements[2], "Third element should match");
}

// Performance benchmarks
void benchmark_value_creation(ember_vm* vm) {
    for (int i = 0; i < 10000; i++) {
        ember_value num = ember_make_number(i);
        ember_value str = ember_make_string_gc(vm, "benchmark");
        ember_value arr = ember_make_array(vm, 5);
        (void)num; (void)str; (void)arr; // Suppress unused variable warnings
    }
}

void benchmark_array_operations(ember_vm* vm) {
    ember_value array = ember_make_array(vm, 1000);
    ember_array* arr = AS_ARRAY(array);
    
    for (int i = 0; i < 1000; i++) {
        array_push(arr, ember_make_number(i));
    }
}

TEST_DEFINE(performance_benchmarks) {
    test_benchmark("Value Creation", benchmark_value_creation, vm, 10);
    test_benchmark("Array Operations", benchmark_array_operations, vm, 10);
}

// Core test suite
TEST_SUITE(core) {
    ember_vm* vm = ember_new_vm();
    test_suite* suite = test_runner_add_suite(runner, "Core Functionality");
    
    test_suite_add_test(suite, "VM Creation", test_vm_creation, vm);
    test_suite_add_test(suite, "Value Creation", test_value_creation, vm);
    test_suite_add_test(suite, "Value Equality", test_value_equality, vm);
    test_suite_add_test(suite, "Array Operations", test_array_operations, vm);
    test_suite_add_test(suite, "Performance Benchmarks", test_performance_benchmarks, vm);
    
    ember_free_vm(vm);
}

// Error handling test
void error_function(ember_vm* vm) {
    ember_error* error = ember_error_runtime(vm, "This is a test error");
    ember_vm_set_error(vm, error);
}

TEST_DEFINE(error_handling) {
    TEST_ASSERT_THROWS(error_function, "test error", "Should throw expected error");
}

TEST_SUITE(error_handling_suite) {
    ember_vm* vm = ember_new_vm();
    test_suite* suite = test_runner_add_suite(runner, "Error Handling");
    
    test_suite_add_test(suite, "Error Throwing", test_error_handling, vm);
    
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc; (void)argv; // Suppress unused parameter warnings
    
    printf("Ember Testing Framework v2.0.3\\n");
    printf("Comprehensive test suite for new language features\\n\\n");
    
    // Create test runner
    test_runner* runner = test_runner_create();
    if (!runner) {
        printf("Failed to create test runner\\n");
        return 1;
    }
    
    // Configure runner
    runner->verbose = 1;
    runner->stop_on_failure = 0;
    
    // Add test suites
    run_core_tests(runner);
    run_error_handling_suite_tests(runner);
    run_collections_tests(runner);
    run_regex_tests(runner);
    
    // Run all tests
    test_runner_run_all(runner);
    
    // Print results
    test_runner_print_results(runner);
    
    // Determine exit code
    int exit_code = (runner->total_failed > 0 || runner->total_errors > 0) ? 1 : 0;
    
    // Cleanup
    test_runner_free(runner);
    
    return exit_code;
}