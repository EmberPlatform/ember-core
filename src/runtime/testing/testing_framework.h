#ifndef EMBER_TESTING_FRAMEWORK_H
#define EMBER_TESTING_FRAMEWORK_H

#include "../../../include/ember.h"

#ifdef __cplusplus
extern "C" {
#endif

// Test result status
typedef enum {
    TEST_PASS,
    TEST_FAIL,
    TEST_SKIP,
    TEST_ERROR
} test_status;

// Test case structure
typedef struct {
    char* name;
    test_status status;
    char* message;
    double duration_ms;
    int line;
    const char* file;
} test_case;

// Test suite structure
typedef struct {
    char* name;
    test_case* tests;
    int test_count;
    int test_capacity;
    int passed;
    int failed;
    int skipped;
    int errors;
    double total_duration_ms;
} test_suite;

// Test runner context
typedef struct {
    test_suite* suites;
    int suite_count;
    int suite_capacity;
    int total_tests;
    int total_passed;
    int total_failed;
    int total_skipped;
    int total_errors;
    double total_duration_ms;
    int verbose;
    int stop_on_failure;
} test_runner;

// Test function type
typedef void (*test_function)(ember_vm* vm);

// Core testing framework functions
test_runner* test_runner_create(void);
void test_runner_free(test_runner* runner);
test_suite* test_runner_add_suite(test_runner* runner, const char* name);
void test_suite_add_test(test_suite* suite, const char* name, test_function func, ember_vm* vm);
void test_runner_run_all(test_runner* runner);
void test_runner_print_results(test_runner* runner);

// Assertion functions
void test_assert_true(ember_vm* vm, int condition, const char* message);
void test_assert_false(ember_vm* vm, int condition, const char* message);
void test_assert_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message);
void test_assert_not_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message);
void test_assert_null(ember_vm* vm, ember_value value, const char* message);
void test_assert_not_null(ember_vm* vm, ember_value value, const char* message);
void test_assert_string_equal(ember_vm* vm, const char* expected, const char* actual, const char* message);
void test_assert_number_equal(ember_vm* vm, double expected, double actual, double tolerance, const char* message);
void test_assert_array_equal(ember_vm* vm, ember_value expected, ember_value actual, const char* message);
void test_assert_throws(ember_vm* vm, void (*func)(ember_vm*), const char* expected_error, const char* message);

// Test utilities
void test_setup_vm(ember_vm* vm);
void test_cleanup_vm(ember_vm* vm);
ember_value test_create_test_array(ember_vm* vm, int size);
ember_value test_create_test_map(ember_vm* vm);
void test_benchmark(const char* name, void (*func)(ember_vm*), ember_vm* vm, int iterations);

// Macros for easier testing
#define TEST_ASSERT(condition, message) \
    test_assert_true(vm, (condition), (message))

#define TEST_ASSERT_FALSE(condition, message) \
    test_assert_false(vm, (condition), (message))

#define TEST_ASSERT_EQ(expected, actual, message) \
    test_assert_equal(vm, (expected), (actual), (message))

#define TEST_ASSERT_NEQ(expected, actual, message) \
    test_assert_not_equal(vm, (expected), (actual), (message))

#define TEST_ASSERT_NULL(value, message) \
    test_assert_null(vm, (value), (message))

#define TEST_ASSERT_NOT_NULL(value, message) \
    test_assert_not_null(vm, (value), (message))

#define TEST_ASSERT_STR_EQ(expected, actual, message) \
    test_assert_string_equal(vm, (expected), (actual), (message))

#define TEST_ASSERT_NUM_EQ(expected, actual, tolerance, message) \
    test_assert_number_equal(vm, (expected), (actual), (tolerance), (message))

#define TEST_ASSERT_THROWS(func, expected_error, message) \
    test_assert_throws(vm, (func), (expected_error), (message))

#define TEST_DEFINE(test_name) \
    void test_##test_name(ember_vm* vm)

#define TEST_SUITE(suite_name) \
    void run_##suite_name##_tests(test_runner* runner)

#ifdef __cplusplus
}
#endif

#endif // EMBER_TESTING_FRAMEWORK_H