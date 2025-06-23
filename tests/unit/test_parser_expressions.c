#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <math.h>
#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))

// Test expression parsing functionality comprehensively

void test_basic_literals(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing basic literals...\n");
    
    // Test number literals
    const char* number_tests[] = {
        "42",
        "3.14",
        "0",
        "123.456",
        "-42",
        "-3.14",
        NULL
    };
    
    for (int i = 0; number_tests[i] != NULL; i++) {
        printf("  Testing number literal: %s\n", number_tests[i]);
        int result = ember_eval(vm, number_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    // Test string literals
    const char* string_tests[] = {
        "\"hello\"",
        "\"world\"",
        "\"\"",  // empty string
        "\"hello world\"",
        "\"123\"",
        NULL
    };
    
    for (int i = 0; string_tests[i] != NULL; i++) {
        printf("  Testing string literal: %s\n", string_tests[i]);
        int result = ember_eval(vm, string_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    // Test boolean literals
    const char* bool_tests[] = {
        "true",
        "false",
        NULL
    };
    
    for (int i = 0; bool_tests[i] != NULL; i++) {
        printf("  Testing boolean literal: %s\n", bool_tests[i]);
        int result = ember_eval(vm, bool_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Basic literals test completed\n");
    ember_free_vm(vm);
}

void test_arithmetic_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing arithmetic expressions...\n");
    
    const char* arithmetic_tests[] = {
        "1 + 2",
        "10 - 5",
        "3 * 4",
        "15 / 3",
        "17 % 5",
        
        // Precedence tests
        "2 + 3 * 4",      // Should be 14, not 20
        "(2 + 3) * 4",    // Should be 20
        "10 / 2 + 3",     // Should be 8
        "10 / (2 + 3)",   // Should be 2
        
        // Complex expressions
        "1 + 2 * 3 - 4",  // Should be 3
        "(1 + 2) * (3 - 4)",  // Should be -3
        "2 * 3 + 4 * 5",  // Should be 26
        
        // Floating point
        "3.14 + 2.86",
        "10.5 - 3.2",
        "2.5 * 4.0",
        "12.6 / 3.0",
        
        NULL
    };
    
    for (int i = 0; arithmetic_tests[i] != NULL; i++) {
        printf("  Testing arithmetic: %s\n", arithmetic_tests[i]);
        int result = ember_eval(vm, arithmetic_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Arithmetic expressions test completed\n");
    ember_free_vm(vm);
}

void test_comparison_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing comparison expressions...\n");
    
    const char* comparison_tests[] = {
        "5 == 5",
        "5 == 3",
        "5 != 3",
        "5 != 5",
        "10 > 5",
        "3 > 7",
        "7 < 10",
        "10 < 3",
        "5 >= 5",
        "5 >= 3",
        "3 >= 5",
        "5 <= 5",
        "3 <= 5",
        "5 <= 3",
        
        // String comparisons
        "\"hello\" == \"hello\"",
        "\"hello\" == \"world\"",
        "\"hello\" != \"world\"",
        
        // Boolean comparisons
        "true == true",
        "true == false",
        "false != true",
        
        // Mixed type comparisons
        "5 == \"5\"",    // Should be false (different types)
        "true == 1",    // Should be false (different types)
        
        NULL
    };
    
    for (int i = 0; comparison_tests[i] != NULL; i++) {
        printf("  Testing comparison: %s\n", comparison_tests[i]);
        int result = ember_eval(vm, comparison_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Comparison expressions test completed\n");
    ember_free_vm(vm);
}

void test_logical_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing logical expressions...\n");
    
    const char* logical_tests[] = {
        "true && true",
        "true && false",
        "false && true",
        "false && false",
        
        "true || true",
        "true || false",
        "false || true",
        "false || false",
        
        "!true",
        "!false",
        "!!true",
        "!!false",
        
        // Short-circuit evaluation tests
        "true || false",   // Should not evaluate second operand
        "false && true",   // Should not evaluate second operand
        
        // Complex logical expressions
        "true && (false || true)",
        "(true || false) && false",
        "!true || !false",
        "!(true && false)",
        
        // Logical with comparisons
        "5 > 3 && 2 < 10",
        "5 < 3 || 2 > 10",
        "!(5 == 3)",
        
        NULL
    };
    
    for (int i = 0; logical_tests[i] != NULL; i++) {
        printf("  Testing logical: %s\n", logical_tests[i]);
        int result = ember_eval(vm, logical_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Logical expressions test completed\n");
    ember_free_vm(vm);
}

void test_unary_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing unary expressions...\n");
    
    const char* unary_tests[] = {
        "-42",
        "-(-42)",
        "-(3 + 4)",
        "-3.14",
        
        "!true",
        "!false",
        "!(5 > 3)",
        "!(true && false)",
        
        // Multiple unary operators
        "--42",      // Should be 42
        "!!true",    // Should be true
        "!!(5 > 3)", // Should be true
        
        NULL
    };
    
    for (int i = 0; unary_tests[i] != NULL; i++) {
        printf("  Testing unary: %s\n", unary_tests[i]);
        int result = ember_eval(vm, unary_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Unary expressions test completed\n");
    ember_free_vm(vm);
}

void test_grouping_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing grouping expressions...\n");
    
    const char* grouping_tests[] = {
        "(42)",
        "(3 + 4)",
        "((5))",
        "(2 * (3 + 4))",
        "((2 + 3) * (4 - 1))",
        
        // Nested grouping
        "(((1 + 2) * 3) - 4)",
        "((true && false) || true)",
        "(!(5 > 3))",
        
        // Complex nested expressions
        "((2 + 3) * 4) / ((10 - 5) + 1)",
        "((true || false) && (5 > 3))",
        
        NULL
    };
    
    for (int i = 0; grouping_tests[i] != NULL; i++) {
        printf("  Testing grouping: %s\n", grouping_tests[i]);
        int result = ember_eval(vm, grouping_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Grouping expressions test completed\n");
    ember_free_vm(vm);
}

void test_variable_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing variable expressions...\n");
    
    // Test variable assignment and access
    const char* variable_tests[] = {
        "x = 42",      // Assignment
        "x",           // Access
        "y = x + 10",  // Assignment using variable
        "y",           // Access
        "z = x * y",   // Assignment using multiple variables
        "z",           // Access
        
        // String variables
        "name = \"hello\"",
        "name",
        "greeting = name + \" world\"",
        "greeting",
        
        // Boolean variables
        "flag = true",
        "flag",
        "result = flag && (x > 0)",
        "result",
        
        NULL
    };
    
    for (int i = 0; variable_tests[i] != NULL; i++) {
        printf("  Testing variable: %s\n", variable_tests[i]);
        int result = ember_eval(vm, variable_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Variable expressions test completed\n");
    ember_free_vm(vm);
}

void test_array_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing array expressions...\n");
    
    const char* array_tests[] = {
        "[]",                    // Empty array
        "[1, 2, 3]",            // Number array
        "[\"a\", \"b\", \"c\"]", // String array
        "[true, false]",        // Boolean array
        "[1, \"hello\", true]", // Mixed array
        
        // Nested arrays
        "[[1, 2], [3, 4]]",
        "[[], [1], [1, 2]]",
        
        // Array with expressions
        "[1 + 2, 3 * 4, 5 - 1]",
        "[x = 10, x + 5, x * 2]",
        
        NULL
    };
    
    for (int i = 0; array_tests[i] != NULL; i++) {
        printf("  Testing array: %s\n", array_tests[i]);
        int result = ember_eval(vm, array_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    // Test array indexing
    const char* indexing_tests[] = {
        "arr = [10, 20, 30]",
        "arr[0]",
        "arr[1]",
        "arr[2]",
        "arr[0] = 100",
        "arr[0]",
        
        // String array indexing
        "words = [\"hello\", \"world\"]",
        "words[0]",
        "words[1]",
        
        NULL
    };
    
    for (int i = 0; indexing_tests[i] != NULL; i++) {
        printf("  Testing array indexing: %s\n", indexing_tests[i]);
        int result = ember_eval(vm, indexing_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Array expressions test completed\n");
    ember_free_vm(vm);
}

void test_hash_map_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing hash map expressions...\n");
    
    const char* hash_map_tests[] = {
        "{}",                           // Empty hash map
        "{\"name\": \"John\"}",         // Single key-value pair
        "{\"age\": 25, \"height\": 180}", // Multiple pairs
        "{1: \"one\", 2: \"two\"}",     // Number keys
        
        // Nested hash maps
        "{\"person\": {\"name\": \"John\", \"age\": 25}}",
        
        // Hash map with expressions
        "{\"sum\": 1 + 2, \"product\": 3 * 4}",
        "{\"x\": x = 10, \"y\": x + 5}",
        
        NULL
    };
    
    for (int i = 0; hash_map_tests[i] != NULL; i++) {
        printf("  Testing hash map: %s\n", hash_map_tests[i]);
        int result = ember_eval(vm, hash_map_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Hash map expressions test completed\n");
    ember_free_vm(vm);
}

void test_function_call_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing function call expressions...\n");
    
    // Test built-in function calls
    ember_value args[3];
    
    // Test mathematical functions
    args[0] = ember_make_number(25.0);
    int result = ember_call(vm, "sqrt", 1, args);

    UNUSED(result);
    printf("  sqrt(25) call result: %d\n", result);
    
    args[0] = ember_make_number(-42.0);
    result = ember_call(vm, "abs", 1, args);
    printf("  abs(-42) call result: %d\n", result);
    
    args[0] = ember_make_number(3.7);
    result = ember_call(vm, "floor", 1, args);
    printf("  floor(3.7) call result: %d\n", result);
    
    args[0] = ember_make_number(3.2);
    result = ember_call(vm, "ceil", 1, args);
    printf("  ceil(3.2) call result: %d\n", result);
    
    args[0] = ember_make_number(2.0);
    args[1] = ember_make_number(3.0);
    result = ember_call(vm, "pow", 2, args);
    printf("  pow(2, 3) call result: %d\n", result);
    
    // Test type function
    args[0] = ember_make_string("test");
    result = ember_call(vm, "type", 1, args);
    printf("  type(\"test\") call result: %d\n", result);
    
    args[0] = ember_make_number(42.0);
    result = ember_call(vm, "type", 1, args);
    printf("  type(42) call result: %d\n", result);
    
    // Test print function
    args[0] = ember_make_string("Hello from function call test");
    result = ember_call(vm, "print", 1, args);
    printf("  print call result: %d\n", result);
    
    printf("Function call expressions test completed\n");
    ember_free_vm(vm);
}

void test_complex_expressions(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing complex expressions...\n");
    
    const char* complex_tests[] = {
        // Mixed arithmetic and logical
        "(5 > 3) && (2 + 2 == 4)",
        "(10 / 2) > 3 || (5 * 2) < 9",
        "!(5 < 3) && (true || false)",
        
        // Variables with complex expressions
        "result = (x = 10, y = 20, x + y * 2)",
        "result",
        
        // Arrays with complex expressions
        "data = [1 + 1, 2 * 3, 4 - 1, 5 / 1]",
        "data[0] + data[1] * data[2] - data[3]",
        
        // Nested operations
        "((3 + 4) * (5 - 2)) / ((6 + 1) - (2 * 2))",
        
        // String operations with variables
        "first = \"Hello\"",
        "second = \"World\"",
        "combined = first + \" \" + second",
        "combined",
        
        NULL
    };
    
    for (int i = 0; complex_tests[i] != NULL; i++) {
        printf("  Testing complex: %s\n", complex_tests[i]);
        int result = ember_eval(vm, complex_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Complex expressions test completed\n");
    ember_free_vm(vm);
}

void test_expression_error_cases(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing expression error cases...\n");
    
    const char* error_tests[] = {
        // Syntax errors
        "2 +",           // Incomplete expression
        "* 3",           // Missing left operand
        "2 + + 3",       // Invalid operator sequence
        "( 2 + 3",       // Unmatched parenthesis
        "2 + 3 )",       // Unmatched parenthesis
        "((2 + 3)",      // Unmatched nested parenthesis
        
        // Invalid tokens
        "2 @ 3",         // Invalid operator
        "2 + # 3",       // Invalid operator
        
        // String errors
        "\"unclosed string",  // Unclosed string
        "'single quotes'",    // Wrong quote type
        
        // Array errors
        "[1, 2,",        // Incomplete array
        "[1 2]",         // Missing comma
        "arr[",          // Incomplete indexing
        
        // Hash map errors
        "{\"key\":",     // Incomplete hash map
        "{key: value}",  // Unquoted key
        "{\"key\" \"value\"}", // Missing colon
        
        NULL
    };
    
    for (int i = 0; error_tests[i] != NULL; i++) {
        printf("  Testing error case: %s\n", error_tests[i]);
        int result = ember_eval(vm, error_tests[i]);

        UNUSED(result);
        printf("    Error result: %d (expected non-zero)\n", result);
        // Clear any error state
        ember_vm_clear_error(vm);
    }
    
    printf("Expression error cases test completed\n");
    ember_free_vm(vm);
}

void test_operator_precedence(void) {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    printf("Testing operator precedence...\n");
    
    const char* precedence_tests[] = {
        // Arithmetic precedence
        "2 + 3 * 4",      // Should be 14 (3*4 first)
        "2 * 3 + 4",      // Should be 10 (2*3 first)
        "10 - 3 * 2",     // Should be 4 (3*2 first)
        "10 / 2 + 3",     // Should be 8 (10/2 first)
        "2 + 3 * 4 - 1",  // Should be 13
        
        // Comparison and logical precedence
        "5 > 3 && 2 < 4",      // Should be true
        "5 < 3 || 2 > 1",      // Should be true
        "!true || false",       // Should be false
        "!(true || false)",     // Should be false
        
        // Mixed precedence
        "2 + 3 > 4",           // Should be true (2+3=5, 5>4)
        "2 * 3 == 6",          // Should be true
        "5 > 3 && 2 + 2 == 4", // Should be true
        
        // Unary precedence
        "-2 + 3",              // Should be 1
        "-(2 + 3)",            // Should be -5
        "!true && false",      // Should be false
        "!(true && false)",    // Should be true
        
        NULL
    };
    
    for (int i = 0; precedence_tests[i] != NULL; i++) {
        printf("  Testing precedence: %s\n", precedence_tests[i]);
        int result = ember_eval(vm, precedence_tests[i]);

        UNUSED(result);
        printf("    Result: %d\n", result);
    }
    
    printf("Operator precedence test completed\n");
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Parser Expression Tests...\n");
    printf("========================================\n\n");
    
    test_basic_literals();
    printf("\n");
    
    test_arithmetic_expressions();
    printf("\n");
    
    test_comparison_expressions();
    printf("\n");
    
    test_logical_expressions();
    printf("\n");
    
    test_unary_expressions();
    printf("\n");
    
    test_grouping_expressions();
    printf("\n");
    
    test_variable_expressions();
    printf("\n");
    
    test_array_expressions();
    printf("\n");
    
    test_hash_map_expressions();
    printf("\n");
    
    test_function_call_expressions();
    printf("\n");
    
    test_complex_expressions();
    printf("\n");
    
    test_operator_precedence();
    printf("\n");
    
    test_expression_error_cases();
    printf("\n");
    
    printf("========================================\n");
    printf("All parser expression tests completed!\n");
    return 0;
}