#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "test_ember_internal.h"

// Test core parser functionality comprehensively

void test_basic_parsing(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing basic parsing functionality...\n");
    
    // Test empty input
    printf("  Testing empty input...\n");
    int result = ember_eval(vm, "");
    printf("    Empty input result: %d\n", result);
    
    // Test whitespace only
    printf("  Testing whitespace-only input...\n");
    result = ember_eval(vm, "   \t  \n  ");
    printf("    Whitespace result: %d\n", result);
    
    // Test comments
    printf("  Testing comments...\n");
    result = ember_eval(vm, "# This is a comment\n42");
    printf("    Comment result: %d\n", result);
    
    // Test single token
    printf("  Testing single tokens...\n");
    result = ember_eval(vm, "42");
    printf("    Single number result: %d\n", result);
    
    result = ember_eval(vm, "true");
    printf("    Single boolean result: %d\n", result);
    
    result = ember_eval(vm, "\"hello\"");
    printf("    Single string result: %d\n", result);
    
    printf("Basic parsing test completed\n");
    ember_free_vm(vm);
}

void test_token_recognition(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing token recognition...\n");
    
    // Test all basic operators
    const char* operator_tests[] = {
        "1 + 2",    // PLUS
        "5 - 3",    // MINUS
        "4 * 6",    // MULTIPLY
        "8 / 2",    // DIVIDE
        "9 % 4",    // MODULO
        "5 == 5",   // EQUAL_EQUAL
        "5 != 3",   // NOT_EQUAL
        "7 > 4",    // GREATER
        "7 >= 4",   // GREATER_EQUAL
        "3 < 7",    // LESS
        "3 <= 7",   // LESS_EQUAL
        "true && false", // AND
        "true || false", // OR
        "!true",    // NOT
        NULL
    };
    
    for (int i = 0; operator_tests[i] != NULL; i++) {
        printf("  Testing operator: %s\n", operator_tests[i]);
        int result = ember_eval(vm, operator_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    // Test punctuation
    const char* punctuation_tests[] = {
        "(42)",           // LPAREN, RPAREN
        "[1, 2, 3]",      // LBRACKET, RBRACKET, COMMA
        "{\"a\": 1}",     // LBRACE, RBRACE, COLON
        "x = 5",          // EQUAL
        NULL
    };
    
    for (int i = 0; punctuation_tests[i] != NULL; i++) {
        printf("  Testing punctuation: %s\n", punctuation_tests[i]);
        int result = ember_eval(vm, punctuation_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Token recognition test completed\n");
    ember_free_vm(vm);
}

void test_keyword_recognition(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing keyword recognition...\n");
    
    const char* keyword_tests[] = {
        // Control flow keywords
        "if true { x = 1 }",
        "if false { x = 1 } else { x = 2 }",
        "while false { x = 1 }",
        "for (i = 0; i < 1; i = i + 1) { x = i }",
        
        // Function keywords
        "fn test() { return 42 }",
        
        // Loop control keywords
        "i = 0\nwhile i < 5 { i = i + 1; if i == 3 { break } }",
        "i = 0\nwhile i < 5 { i = i + 1; if i == 3 { continue } }",
        
        // Boolean keywords
        "x = true",
        "y = false",
        
        // Exception keywords
        "try { x = 1 } catch { x = 2 }",
        "try { throw \"error\" } catch { x = 1 }",
        
        // Import keyword
        "import math",
        
        NULL
    };
    
    for (int i = 0; keyword_tests[i] != NULL; i++) {
        printf("  Testing keyword: %s\n", keyword_tests[i]);
        int result = ember_eval(vm, keyword_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Keyword recognition test completed\n");
    ember_free_vm(vm);
}

void test_number_parsing(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing number parsing...\n");
    
    const char* number_tests[] = {
        // Integer numbers
        "0",
        "1",
        "42",
        "123456",
        
        // Floating point numbers
        "3.14",
        "0.5",
        "123.456",
        "0.0",
        "1.0",
        
        // Edge cases
        "999999999",     // Large integer
        "0.000001",      // Small decimal
        "1234.5678",     // Mixed
        
        NULL
    };
    
    for (int i = 0; number_tests[i] != NULL; i++) {
        printf("  Testing number: %s\n", number_tests[i]);
        int result = ember_eval(vm, number_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Number parsing test completed\n");
    ember_free_vm(vm);
}

void test_string_parsing(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing string parsing...\n");
    
    const char* string_tests[] = {
        // Basic strings
        "\"hello\"",
        "\"world\"",
        "\"\"",              // Empty string
        "\" \"",             // Space string
        
        // Strings with special characters
        "\"hello world\"",   // With space
        "\"123\"",           // Numeric content
        "\"true\"",          // Boolean content
        "\"!@#$%^&*()\"",    // Special characters
        
        // Longer strings
        "\"This is a longer string for testing purposes\"",
        "\"Line 1\\nLine 2\"",  // With newline (if supported)
        
        NULL
    };
    
    for (int i = 0; string_tests[i] != NULL; i++) {
        printf("  Testing string: %s\n", string_tests[i]);
        int result = ember_eval(vm, string_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("String parsing test completed\n");
    ember_free_vm(vm);
}

void test_identifier_parsing(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing identifier parsing...\n");
    
    const char* identifier_tests[] = {
        // Simple identifiers
        "x = 42\nx",
        "name = \"John\"\nname",
        "counter = 0\ncounter",
        
        // Identifiers with different patterns
        "variable_name = 1\nvariable_name",
        "CamelCase = 2\nCamelCase",
        "snake_case = 3\nsnake_case",
        "mixedCase123 = 4\nmixedCase123",
        
        // Single letter identifiers
        "a = 1\na",
        "z = 26\nz",
        
        // Identifiers that look like keywords but aren't
        "iff = 1\niff",       // Not 'if'
        "whilee = 2\nwhilee", // Not 'while'
        "truee = 3\ntruee",   // Not 'true'
        
        NULL
    };
    
    for (int i = 0; identifier_tests[i] != NULL; i++) {
        printf("  Testing identifier: %s\n", identifier_tests[i]);
        int result = ember_eval(vm, identifier_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Identifier parsing test completed\n");
    ember_free_vm(vm);
}

void test_statement_separation(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing statement separation...\n");
    
    const char* separation_tests[] = {
        // Semicolon separation
        "x = 1; y = 2; z = 3",
        "a = 5; b = a * 2; result = a + b",
        
        // Newline separation
        "x = 1\ny = 2\nz = 3",
        "a = 5\nb = a * 2\nresult = a + b",
        
        // Mixed separation
        "x = 1; y = 2\nz = x + y",
        "a = 5\nb = a * 2; result = a + b",
        
        // Multiple separators
        "x = 1;; y = 2",      // Double semicolon
        "x = 1\n\ny = 2",     // Double newline
        "x = 1;\ny = 2",      // Semicolon then newline
        
        // Trailing separators
        "x = 1;",             // Trailing semicolon
        "x = 1\n",            // Trailing newline
        
        NULL
    };
    
    for (int i = 0; separation_tests[i] != NULL; i++) {
        printf("  Testing separation: %s\n", separation_tests[i]);
        int result = ember_eval(vm, separation_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Statement separation test completed\n");
    ember_free_vm(vm);
}

void test_nested_structures(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing nested structures parsing...\n");
    
    const char* nested_tests[] = {
        // Nested parentheses
        "((((1))))",
        "(1 + (2 * (3 - 4)))",
        "((2 + 3) * (4 + 5))",
        
        // Nested arrays
        "[[1, 2], [3, 4]]",
        "[[[1]], [[2, 3]]]",
        "[[1, [2, 3]], [4, [5, 6]]]",
        
        // Nested hash maps
        "{\"a\": {\"b\": 1}}",
        "{\"x\": {\"y\": {\"z\": 42}}}",
        "{\"data\": {\"numbers\": [1, 2, 3], \"name\": \"test\"}}",
        
        // Mixed nesting
        "{\"arrays\": [[1, 2], [3, 4]], \"number\": 42}",
        "[{\"name\": \"first\"}, {\"name\": \"second\"}]",
        
        NULL
    };
    
    for (int i = 0; nested_tests[i] != NULL; i++) {
        printf("  Testing nested: %s\n", nested_tests[i]);
        int result = ember_eval(vm, nested_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Nested structures test completed\n");
    ember_free_vm(vm);
}

void test_whitespace_handling(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing whitespace handling...\n");
    
    const char* whitespace_tests[] = {
        // Various whitespace patterns
        "  42  ",                    // Leading/trailing spaces
        "\t42\t",                    // Leading/trailing tabs
        "\n42\n",                    // Leading/trailing newlines
        "  \t\n42\n\t  ",           // Mixed whitespace
        
        // Whitespace in expressions
        "1 + 2",                     // Normal spacing
        "1+2",                       // No spacing
        "1  +  2",                   // Extra spacing
        "1\t+\t2",                   // Tab spacing
        
        // Whitespace in complex structures
        "[ 1 , 2 , 3 ]",            // Spaced array
        "{\n  \"key\" : \"value\"\n}",  // Formatted hash map
        "(\n  1 + 2\n)",            // Formatted parentheses
        
        // Whitespace in statements
        "x = 1 ; y = 2",            // Spaced statements
        "if  (  x  >  0  )  {  y = 1  }", // Spaced if statement
        
        NULL
    };
    
    for (int i = 0; whitespace_tests[i] != NULL; i++) {
        printf("  Testing whitespace: %s\n", whitespace_tests[i]);
        int result = ember_eval(vm, whitespace_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test  
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Whitespace handling test completed\n");
    ember_free_vm(vm);
}

void test_error_recovery(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing error recovery...\n");
    
    const char* error_tests[] = {
        // Syntax errors
        "1 +",                      // Incomplete expression
        "( 1 + 2",                  // Unmatched parenthesis
        "1 + 2 )",                  // Unmatched parenthesis
        "[1, 2",                    // Incomplete array
        "{\"key\":",                // Incomplete hash map
        
        // Invalid tokens
        "1 @ 2",                    // Invalid operator
        "1 + # 2",                  // Invalid in expression
        "$invalid",                 // Invalid identifier
        
        // Invalid keywords
        "iff x > 0",                // Misspelled keyword
        "wile x > 0",               // Misspelled keyword
        "funtion test()",           // Misspelled keyword
        
        // Invalid structure
        "if",                       // Incomplete if
        "while",                    // Incomplete while
        "for (",                    // Incomplete for
        "fn",                       // Incomplete function
        
        NULL
    };
    
    for (int i = 0; error_tests[i] != NULL; i++) {
        printf("  Testing error case: %s\n", error_tests[i]);
        int result = ember_eval(vm, error_tests[i]);
        printf("    Error result: %d (expected non-zero)\n", result);
        
        // Clear any error state
        ember_vm_clear_error(vm);
        
        // Test that parser can continue after error
        result = ember_eval(vm, "42");
        printf("    Recovery test result: %d\n", result);
    }
    
    printf("Error recovery test completed\n");
    ember_free_vm(vm);
}

void test_large_input_handling(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing large input handling...\n");
    
    // Test with long expression
    char long_expr[2000];
    strcpy(long_expr, "1");
    for (int i = 0; i < 100; i++) {
        strcat(long_expr, " + 1");
    }
    
    printf("  Testing long arithmetic expression...\n");
    int result = ember_eval(vm, long_expr);
    printf("    Long expression result: %d\n", result);
    
    // Test with deep nesting
    char nested_expr[1000];
    strcpy(nested_expr, "((((((((((42))))))))))");
    
    printf("  Testing deeply nested expression...\n");
    result = ember_eval(vm, nested_expr);
    printf("    Nested expression result: %d\n", result);
    
    // Test with many statements
    char many_stmts[1000];
    strcpy(many_stmts, "x = 1");
    for (int i = 1; i < 50; i++) {
        char stmt[20];
        sprintf(stmt, "; x = x + 1");
        strcat(many_stmts, stmt);
    }
    
    printf("  Testing many statements...\n");
    result = ember_eval(vm, many_stmts);
    printf("    Many statements result: %d\n", result);
    
    printf("Large input handling test completed\n");
    ember_free_vm(vm);
}

void test_parser_state_management(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing parser state management...\n");
    
    // Test that parser state is properly reset between evaluations
    printf("  Testing state reset...\n");
    
    int result1 = ember_eval(vm, "x = 42");
    printf("    First eval result: %d\n", result1);
    
    int result2 = ember_eval(vm, "x");
    printf("    Second eval result: %d\n", result2);
    
    // Test error state doesn't persist
    printf("  Testing error state isolation...\n");
    
    int error_result = ember_eval(vm, "1 +");  // Should cause error
    printf("    Error eval result: %d\n", error_result);
    
    int recovery_result = ember_eval(vm, "2 + 2");  // Should work
    printf("    Recovery eval result: %d\n", recovery_result);
    
    // Test with complex nested structures
    printf("  Testing complex parsing...\n");
    
    const char* complex_code = 
        "data = {\n"
        "  \"numbers\": [1, 2, 3, 4, 5],\n"
        "  \"strings\": [\"a\", \"b\", \"c\"],\n"
        "  \"nested\": {\n"
        "    \"inner\": [true, false]\n"
        "  }\n"
        "}\n"
        "result = data[\"numbers\"][0] + data[\"nested\"][\"inner\"][0]";
    
    int complex_result = ember_eval(vm, complex_code);
    printf("    Complex parsing result: %d\n", complex_result);
    
    printf("Parser state management test completed\n");
    ember_free_vm(vm);
}

void test_edge_cases(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing edge cases...\n");
    
    const char* edge_tests[] = {
        // Minimal valid programs
        "1",
        "true",
        "\"\"",
        "[]",
        "{}",
        "()",  // Should be error - empty parentheses
        
        // Boundary values
        "0",
        "999999999",
        "0.0",
        "0.000001",
        
        // Empty structures
        "x = []; x",
        "y = {}; y",
        
        // Single character tokens
        "a = 1; a",
        "_ = 2; _",  // If underscore is valid identifier
        
        // Maximum nesting (within reasonable limits)
        "[[[[1]]]]",
        "{{{{\"a\": 1}}}}",  // Nested hash maps with same key
        
        NULL
    };
    
    for (int i = 0; edge_tests[i] != NULL; i++) {
        printf("  Testing edge case: %s\n", edge_tests[i]);
        int result = ember_eval(vm, edge_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear any error state
        ember_vm_clear_error(vm);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Edge cases test completed\n");
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Parser Core Tests...\n");
    printf("==================================\n\n");
    
    test_basic_parsing();
    printf("\n");
    
    test_token_recognition();
    printf("\n");
    
    test_keyword_recognition();
    printf("\n");
    
    test_number_parsing();
    printf("\n");
    
    test_string_parsing();
    printf("\n");
    
    test_identifier_parsing();
    printf("\n");
    
    test_statement_separation();
    printf("\n");
    
    test_nested_structures();
    printf("\n");
    
    test_whitespace_handling();
    printf("\n");
    
    test_error_recovery();
    printf("\n");
    
    test_large_input_handling();
    printf("\n");
    
    test_parser_state_management();
    printf("\n");
    
    test_edge_cases();
    printf("\n");
    
    printf("==================================\n");
    printf("All parser core tests completed!\n");
    return 0;
}