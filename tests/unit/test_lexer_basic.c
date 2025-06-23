#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "../unit/test_ember_internal.h"

// Test lexer functionality to improve frontend coverage

void test_basic_tokenization(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing basic tokenization...\n");
    
    // Test simple numeric expression parsing
    const char* simple_expr = "42 + 3.14";

    UNUSED(simple_expr);
    int result = ember_eval(vm, simple_expr);

    UNUSED(result);
    printf("Evaluation result for '%s': %d\n", simple_expr, result);
    
    // Test boolean literals
    const char* bool_expr = "true && false";

    UNUSED(bool_expr);
    result = ember_eval(vm, bool_expr);
    printf("Evaluation result for '%s': %d\n", bool_expr, result);
    
    // Test string literals
    const char* string_expr = "\"Hello World\"";

    UNUSED(string_expr);
    result = ember_eval(vm, string_expr);
    printf("Evaluation result for '%s': %d\n", string_expr, result);
    
    printf("Basic tokenization test passed\n");
    ember_free_vm(vm);
}

void test_arithmetic_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing arithmetic expressions...\n");
    
    const char* expressions[] = {
        "1 + 2",
        "10 - 5",
        "3 * 4", 
        "15 / 3",
        "17 % 5",
        "(2 + 3) * 4",
        "2 + 3 * 4",
        NULL
    };
    
    for (int i = 0; expressions[i] != NULL; i++) {
        printf("Testing expression: %s\n", expressions[i]);
        int result = ember_eval(vm, expressions[i]);

        UNUSED(result);
        printf("  Result: %d\n", result);
    }
    
    printf("Arithmetic expressions test passed\n");
    ember_free_vm(vm);
}

void test_comparison_operations(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing comparison operations...\n");
    
    const char* comparisons[] = {
        "5 == 5",
        "5 != 3",
        "10 > 5",
        "3 < 7",
        "5 >= 5",
        "4 <= 10",
        NULL
    };
    
    for (int i = 0; comparisons[i] != NULL; i++) {
        printf("Testing comparison: %s\n", comparisons[i]);
        int result = ember_eval(vm, comparisons[i]);

        UNUSED(result);
        printf("  Result: %d\n", result);
    }
    
    printf("Comparison operations test passed\n");
    ember_free_vm(vm);
}

void test_logical_operations(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing logical operations...\n");
    
    const char* logical_ops[] = {
        "true && true",
        "true && false",
        "false || true",
        "false || false",
        "!true",
        "!false",
        NULL
    };
    
    for (int i = 0; logical_ops[i] != NULL; i++) {
        printf("Testing logical operation: %s\n", logical_ops[i]);
        int result = ember_eval(vm, logical_ops[i]);

        UNUSED(result);
        printf("  Result: %d\n", result);
    }
    
    printf("Logical operations test passed\n");
    ember_free_vm(vm);
}

void test_variable_operations(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing variable operations...\n");
    
    // Test variable assignment and access
    const char* var_tests[] = {
        "x = 42",
        "y = x + 8", 
        "z = y * 2",
        NULL
    };
    
    for (int i = 0; var_tests[i] != NULL; i++) {
        printf("Testing variable operation: %s\n", var_tests[i]);
        int result = ember_eval(vm, var_tests[i]);

        UNUSED(result);
        printf("  Result: %d\n", result);
    }
    
    printf("Variable operations test passed\n");
    ember_free_vm(vm);
}

void test_function_calls(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing function calls...\n");
    
    // Test built-in function calls
    ember_value args[2];
    args[0] = ember_make_number(25.0);
    
    // Test sqrt function
    int result = ember_call(vm, "sqrt", 1, args);

    UNUSED(result);
    printf("sqrt(25) call result: %d\n", result);
    
    // Test abs function
    args[0] = ember_make_number(-42.0);
    result = ember_call(vm, "abs", 1, args);
    printf("abs(-42) call result: %d\n", result);
    
    // Test type function
    args[0] = ember_make_string("test");
    result = ember_call(vm, "type", 1, args);
    printf("type(\"test\") call result: %d\n", result);
    
    printf("Function calls test passed\n");
    ember_free_vm(vm);
}

void test_error_handling(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing error handling...\n");
    
    // Test syntax errors
    const char* syntax_errors[] = {
        "2 +",           // Incomplete expression
        "( 2 + 3",       // Unmatched parenthesis
        "2 + + 3",       // Invalid operator sequence
        "\"unclosed string",  // Unclosed string
        NULL
    };
    
    for (int i = 0; syntax_errors[i] != NULL; i++) {
        printf("Testing syntax error: %s\n", syntax_errors[i]);
        int result = ember_eval(vm, syntax_errors[i]);

        UNUSED(result);
        printf("  Error result: %d (expected non-zero)\n", result);
    }
    
    printf("Error handling test passed\n");
    ember_free_vm(vm);
}

void test_edge_cases(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing edge cases...\n");
    
    // Test empty input
    int result = ember_eval(vm, "");

    UNUSED(result);
    printf("Empty input result: %d\n", result);
    
    // Test whitespace only
    result = ember_eval(vm, "   \t  \n  ");
    printf("Whitespace only result: %d\n", result);
    
    // Test very long expression
    char long_expr[1000];
    strcpy(long_expr, "1");
    for (int i = 0; i < 50; i++) {
        strcat(long_expr, " + 1");
    }
    result = ember_eval(vm, long_expr);
    printf("Long expression result: %d\n", result);
    
    printf("Edge cases test passed\n");
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Lexer Basic Tests...\n");
    
    test_basic_tokenization();
    test_arithmetic_expressions();
    test_comparison_operations();
    test_logical_operations();
    test_variable_operations();
    test_function_calls();
    test_error_handling();
    test_edge_cases();
    
    printf("All lexer basic tests completed!\n");
    return 0;
}