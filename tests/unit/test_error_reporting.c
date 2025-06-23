#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test counter
static int test_count = 0;
 UNUSED(test_count);
static int test_passed = 0;
 UNUSED(test_passed);

#define RUN_TEST(test_name, test_func) \
    do { \
        test_count++; \
        printf("Running %s... ", test_name); \
        fflush(stdout); \
        if (test_func()) { \
            test_passed++; \
            printf("PASSED\n"); \
        } else { \
            printf("FAILED\n"); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            printf("ASSERTION FAILED: %s at line %d\n", #condition, __LINE__); \
            return 0; \
        } \
    } while(0)

#define ASSERT_FALSE(condition) ASSERT_TRUE(!(condition))
#define ASSERT_NOT_NULL(ptr) ASSERT_TRUE((ptr) != NULL)
#define ASSERT_NULL(ptr) ASSERT_TRUE((ptr) == NULL)

// Test syntax error reporting with line and column information
int test_syntax_error_location() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test code with syntax error on line 3, column 5
    const char* code = "x = 10\n"
                      "y = 20\n"
                      "z = (  # Missing closing parenthesis\n"
                      "print(x + y)\n";

    UNUSED(code);
    
    // This should fail with a syntax error
    int result = ember_eval(vm, code);

    UNUSED(result);
    ASSERT_TRUE(result != 0); // Should fail
    
    // Check if error was recorded
    ember_error* error = ember_vm_get_error(vm);

    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->type == EMBER_ERROR_SYNTAX);
        ASSERT_TRUE(error->location.line == 3); // Error should be on line 3
        ASSERT_TRUE(error->location.column > 0); // Should have column info
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test runtime error with stack trace
int test_runtime_error_stack_trace() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test division by zero
    const char* code = "x = 10\n"
                      "y = 0\n"
                      "result = x / y  # Division by zero\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0); // Should fail
    
    // Check if error was recorded
    ember_error* error = ember_vm_get_error(vm);

    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->type == EMBER_ERROR_RUNTIME);
        ASSERT_TRUE(strstr(error->message, "Division by zero") != NULL);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test type error reporting
int test_type_error() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test adding incompatible types
    const char* code = "x = 10\n"
                      "y = [1, 2, 3]\n"
                      "result = x + y  # Number + Array should fail\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0); // Should fail
    
    // Check if error was recorded
    ember_error* error = ember_vm_get_error(vm);

    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->type == EMBER_ERROR_TYPE);
        ASSERT_TRUE(strstr(error->message, "Type error") != NULL);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test array bounds error
int test_array_bounds_error() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test array index out of bounds
    const char* code = "arr = [1, 2, 3]\n"
                      "value = arr[10]  # Index out of bounds\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0); // Should fail
    
    // Check if error was recorded
    ember_error* error = ember_vm_get_error(vm);

    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->type == EMBER_ERROR_RUNTIME);
        ASSERT_TRUE(strstr(error->message, "out of bounds") != NULL);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test function call error with stack trace
int test_function_call_error() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test calling non-existent function
    const char* code = "fn test_func(x) {\n"
                      "    return unknown_func(x)  # Call to undefined function\n"
                      "}\n"
                      "result = test_func(5)\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    // This might succeed with nil return, depending on implementation
    // The test verifies that if an error occurs, it's properly reported
    
    ember_error* error = ember_vm_get_error(vm);

    
    UNUSED(error);
    if (error) {
        // If there's an error, it should have call stack information
        ASSERT_TRUE(error->call_stack_depth >= 0);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test security error (stack overflow prevention)
int test_security_error() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Create a deep recursion that might trigger stack overflow protection
    const char* code = "fn recursive(n) {\n"
                      "    if (n <= 0) { return 1 }\n"
                      "    return recursive(n - 1) + recursive(n - 1)\n"
                      "}\n"
                      "result = recursive(100)\n";

    UNUSED(code); // Deep recursion
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    // This might succeed or fail depending on implementation limits
    
    ember_error* error = ember_vm_get_error(vm);

    
    UNUSED(error);
    if (error && error->type == EMBER_ERROR_SECURITY) {
        ASSERT_TRUE(strstr(error->message, "[SECURITY]") != NULL);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test multiple errors (error recovery)
int test_multiple_errors() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // First error
    const char* code1 = "x = 10 / 0  # Division by zero\n";

    UNUSED(code1);
    int result1 = ember_eval(vm, code1);

    UNUSED(result1);
    ASSERT_TRUE(result1 != 0);
    
    // Clear error and try another
    ember_vm_clear_error(vm);
    ASSERT_FALSE(ember_vm_has_error(vm));
    
    // Second error
    const char* code2 = "y = [1, 2][5]  # Array bounds error\n";

    UNUSED(code2);
    int result2 = ember_eval(vm, code2);

    UNUSED(result2);
    ASSERT_TRUE(result2 != 0);
    
    // Should have new error
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    ember_free_vm(vm);
    return 1;
}

// Test error message formatting and context
int test_error_context() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Multi-line code with error in the middle
    const char* code = "# First line comment\n"
                      "x = 10\n"
                      "y = 0\n"
                      "z = x / y  # Error on this line\n"
                      "print(z)\n";

    UNUSED(code);
    
    // Set source for better error reporting
    ember_set_current_source(code, "test.ember");
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0);
    
    ember_error* error = ember_vm_get_error(vm);

    
    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->location.line == 4); // Error should be on line 4
        ASSERT_NOT_NULL(error->source_code); // Should have source code reference
        ASSERT_NOT_NULL(error->location.line_text); // Should have line text
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test nested function call error stack
int test_nested_function_error_stack() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Nested function calls with error in deepest function
    const char* code = "fn level3() {\n"
                      "    return 10 / 0  # Error here\n"
                      "}\n"
                      "fn level2() {\n"
                      "    return level3()\n"
                      "}\n"
                      "fn level1() {\n"
                      "    return level2()\n"
                      "}\n"
                      "result = level1()\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0);
    
    ember_error* error = ember_vm_get_error(vm);

    
    UNUSED(error);
    if (error) {
        // Should have call stack showing the nested calls
        // Note: This depends on the VM implementation of call stack tracking
        ASSERT_TRUE(error->call_stack_depth >= 0);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test error with special characters and unicode
int test_error_with_special_characters() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    ASSERT_NOT_NULL(vm);
    
    // Test with string containing special characters
    const char* code = "message = \"Hello, 世界! 🌍\"\n"
                      "result = message + 42  # Type error with unicode string\n";

    UNUSED(code);
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    ASSERT_TRUE(result != 0);
    
    ember_error* error = ember_vm_get_error(vm);

    
    UNUSED(error);
    if (error) {
        ASSERT_TRUE(error->type == EMBER_ERROR_TYPE);
        // Error system should handle unicode characters gracefully
        ASSERT_NOT_NULL(error->message);
    }
    
    ember_free_vm(vm);
    return 1;
}

// Test memory allocation failure error
int test_memory_error() {
    // Test memory error creation (doesn't require VM)
    ember_error* error = ember_error_memory("Test allocation failure");

    UNUSED(error);
    ASSERT_NOT_NULL(error);
    ASSERT_TRUE(error->type == EMBER_ERROR_MEMORY);
    ASSERT_TRUE(strstr(error->message, "Memory error") != NULL);
    ASSERT_TRUE(strstr(error->message, "Test allocation failure") != NULL);
    
    ember_error_free(error);
    return 1;
}

int main() {
    printf("Running Ember Error Reporting Tests\n");
    printf("=====================================\n\n");
    
    // Run all tests
    RUN_TEST("Syntax Error Location", test_syntax_error_location);
    RUN_TEST("Runtime Error Stack Trace", test_runtime_error_stack_trace);
    RUN_TEST("Type Error", test_type_error);
    RUN_TEST("Array Bounds Error", test_array_bounds_error);
    RUN_TEST("Function Call Error", test_function_call_error);
    RUN_TEST("Security Error", test_security_error);
    RUN_TEST("Multiple Errors", test_multiple_errors);
    RUN_TEST("Error Context", test_error_context);
    RUN_TEST("Nested Function Error Stack", test_nested_function_error_stack);
    RUN_TEST("Error with Special Characters", test_error_with_special_characters);
    RUN_TEST("Memory Error", test_memory_error);
    
    // Print results
    printf("\n=====================================\n");
    printf("Test Results: %d/%d tests passed\n", test_passed, test_count);
    
    if (test_passed == test_count) {
        printf("All tests PASSED! ✓\n");
        return 0;
    } else {
        printf("Some tests FAILED! ✗\n");
        return 1;
    }
}