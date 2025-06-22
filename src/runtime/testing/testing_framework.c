#include "testing_framework.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <math.h>

// Color codes for output
#define COLOR_RED     "\x1b[31m"
#define COLOR_GREEN   "\x1b[32m"
#define COLOR_YELLOW  "\x1b[33m"
#define COLOR_BLUE    "\x1b[34m"
#define COLOR_MAGENTA "\x1b[35m"
#define COLOR_CYAN    "\x1b[36m"
#define COLOR_RESET   "\x1b[0m"

// Global test context for assertions
static test_case* current_test = NULL;
static test_suite* current_suite = NULL;

// Helper function to get current time in milliseconds
static double get_time_ms(void) {
    struct timespec ts;
    clock_gettime(CLOCK_MONOTONIC, &ts);
    return ts.tv_sec * 1000.0 + ts.tv_nsec / 1000000.0;
}

// Test runner functions
test_runner* test_runner_create(void) {
    test_runner* runner = malloc(sizeof(test_runner));
    if (!runner) return NULL;
    
    runner->suites = NULL;
    runner->suite_count = 0;
    runner->suite_capacity = 0;
    runner->total_tests = 0;
    runner->total_passed = 0;
    runner->total_failed = 0;
    runner->total_skipped = 0;
    runner->total_errors = 0;
    runner->total_duration_ms = 0.0;
    runner->verbose = 1;
    runner->stop_on_failure = 0;
    
    return runner;
}

void test_runner_free(test_runner* runner) {
    if (!runner) return;
    
    for (int i = 0; i < runner->suite_count; i++) {
        test_suite* suite = &runner->suites[i];
        for (int j = 0; j < suite->test_count; j++) {
            free(suite->tests[j].name);
            free(suite->tests[j].message);
        }
        free(suite->tests);
        free(suite->name);
    }
    
    free(runner->suites);
    free(runner);
}

test_suite* test_runner_add_suite(test_runner* runner, const char* name) {
    if (!runner || !name) return NULL;
    
    // Expand suites array if needed
    if (runner->suite_count >= runner->suite_capacity) {
        int new_capacity = runner->suite_capacity == 0 ? 4 : runner->suite_capacity * 2;
        test_suite* new_suites = realloc(runner->suites, sizeof(test_suite) * new_capacity);
        if (!new_suites) return NULL;
        
        runner->suites = new_suites;
        runner->suite_capacity = new_capacity;
    }
    
    test_suite* suite = &runner->suites[runner->suite_count++];
    suite->name = strdup(name);
    suite->tests = NULL;
    suite->test_count = 0;
    suite->test_capacity = 0;
    suite->passed = 0;
    suite->failed = 0;
    suite->skipped = 0;
    suite->errors = 0;
    suite->total_duration_ms = 0.0;
    
    return suite;
}

void test_suite_add_test(test_suite* suite, const char* name, test_function func, ember_vm* vm) {
    if (!suite || !name || !func || !vm) return;
    
    // Expand tests array if needed
    if (suite->test_count >= suite->test_capacity) {
        int new_capacity = suite->test_capacity == 0 ? 8 : suite->test_capacity * 2;
        test_case* new_tests = realloc(suite->tests, sizeof(test_case) * new_capacity);
        if (!new_tests) return;
        
        suite->tests = new_tests;
        suite->test_capacity = new_capacity;
    }
    
    test_case* test = &suite->tests[suite->test_count++];
    test->name = strdup(name);
    test->status = TEST_PASS;
    test->message = NULL;
    test->duration_ms = 0.0;
    test->line = 0;
    test->file = NULL;
    
    // Set global context for assertions
    current_test = test;
    current_suite = suite;
    
    // Run the test
    double start_time = get_time_ms();
    
    // Setup VM for test
    test_setup_vm(vm);
    
    // Clear any existing errors
    ember_vm_clear_error(vm);
    
    // Run test function
    func(vm);
    
    // Check if VM has error after test
    if (ember_vm_has_error(vm)) {
        test->status = TEST_ERROR;
        ember_error* error = ember_vm_get_error(vm);
        if (error && error->message) {
            test->message = strdup(error->message);
        } else {
            test->message = strdup("Unknown VM error occurred");
        }
    }
    
    // Cleanup VM after test
    test_cleanup_vm(vm);
    
    double end_time = get_time_ms();
    test->duration_ms = end_time - start_time;
    
    // Update suite statistics
    switch (test->status) {
        case TEST_PASS:
            suite->passed++;
            break;
        case TEST_FAIL:
            suite->failed++;
            break;
        case TEST_SKIP:
            suite->skipped++;
            break;
        case TEST_ERROR:
            suite->errors++;
            break;
    }
    
    suite->total_duration_ms += test->duration_ms;
    
    // Clear global context
    current_test = NULL;
    current_suite = NULL;
}

void test_runner_run_all(test_runner* runner) {
    if (!runner) return;
    
    printf("%s=== Ember Testing Framework ===%s\\n", COLOR_CYAN, COLOR_RESET);
    printf("Running %d test suite(s)\\n\\n", runner->suite_count);
    
    double start_time = get_time_ms();
    
    for (int i = 0; i < runner->suite_count; i++) {
        test_suite* suite = &runner->suites[i];
        
        printf("%s[%d/%d] %s%s\\n", COLOR_BLUE, i + 1, runner->suite_count, suite->name, COLOR_RESET);
        
        for (int j = 0; j < suite->test_count; j++) {
            test_case* test = &suite->tests[j];
            
            const char* status_color;
            const char* status_text;
            
            switch (test->status) {
                case TEST_PASS:
                    status_color = COLOR_GREEN;
                    status_text = "PASS";
                    break;
                case TEST_FAIL:
                    status_color = COLOR_RED;
                    status_text = "FAIL";
                    break;
                case TEST_SKIP:
                    status_color = COLOR_YELLOW;
                    status_text = "SKIP";
                    break;
                case TEST_ERROR:
                    status_color = COLOR_MAGENTA;
                    status_text = "ERROR";
                    break;
            }
            
            printf("  %s%s%s %s (%.2fms)\\n", 
                   status_color, status_text, COLOR_RESET,
                   test->name, test->duration_ms);
            
            if (test->status != TEST_PASS && test->message) {
                printf("    %s%s%s\\n", COLOR_RED, test->message, COLOR_RESET);
            }
            
            if (runner->stop_on_failure && test->status == TEST_FAIL) {
                printf("\\n%sSTOPPING ON FIRST FAILURE%s\\n", COLOR_RED, COLOR_RESET);
                return;
            }
        }
        
        // Print suite summary
        printf("  %sSuite Summary:%s %d/%d passed (%.2fms)\\n\\n", 
               COLOR_CYAN, COLOR_RESET,
               suite->passed, suite->test_count, suite->total_duration_ms);
        
        // Update runner totals
        runner->total_tests += suite->test_count;
        runner->total_passed += suite->passed;
        runner->total_failed += suite->failed;
        runner->total_skipped += suite->skipped;
        runner->total_errors += suite->errors;
    }
    
    double end_time = get_time_ms();
    runner->total_duration_ms = end_time - start_time;
}

void test_runner_print_results(test_runner* runner) {
    if (!runner) return;
    
    printf("%s=== Test Results ===%s\\n", COLOR_CYAN, COLOR_RESET);
    printf("Total Tests: %d\\n", runner->total_tests);
    printf("%sPassed: %d%s\\n", COLOR_GREEN, runner->total_passed, COLOR_RESET);
    
    if (runner->total_failed > 0) {
        printf("%sFailed: %d%s\\n", COLOR_RED, runner->total_failed, COLOR_RESET);
    }
    
    if (runner->total_skipped > 0) {
        printf("%sSkipped: %d%s\\n", COLOR_YELLOW, runner->total_skipped, COLOR_RESET);
    }
    
    if (runner->total_errors > 0) {
        printf("%sErrors: %d%s\\n", COLOR_MAGENTA, runner->total_errors, COLOR_RESET);
    }
    
    printf("Total Duration: %.2fms\\n", runner->total_duration_ms);
    
    double success_rate = runner->total_tests > 0 ? 
        (double)runner->total_passed / runner->total_tests * 100.0 : 0.0;
    
    printf("Success Rate: %.1f%%\\n\\n", success_rate);
    
    if (runner->total_failed == 0 && runner->total_errors == 0) {
        printf("%s✓ ALL TESTS PASSED%s\\n", COLOR_GREEN, COLOR_RESET);
    } else {
        printf("%s✗ SOME TESTS FAILED%s\\n", COLOR_RED, COLOR_RESET);
    }
}

// Assertion functions
static void set_test_failure(const char* message) {
    if (current_test) {
        current_test->status = TEST_FAIL;
        if (current_test->message) {
            free(current_test->message);
        }
        current_test->message = strdup(message);
    }
}

void test_assert_true(ember_vm* vm, int condition, const char* message) {
    (void)vm; // Unused parameter
    
    if (!condition) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected true, got false: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_false(ember_vm* vm, int condition, const char* message) {
    (void)vm; // Unused parameter
    
    if (condition) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected false, got true: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message) {
    (void)vm; // Unused parameter
    
    if (!values_equal(expected, actual)) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Values not equal: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_not_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message) {
    (void)vm; // Unused parameter
    
    if (values_equal(expected, actual)) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Values should not be equal: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_null(ember_vm* vm, ember_value value, const char* message) {
    (void)vm; // Unused parameter
    
    if (value.type != EMBER_VAL_NIL) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected nil value: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_not_null(ember_vm* vm, ember_value value, const char* message) {
    (void)vm; // Unused parameter
    
    if (value.type == EMBER_VAL_NIL) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected non-nil value: %s", 
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_string_equal(ember_vm* vm, const char* expected, const char* actual, const char* message) {
    (void)vm; // Unused parameter
    
    if (!expected && !actual) {
        return; // Both null, equal
    }
    
    if (!expected || !actual || strcmp(expected, actual) != 0) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Strings not equal. Expected: '%s', Got: '%s': %s", 
                 expected ? expected : "(null)", 
                 actual ? actual : "(null)",
                 message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_number_equal(ember_vm* vm, double expected, double actual, double tolerance, const char* message) {
    (void)vm; // Unused parameter
    
    if (fabs(expected - actual) > tolerance) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), 
                 "Numbers not equal within tolerance. Expected: %f, Got: %f, Tolerance: %f: %s", 
                 expected, actual, tolerance, message ? message : "");
        set_test_failure(full_message);
    }
}

void test_assert_array_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message) {
    (void)vm; // Unused parameter
    
    if (expected.type != EMBER_VAL_ARRAY || actual.type != EMBER_VAL_ARRAY) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected array values: %s", 
                 message ? message : "");
        set_test_failure(full_message);
        return;
    }
    
    ember_array* expected_arr = AS_ARRAY(expected);
    ember_array* actual_arr = AS_ARRAY(actual);
    
    if (expected_arr->length != actual_arr->length) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), 
                 "Array lengths differ. Expected: %d, Got: %d: %s", 
                 expected_arr->length, actual_arr->length, message ? message : "");
        set_test_failure(full_message);
        return;
    }
    
    for (int i = 0; i < expected_arr->length; i++) {
        if (!values_equal(expected_arr->elements[i], actual_arr->elements[i])) {
            char full_message[512];
            snprintf(full_message, sizeof(full_message), 
                     "Array elements differ at index %d: %s", 
                     i, message ? message : "");
            set_test_failure(full_message);
            return;
        }
    }
}

void test_assert_throws(ember_vm* vm, void (*func)(ember_vm*), const char* expected_error, const char* message) {
    if (!vm || !func) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Invalid parameters for test_assert_throws: %s", 
                 message ? message : "");
        set_test_failure(full_message);
        return;
    }
    
    // Clear any existing errors
    ember_vm_clear_error(vm);
    
    // Run the function
    func(vm);
    
    // Check if an error was thrown
    if (!ember_vm_has_error(vm)) {
        char full_message[512];
        snprintf(full_message, sizeof(full_message), "Expected exception but none was thrown: %s", 
                 message ? message : "");
        set_test_failure(full_message);
        return;
    }
    
    // Check error message if specified
    if (expected_error) {
        ember_error* error = ember_vm_get_error(vm);
        if (!error || !error->message || strstr(error->message, expected_error) == NULL) {
            char full_message[512];
            snprintf(full_message, sizeof(full_message), 
                     "Expected error containing '%s', got: '%s': %s", 
                     expected_error, 
                     error && error->message ? error->message : "(no error message)",
                     message ? message : "");
            set_test_failure(full_message);
        }
    }
    
    // Clear the error after checking
    ember_vm_clear_error(vm);
}

// Test utilities
void test_setup_vm(ember_vm* vm) {
    if (!vm) return;
    
    // Reset VM state for clean test environment
    vm->stack_top = 0;
    ember_vm_clear_error(vm);
    
    // Clear any pending exceptions
    vm->exception_pending = 0;
    vm->current_exception = ember_make_nil();
}

void test_cleanup_vm(ember_vm* vm) {
    if (!vm) return;
    
    // Reset VM state after test
    vm->stack_top = 0;
    ember_vm_clear_error(vm);
    vm->exception_pending = 0;
    vm->current_exception = ember_make_nil();
}

ember_value test_create_test_array(ember_vm* vm, int size) {
    ember_value array = ember_make_array(vm, size);
    if (array.type != EMBER_VAL_ARRAY) {
        return ember_make_nil();
    }
    
    ember_array* arr = AS_ARRAY(array);
    for (int i = 0; i < size; i++) {
        array_push(arr, ember_make_number(i));
    }
    
    return array;
}

ember_value test_create_test_map(ember_vm* vm) {
    ember_value map = ember_make_map(vm);
    if (map.type != EMBER_VAL_MAP) {
        return ember_make_nil();
    }
    
    ember_map* map_obj = AS_MAP(map);
    
    // Add some test key-value pairs
    map_set(map_obj, ember_make_string_gc(vm, "key1"), ember_make_number(1));
    map_set(map_obj, ember_make_string_gc(vm, "key2"), ember_make_number(2));
    map_set(map_obj, ember_make_string_gc(vm, "key3"), ember_make_string_gc(vm, "value3"));
    
    return map;
}

void test_benchmark(const char* name, void (*func)(ember_vm*), ember_vm* vm, int iterations) {
    if (!name || !func || !vm || iterations <= 0) return;
    
    printf("%sBenchmark: %s%s\\n", COLOR_YELLOW, name, COLOR_RESET);
    
    double total_time = 0.0;
    double min_time = INFINITY;
    double max_time = 0.0;
    
    for (int i = 0; i < iterations; i++) {
        test_setup_vm(vm);
        
        double start_time = get_time_ms();
        func(vm);
        double end_time = get_time_ms();
        
        double iteration_time = end_time - start_time;
        total_time += iteration_time;
        
        if (iteration_time < min_time) min_time = iteration_time;
        if (iteration_time > max_time) max_time = iteration_time;
        
        test_cleanup_vm(vm);
    }
    
    double avg_time = total_time / iterations;
    
    printf("  Iterations: %d\\n", iterations);
    printf("  Total Time: %.2fms\\n", total_time);
    printf("  Average: %.4fms\\n", avg_time);
    printf("  Min: %.4fms\\n", min_time);
    printf("  Max: %.4fms\\n", max_time);
    printf("  Ops/sec: %.0f\\n\\n", 1000.0 / avg_time);
}