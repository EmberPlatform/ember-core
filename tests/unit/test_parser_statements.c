#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include "test_ember_internal.h"

// Test statement parsing functionality comprehensively

void test_variable_declarations(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing variable declarations...\n");
    
    const char* variable_tests[] = {
        // Simple assignments
        "x = 42",
        "name = \"John\"",
        "flag = true",
        "pi = 3.14159",
        
        // Multiple assignments
        "a = 1\nb = 2\nc = 3",
        "x = 10; y = 20; z = 30",
        
        // Assignment with expressions
        "sum = 10 + 20",
        "product = 5 * 6",
        "comparison = 5 > 3",
        "logical = true && false",
        
        // Chained assignments (using previous variables)
        "first = 100",
        "second = first + 50",
        "third = first + second",
        
        // Array assignments
        "numbers = [1, 2, 3, 4, 5]",
        "words = [\"hello\", \"world\"]",
        "mixed = [1, \"two\", true]",
        
        // Hash map assignments
        "person = {\"name\": \"Alice\", \"age\": 30}",
        "config = {\"debug\": true, \"port\": 8080}",
        
        NULL
    };
    
    for (int i = 0; variable_tests[i] != NULL; i++) {
        printf("  Testing variable declaration: %s\n", variable_tests[i]);
        int result = ember_eval(vm, variable_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Variable declarations test completed\n");
    ember_free_vm(vm);
}

void test_assignment_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing assignment statements...\n");
    
    const char* assignment_tests[] = {
        // Basic variable assignments
        "x = 10",
        "x = x + 5",
        "x = x * 2",
        
        // Array element assignments
        "arr = [1, 2, 3]",
        "arr[0] = 100",
        "arr[1] = arr[0] + 10",
        "arr[2] = arr[0] + arr[1]",
        
        // String assignments
        "message = \"Hello\"",
        "message = message + \" World\"",
        
        // Boolean assignments
        "enabled = true",
        "enabled = !enabled",
        "enabled = enabled || false",
        
        // Complex assignments with expressions
        "result = (x > 0) ? x : -x",   // Note: May not work if ternary not implemented
        "counter = counter + 1",       // Increment pattern
        
        NULL
    };
    
    for (int i = 0; assignment_tests[i] != NULL; i++) {
        printf("  Testing assignment: %s\n", assignment_tests[i]);
        int result = ember_eval(vm, assignment_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Assignment statements test completed\n");
    ember_free_vm(vm);
}

void test_if_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing if statements...\n");
    
    const char* if_tests[] = {
        // Simple if statements
        "x = 10\nif x > 5 { result = \"big\" }",
        "x = 2\nif x > 5 { result = \"big\" }",
        
        // If with else
        "x = 10\nif x > 5 { result = \"big\" } else { result = \"small\" }",
        "x = 2\nif x > 5 { result = \"big\" } else { result = \"small\" }",
        
        // Nested if statements
        "x = 15\nif x > 10 { if x > 20 { result = \"very big\" } else { result = \"big\" } } else { result = \"small\" }",
        
        // If with complex conditions
        "a = 5\nb = 10\nif a > 3 && b < 15 { result = \"both conditions true\" }",
        "a = 5\nb = 20\nif a > 3 && b < 15 { result = \"both conditions true\" } else { result = \"condition failed\" }",
        
        // If with multiple statements in blocks
        "x = 8\nif x > 5 {\n  y = x * 2\n  z = y + 1\n  result = z\n}",
        
        // Boolean conditions
        "flag = true\nif flag { result = \"flag is true\" }",
        "flag = false\nif flag { result = \"flag is true\" } else { result = \"flag is false\" }",
        
        NULL
    };
    
    for (int i = 0; if_tests[i] != NULL; i++) {
        printf("  Testing if statement: %s\n", if_tests[i]);
        int result = ember_eval(vm, if_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("If statements test completed\n");
    ember_free_vm(vm);
}

void test_while_loops(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing while loops...\n");
    
    const char* while_tests[] = {
        // Simple while loop
        "counter = 0\nsum = 0\nwhile counter < 5 { sum = sum + counter; counter = counter + 1 }",
        
        // While loop with single expression body  
        "i = 0\nwhile i < 3 i = i + 1",
        
        // While loop with block body
        "i = 0\nresult = 0\nwhile i < 4 {\n  result = result + i\n  i = i + 1\n}",
        
        // Nested while loops
        "i = 0\ntotal = 0\nwhile i < 3 {\n  j = 0\n  while j < 2 {\n    total = total + 1\n    j = j + 1\n  }\n  i = i + 1\n}",
        
        // While loop with complex condition
        "x = 10\ny = 1\nwhile x > 0 && y < 100 {\n  y = y * 2\n  x = x - 1\n}",
        
        NULL
    };
    
    for (int i = 0; while_tests[i] != NULL; i++) {
        printf("  Testing while loop: %s\n", while_tests[i]);
        int result = ember_eval(vm, while_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("While loops test completed\n");
    ember_free_vm(vm);
}

void test_for_loops(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing for loops...\n");
    
    const char* for_tests[] = {
        // Basic for loop
        "sum = 0\nfor (i = 0; i < 5; i = i + 1) { sum = sum + i }",
        
        // For loop with different increment
        "product = 1\nfor (i = 1; i <= 4; i = i + 1) { product = product * i }",
        
        // For loop with complex condition
        "count = 0\nfor (i = 1; i < 100 && count < 5; i = i * 2) { count = count + 1 }",
        
        // Nested for loops
        "total = 0\nfor (i = 0; i < 3; i = i + 1) {\n  for (j = 0; j < 2; j = j + 1) {\n    total = total + 1\n  }\n}",
        
        // For loop with empty initialization
        "i = 0\nsum = 0\nfor (; i < 4; i = i + 1) { sum = sum + i }",
        
        NULL
    };
    
    for (int i = 0; for_tests[i] != NULL; i++) {
        printf("  Testing for loop: %s\n", for_tests[i]);
        int result = ember_eval(vm, for_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("For loops test completed\n");
    ember_free_vm(vm);
}

void test_break_continue(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing break and continue statements...\n");
    
    const char* break_continue_tests[] = {
        // Break in while loop
        "i = 0\nsum = 0\nwhile i < 10 {\n  if i == 5 { break }\n  sum = sum + i\n  i = i + 1\n}",
        
        // Continue in while loop
        "i = 0\nsum = 0\nwhile i < 5 {\n  i = i + 1\n  if i == 3 { continue }\n  sum = sum + i\n}",
        
        // Break in for loop
        "sum = 0\nfor (i = 0; i < 10; i = i + 1) {\n  if i == 4 { break }\n  sum = sum + i\n}",
        
        // Continue in for loop
        "sum = 0\nfor (i = 0; i < 5; i = i + 1) {\n  if i == 2 { continue }\n  sum = sum + i\n}",
        
        // Nested loops with break
        "found = false\nfor (i = 0; i < 3; i = i + 1) {\n  for (j = 0; j < 3; j = j + 1) {\n    if i * j == 2 {\n      found = true\n      break\n    }\n  }\n  if found { break }\n}",
        
        NULL
    };
    
    for (int i = 0; break_continue_tests[i] != NULL; i++) {
        printf("  Testing break/continue: %s\n", break_continue_tests[i]);
        int result = ember_eval(vm, break_continue_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Break and continue statements test completed\n");
    ember_free_vm(vm);
}

void test_function_definitions(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing function definitions...\n");
    
    const char* function_tests[] = {
        // Simple function definition
        "fn greet() {\n  return \"Hello\"\n}",
        
        // Function with parameters
        "fn add(a, b) {\n  return a + b\n}",
        
        // Function with multiple statements
        "fn factorial(n) {\n  if n <= 1 {\n    return 1\n  } else {\n    return n * factorial(n - 1)\n  }\n}",
        
        // Function with local variables
        "fn calculate(x) {\n  temp = x * 2\n  result = temp + 10\n  return result\n}",
        
        // Function that returns boolean
        "fn isEven(n) {\n  return n % 2 == 0\n}",
        
        // Function with array operations
        "fn sum_array(arr) {\n  total = 0\n  i = 0\n  while i < len(arr) {\n    total = total + arr[i]\n    i = i + 1\n  }\n  return total\n}",
        
        NULL
    };
    
    for (int i = 0; function_tests[i] != NULL; i++) {
        printf("  Testing function definition: %s\n", function_tests[i]);
        int result = ember_eval(vm, function_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Function definitions test completed\n");
    ember_free_vm(vm);
}

void test_return_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing return statements...\n");
    
    const char* return_tests[] = {
        // Function with simple return
        "fn getValue() {\n  return 42\n}\nresult = getValue()",
        
        // Function with conditional return
        "fn absolute(x) {\n  if x < 0 {\n    return -x\n  }\n  return x\n}\nresult = absolute(-5)",
        
        // Function with early return
        "fn findFirst(arr, target) {\n  i = 0\n  while i < len(arr) {\n    if arr[i] == target {\n      return i\n    }\n    i = i + 1\n  }\n  return -1\n}",
        
        // Function with expression return
        "fn square(x) {\n  return x * x\n}\nresult = square(5)",
        
        // Function with void return (no value)
        "fn doSomething() {\n  x = 10\n  return\n}\ndoSomething()",
        
        // Function with multiple return paths
        "fn classify(x) {\n  if x > 0 {\n    return \"positive\"\n  } else if x < 0 {\n    return \"negative\"\n  } else {\n    return \"zero\"\n  }\n}",
        
        NULL
    };
    
    for (int i = 0; return_tests[i] != NULL; i++) {
        printf("  Testing return statement: %s\n", return_tests[i]);
        int result = ember_eval(vm, return_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Return statements test completed\n");
    ember_free_vm(vm);
}

void test_try_catch_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing try-catch statements...\n");
    
    const char* exception_tests[] = {
        // Basic try-catch
        "try {\n  result = \"success\"\n} catch {\n  result = \"error\"\n}",
        
        // Try-catch with exception variable
        "try {\n  throw \"something went wrong\"\n} catch (e) {\n  result = e\n}",
        
        // Try-catch-finally
        "cleanup = false\ntry {\n  result = \"success\"\n} catch {\n  result = \"error\"\n} finally {\n  cleanup = true\n}",
        
        // Nested try-catch
        "try {\n  try {\n    throw \"inner error\"\n  } catch {\n    result = \"inner caught\"\n  }\n} catch {\n  result = \"outer caught\"\n}",
        
        // Try with throw
        "try {\n  x = 10\n  if x > 5 {\n    throw \"x is too big\"\n  }\n  result = \"ok\"\n} catch (e) {\n  result = e\n}",
        
        NULL
    };
    
    for (int i = 0; exception_tests[i] != NULL; i++) {
        printf("  Testing try-catch: %s\n", exception_tests[i]);
        int result = ember_eval(vm, exception_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Try-catch statements test completed\n");
    ember_free_vm(vm);
}

void test_import_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing import statements...\n");
    
    const char* import_tests[] = {
        // Basic import
        "import math",
        
        // Import with version
        "import string@latest",
        "import crypto@1.0",
        
        // Multiple imports
        "import io\nimport json",
        
        NULL
    };
    
    for (int i = 0; import_tests[i] != NULL; i++) {
        printf("  Testing import: %s\n", import_tests[i]);
        int result = ember_eval(vm, import_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Import statements test completed\n");
    ember_free_vm(vm);
}

void test_expression_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing expression statements...\n");
    
    const char* expr_stmt_tests[] = {
        // Standalone expressions
        "42",
        "\"hello world\"",
        "true",
        "5 + 3",
        "10 * 2",
        
        // Function calls as statements
        "print(\"Hello\")",
        "abs(-10)",
        "sqrt(16)",
        
        // Complex expressions as statements
        "(5 > 3) && (2 < 4)",
        "x = 10; x + 5",
        "arr = [1, 2, 3]; arr[1]",
        
        // Assignment expressions
        "x = 5",
        "y = x * 2",
        "z = x + y",
        
        NULL
    };
    
    for (int i = 0; expr_stmt_tests[i] != NULL; i++) {
        printf("  Testing expression statement: %s\n", expr_stmt_tests[i]);
        int result = ember_eval(vm, expr_stmt_tests[i]);
        printf("    Result: %d\n", result);
    }
    
    printf("Expression statements test completed\n");
    ember_free_vm(vm);
}

void test_block_statements(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing block statements...\n");
    
    const char* block_tests[] = {
        // Simple block
        "{\n  x = 10\n  y = 20\n  result = x + y\n}",
        
        // Nested blocks
        "{\n  x = 5\n  {\n    y = 10\n    z = x + y\n  }\n  result = z\n}",
        
        // Block with control flow
        "{\n  i = 0\n  sum = 0\n  while i < 5 {\n    sum = sum + i\n    i = i + 1\n  }\n  result = sum\n}",
        
        // Empty block
        "{}",
        
        // Block with function definition
        "{\n  fn helper(x) {\n    return x * 2\n  }\n  result = helper(5)\n}",
        
        NULL
    };
    
    for (int i = 0; block_tests[i] != NULL; i++) {
        printf("  Testing block statement: %s\n", block_tests[i]);
        int result = ember_eval(vm, block_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Block statements test completed\n");
    ember_free_vm(vm);
}

void test_statement_error_cases(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing statement error cases...\n");
    
    const char* error_tests[] = {
        // Invalid syntax
        "if x > 5",          // Missing braces
        "while",             // Incomplete while
        "for (;;",           // Incomplete for loop
        "fn",                // Incomplete function
        "return",            // Return outside function
        
        // Invalid control flow
        "break",             // Break outside loop
        "continue",          // Continue outside loop
        
        // Invalid assignments
        "42 = x",            // Invalid left-hand side
        "(x + y) = 10",      // Invalid left-hand side
        
        // Unmatched braces
        "if x > 5 {",        // Unmatched opening brace
        "if x > 5 }",        // Unmatched closing brace
        "{ x = 10",          // Unmatched opening brace
        
        // Invalid exception handling
        "catch",             // Catch without try
        "finally",           // Finally without try
        "throw",             // Throw without value
        
        NULL
    };
    
    for (int i = 0; error_tests[i] != NULL; i++) {
        printf("  Testing error case: %s\n", error_tests[i]);
        int result = ember_eval(vm, error_tests[i]);
        printf("    Error result: %d (expected non-zero)\n", result);
        // Clear any error state
        ember_vm_clear_error(vm);
    }
    
    printf("Statement error cases test completed\n");
    ember_free_vm(vm);
}

void test_complex_statement_combinations(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    printf("Testing complex statement combinations...\n");
    
    const char* complex_tests[] = {
        // Function with multiple control structures
        "fn fibonacci(n) {\n  if n <= 1 {\n    return n\n  }\n  a = 0\n  b = 1\n  for (i = 2; i <= n; i = i + 1) {\n    temp = a + b\n    a = b\n    b = temp\n  }\n  return b\n}\nresult = fibonacci(10)",
        
        // Nested loops with conditionals
        "matrix = [[1, 2, 3], [4, 5, 6], [7, 8, 9]]\nsum = 0\nfor (i = 0; i < 3; i = i + 1) {\n  for (j = 0; j < 3; j = j + 1) {\n    if matrix[i][j] % 2 == 0 {\n      sum = sum + matrix[i][j]\n    }\n  }\n}",
        
        // Exception handling with control flow
        "result = 0\ntry {\n  for (i = 0; i < 10; i = i + 1) {\n    if i == 5 {\n      throw \"error at 5\"\n    }\n    result = result + i\n  }\n} catch (e) {\n  result = -1\n}",
        
        // Function with array processing
        "fn processArray(arr) {\n  result = []\n  for (i = 0; i < len(arr); i = i + 1) {\n    item = arr[i]\n    if item > 0 {\n      result[len(result)] = item * 2\n    }\n  }\n  return result\n}\nprocessed = processArray([1, -2, 3, -4, 5])",
        
        NULL
    };
    
    for (int i = 0; complex_tests[i] != NULL; i++) {
        printf("  Testing complex combination %d\n", i + 1);
        int result = ember_eval(vm, complex_tests[i]);
        printf("    Result: %d\n", result);
        
        // Clear VM state for next test
        ember_free_vm(vm);
        vm = ember_new_vm();
    }
    
    printf("Complex statement combinations test completed\n");
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Parser Statement Tests...\n");
    printf("======================================\n\n");
    
    test_variable_declarations();
    printf("\n");
    
    test_assignment_statements();
    printf("\n");
    
    test_if_statements();
    printf("\n");
    
    test_while_loops();
    printf("\n");
    
    test_for_loops();
    printf("\n");
    
    test_break_continue();
    printf("\n");
    
    test_function_definitions();
    printf("\n");
    
    test_return_statements();
    printf("\n");
    
    test_try_catch_statements();
    printf("\n");
    
    test_import_statements();
    printf("\n");
    
    test_expression_statements();
    printf("\n");
    
    test_block_statements();
    printf("\n");
    
    test_complex_statement_combinations();
    printf("\n");
    
    test_statement_error_cases();
    printf("\n");
    
    printf("======================================\n");
    printf("All parser statement tests completed!\n");
    return 0;
}