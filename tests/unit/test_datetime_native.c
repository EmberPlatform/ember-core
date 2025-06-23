#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <time.h>
#include <math.h>
#include "ember.h"

// Forward declarations for datetime functions
ember_value ember_native_datetime_now(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_timestamp(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_format(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_parse(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_add(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_diff(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_to_utc(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_from_utc(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_year(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_month(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_day(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_hour(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_minute(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_second(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_weekday(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_datetime_yearday(ember_vm* vm, int argc, ember_value* argv);

// Test result counter
static int tests_passed = 0;
 UNUSED(tests_passed);
static int tests_failed = 0;
 UNUSED(tests_failed);

// Helper macro for tests
#define TEST_ASSERT(condition, message) \
    do { \
        if (condition) { \
            printf("PASS: %s\n", message); \
            tests_passed++; \
        } else { \
            printf("FAIL: %s\n", message); \
            tests_failed++; \
        } \
    } while(0)

// Helper function to get double value from ember_value
static double get_number_value(ember_value val) {
    return (val.type == EMBER_VAL_NUMBER) ? val.as.number_val : 0.0;
}

// Helper function to get string value from ember_value
static const char* get_string_value(ember_value val) {
    if (val.type != EMBER_VAL_STRING) return NULL;
    // Use the proper API to get string value
    ember_string* str = AS_STRING(val);

    UNUSED(str);
    return str ? str->chars : NULL;
}

// Test basic datetime_now functionality
static void test_datetime_now(ember_vm* vm) {
    printf("\n=== Testing datetime_now ===\n");
    
    // Test with no arguments
    ember_value result = ember_native_datetime_now(vm, 0, NULL);

    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_now returns number");
    
    double timestamp = get_number_value(result);
    time_t current_time = time(NULL);
    double diff = fabs(timestamp - (double)current_time);
    TEST_ASSERT(diff < 2.0, "datetime_now returns current timestamp within 2 seconds");
    
    // Test with wrong argument count
    ember_value arg = ember_make_number(123);

    UNUSED(arg);
    result = ember_native_datetime_now(vm, 1, &arg);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_now with arguments returns nil");
}

// Test datetime_timestamp (should be same as datetime_now)
static void test_datetime_timestamp(ember_vm* vm) {
    printf("\n=== Testing datetime_timestamp ===\n");
    
    ember_value result = ember_native_datetime_timestamp(vm, 0, NULL);

    
    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_timestamp returns number");
    
    double timestamp = get_number_value(result);
    time_t current_time = time(NULL);
    double diff = fabs(timestamp - (double)current_time);
    TEST_ASSERT(diff < 2.0, "datetime_timestamp returns current timestamp within 2 seconds");
}

// Test datetime_format functionality
static void test_datetime_format(ember_vm* vm) {
    printf("\n=== Testing datetime_format ===\n");
    
    // Test known timestamp (Unix epoch: 1970-01-01 00:00:00 UTC)
    ember_value timestamp = ember_make_number(0.0);

    UNUSED(timestamp);
    ember_value result = ember_native_datetime_format(vm, 1, &timestamp);

    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_STRING, "datetime_format returns string");
    
    const char* formatted = get_string_value(result);

    
    UNUSED(formatted);
    TEST_ASSERT(strcmp(formatted, "1970-01-01T00:00:00Z") == 0, 
                "datetime_format with epoch returns correct ISO format");
    
    // Test with custom format
    ember_value args[2];
    args[0] = ember_make_number(0.0);
    args[1] = ember_make_string("%Y-%m-%d");
    result = ember_native_datetime_format(vm, 2, args);
    
    const char* custom_formatted = get_string_value(result);

    
    UNUSED(custom_formatted);
    TEST_ASSERT(strcmp(custom_formatted, "1970-01-01") == 0, 
                "datetime_format with custom format works");
    
    // Test with invalid timestamp (out of range)
    ember_value invalid_timestamp = ember_make_number(1e20);

    UNUSED(invalid_timestamp);
    result = ember_native_datetime_format(vm, 1, &invalid_timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_format with invalid timestamp returns nil");
    
    // Test with non-number argument
    ember_value non_number = ember_make_string("not a number");

    UNUSED(non_number);
    result = ember_native_datetime_format(vm, 1, &non_number);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_format with non-number returns nil");
}

// Test datetime_add functionality
static void test_datetime_add(ember_vm* vm) {
    printf("\n=== Testing datetime_add ===\n");
    
    // Test adding seconds to epoch
    ember_value args[2];
    args[0] = ember_make_number(0.0);    // Unix epoch
    args[1] = ember_make_number(3600.0); // Add 1 hour
    
    ember_value result = ember_native_datetime_add(vm, 2, args);

    
    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_add returns number");
    
    double new_timestamp = get_number_value(result);
    TEST_ASSERT(new_timestamp == 3600.0, "datetime_add adds seconds correctly");
    
    // Test with negative seconds (subtraction)
    args[1] = ember_make_number(-1800.0); // Subtract 30 minutes
    result = ember_native_datetime_add(vm, 2, args);
    new_timestamp = get_number_value(result);
    TEST_ASSERT(new_timestamp == -1800.0, "datetime_add with negative seconds works");
    
    // Test with invalid timestamp
    args[0] = ember_make_number(1e20);
    args[1] = ember_make_number(100.0);
    result = ember_native_datetime_add(vm, 2, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_add with invalid timestamp returns nil");
    
    // Test with wrong argument count
    result = ember_native_datetime_add(vm, 1, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_add with wrong arg count returns nil");
}

// Test datetime_diff functionality
static void test_datetime_diff(ember_vm* vm) {
    printf("\n=== Testing datetime_diff ===\n");
    
    ember_value args[2];
    args[0] = ember_make_number(3600.0); // 1 hour after epoch
    args[1] = ember_make_number(0.0);    // Unix epoch
    
    ember_value result = ember_native_datetime_diff(vm, 2, args);

    
    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_diff returns number");
    
    double diff = get_number_value(result);
    TEST_ASSERT(diff == 3600.0, "datetime_diff calculates difference correctly");
    
    // Test reverse order
    args[0] = ember_make_number(0.0);
    args[1] = ember_make_number(3600.0);
    result = ember_native_datetime_diff(vm, 2, args);
    diff = get_number_value(result);
    TEST_ASSERT(diff == -3600.0, "datetime_diff with reverse order returns negative");
    
    // Test with invalid timestamp
    args[0] = ember_make_number(1e20);
    args[1] = ember_make_number(0.0);
    result = ember_native_datetime_diff(vm, 2, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_diff with invalid timestamp returns nil");
}

// Test datetime component extraction functions
static void test_datetime_components(ember_vm* vm) {
    printf("\n=== Testing datetime component extraction ===\n");
    
    // Test with Unix epoch (1970-01-01 00:00:00 UTC = Thursday)
    ember_value timestamp = ember_make_number(0.0);

    UNUSED(timestamp);
    
    // Test year extraction
    ember_value result = ember_native_datetime_year(vm, 1, &timestamp);

    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_year returns number");
    TEST_ASSERT(get_number_value(result) == 1970.0, "datetime_year returns correct year for epoch");
    
    // Test month extraction
    result = ember_native_datetime_month(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_month returns number");
    TEST_ASSERT(get_number_value(result) == 1.0, "datetime_month returns correct month for epoch");
    
    // Test day extraction
    result = ember_native_datetime_day(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_day returns number");
    TEST_ASSERT(get_number_value(result) == 1.0, "datetime_day returns correct day for epoch");
    
    // Test hour extraction
    result = ember_native_datetime_hour(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_hour returns number");
    TEST_ASSERT(get_number_value(result) == 0.0, "datetime_hour returns correct hour for epoch");
    
    // Test minute extraction
    result = ember_native_datetime_minute(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_minute returns number");
    TEST_ASSERT(get_number_value(result) == 0.0, "datetime_minute returns correct minute for epoch");
    
    // Test second extraction
    result = ember_native_datetime_second(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_second returns number");
    TEST_ASSERT(get_number_value(result) == 0.0, "datetime_second returns correct second for epoch");
    
    // Test weekday extraction (Unix epoch was a Thursday = 4)
    result = ember_native_datetime_weekday(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_weekday returns number");
    TEST_ASSERT(get_number_value(result) == 4.0, "datetime_weekday returns correct weekday for epoch");
    
    // Test yearday extraction (January 1st = day 1)
    result = ember_native_datetime_yearday(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_yearday returns number");
    TEST_ASSERT(get_number_value(result) == 1.0, "datetime_yearday returns correct yearday for epoch");
    
    // Test with invalid timestamp
    ember_value invalid_timestamp = ember_make_number(1e20);

    UNUSED(invalid_timestamp);
    result = ember_native_datetime_year(vm, 1, &invalid_timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_year with invalid timestamp returns nil");
}

// Test timezone conversion functions
static void test_timezone_functions(ember_vm* vm) {
    printf("\n=== Testing timezone functions ===\n");
    
    ember_value timestamp = ember_make_number(0.0);

    
    UNUSED(timestamp); // Unix epoch
    
    // Test datetime_to_utc
    ember_value result = ember_native_datetime_to_utc(vm, 1, &timestamp);

    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_to_utc returns number");
    // Note: exact result depends on system timezone
    
    // Test datetime_from_utc
    result = ember_native_datetime_from_utc(vm, 1, &timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NUMBER, "datetime_from_utc returns number");
    
    // Test with invalid timestamp
    ember_value invalid_timestamp = ember_make_number(1e20);

    UNUSED(invalid_timestamp);
    result = ember_native_datetime_to_utc(vm, 1, &invalid_timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_to_utc with invalid timestamp returns nil");
    
    result = ember_native_datetime_from_utc(vm, 1, &invalid_timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "datetime_from_utc with invalid timestamp returns nil");
}

// Test error conditions and security measures
static void test_error_conditions(ember_vm* vm) {
    printf("\n=== Testing error conditions ===\n");
    
    // Test with extremely large timestamps (security check)
    ember_value huge_timestamp = ember_make_number(1e50);

    UNUSED(huge_timestamp);
    ember_value result = ember_native_datetime_format(vm, 1, &huge_timestamp);

    UNUSED(result);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "Extremely large timestamp rejected");
    
    // Test with extremely small timestamps
    ember_value tiny_timestamp = ember_make_number(-1e50);

    UNUSED(tiny_timestamp);
    result = ember_native_datetime_format(vm, 1, &tiny_timestamp);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "Extremely small timestamp rejected");
    
    // Test datetime_format with non-string format
    ember_value args[2];
    args[0] = ember_make_number(0.0);
    args[1] = ember_make_number(123.0); // Non-string format
    result = ember_native_datetime_format(vm, 2, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "Non-string format parameter rejected");
    
    // Test datetime_add with non-numbers
    args[0] = ember_make_string("not a number");
    args[1] = ember_make_number(100.0);
    result = ember_native_datetime_add(vm, 2, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "Non-number timestamp in datetime_add rejected");
    
    args[0] = ember_make_number(0.0);
    args[1] = ember_make_string("not a number");
    result = ember_native_datetime_add(vm, 2, args);
    TEST_ASSERT(result.type == EMBER_VAL_NIL, "Non-number seconds in datetime_add rejected");
}

// Test integration scenarios
static void test_integration_scenarios(ember_vm* vm) {
    printf("\n=== Testing integration scenarios ===\n");
    
    // Test: Get current time, format it, add time, format again
    ember_value now = ember_native_datetime_now(vm, 0, NULL);

    UNUSED(now);
    TEST_ASSERT(now.type == EMBER_VAL_NUMBER, "Get current timestamp");
    
    ember_value formatted = ember_native_datetime_format(vm, 1, &now);

    
    UNUSED(formatted);
    TEST_ASSERT(formatted.type == EMBER_VAL_STRING, "Format current timestamp");
    
    ember_value args[2];
    args[0] = now;
    args[1] = ember_make_number(3600.0); // Add 1 hour
    
    ember_value later = ember_native_datetime_add(vm, 2, args);

    
    UNUSED(later);
    TEST_ASSERT(later.type == EMBER_VAL_NUMBER, "Add time to current timestamp");
    
    double diff_result = get_number_value(ember_native_datetime_diff(vm, 2, args));
    TEST_ASSERT(fabs(diff_result - 3600.0) < 0.001, "Difference calculation matches added time");
    
    // Test: Extract components from known timestamp
    ember_value known_time = ember_make_number(1609459200.0);

    UNUSED(known_time); // 2021-01-01 00:00:00 UTC
    
    double year = get_number_value(ember_native_datetime_year(vm, 1, &known_time));
    double month = get_number_value(ember_native_datetime_month(vm, 1, &known_time));
    double day = get_number_value(ember_native_datetime_day(vm, 1, &known_time));
    
    TEST_ASSERT(year == 2021.0 && month == 1.0 && day == 1.0, 
                "Component extraction for known timestamp");
}

int main() {
    printf("Starting Ember DateTime Native Function Tests\n");
    printf("=============================================\n");
    
    // Initialize Ember VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        printf("ERROR: Failed to create Ember VM\n");
        return 1;
    }
    
    // Run all tests
    test_datetime_now(vm);
    test_datetime_timestamp(vm);
    test_datetime_format(vm);
    test_datetime_add(vm);
    test_datetime_diff(vm);
    test_datetime_components(vm);
    test_timezone_functions(vm);
    test_error_conditions(vm);
    test_integration_scenarios(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    // Print results
    printf("\n=============================================\n");
    printf("Test Results: %d passed, %d failed\n", tests_passed, tests_failed);
    
    if (tests_failed > 0) {
        printf("Some tests failed. Please check the implementation.\n");
        return 1;
    } else {
        printf("All tests passed!\n");
        return 0;
    }
}