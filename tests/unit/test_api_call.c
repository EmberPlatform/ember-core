#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing ember_call() with user-defined functions...\n");
    
    // Create VM
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Define a test Ember script with user-defined functions
    const char* script = 
        "fn add(a, b) {\n"
        "    return a + b;\n"
        "}\n"
        "\n"
        "fn multiply(x, y) {\n"
        "    return x * y;\n"
        "}\n"
        "\n"
        "fn greet(name) {\n"
        "    return \"Hello, \" + name + \"!\";\n"
        "}\n"
        "\n"
        "fn no_params() {\n"
        "    return 42;\n"
        "}\n";
    
    // Execute the script to define functions
    printf("Defining functions...\n");
    int result = ember_eval(vm, script);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    // Test 1: Call function with two numeric arguments
    printf("\nTest 1: Calling add(5, 3)...\n");
    ember_value args1[2];
    args1[0] = ember_make_number(5.0);
    args1[1] = ember_make_number(3.0);
    
    result = ember_call(vm, "add", 2, args1);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        
        if (result_val.type == EMBER_VAL_NUMBER && result_val.as.number_val == 8.0) {
            printf("✓ Test 1 PASSED\n");
        } else {
            printf("✗ Test 1 FAILED - Expected 8, got different value\n");
        }
    } else {
        printf("✗ Test 1 FAILED - Function call returned error: %d\n", result);
    }
    
    // Clear stack for next test
    if (vm->stack_top > 0) pop(vm);
    
    // Test 2: Call function with multiplication
    printf("\nTest 2: Calling multiply(4, 7)...\n");
    ember_value args2[2];
    args2[0] = ember_make_number(4.0);
    args2[1] = ember_make_number(7.0);
    
    result = ember_call(vm, "multiply", 2, args2);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        
        if (result_val.type == EMBER_VAL_NUMBER && result_val.as.number_val == 28.0) {
            printf("✓ Test 2 PASSED\n");
        } else {
            printf("✗ Test 2 FAILED - Expected 28, got different value\n");
        }
    } else {
        printf("✗ Test 2 FAILED - Function call returned error: %d\n", result);
    }
    
    // Clear stack for next test
    if (vm->stack_top > 0) pop(vm);
    
    // Test 3: Call function with string argument
    printf("\nTest 3: Calling greet(\"World\")...\n");
    ember_value args3[1];
    args3[0] = ember_make_string("World");
    
    result = ember_call(vm, "greet", 1, args3);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        printf("✓ Test 3 COMPLETED (string concatenation test)\n");
    } else {
        printf("✗ Test 3 FAILED - Function call returned error: %d\n", result);
    }
    
    // Clear stack for next test
    if (vm->stack_top > 0) pop(vm);
    
    // Test 4: Call function with no parameters
    printf("\nTest 4: Calling no_params()...\n");
    result = ember_call(vm, "no_params", 0, NULL);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        
        if (result_val.type == EMBER_VAL_NUMBER && result_val.as.number_val == 42.0) {
            printf("✓ Test 4 PASSED\n");
        } else {
            printf("✗ Test 4 FAILED - Expected 42, got different value\n");
        }
    } else {
        printf("✗ Test 4 FAILED - Function call returned error: %d\n", result);
    }
    
    // Clear stack for next test
    if (vm->stack_top > 0) pop(vm);
    
    // Test 5: Error handling - call non-existent function
    printf("\nTest 5: Calling non-existent function 'undefined_func'...\n");
    result = ember_call(vm, "undefined_func", 0, NULL);
    if (result != 0) {
        printf("✓ Test 5 PASSED - Correctly returned error for undefined function\n");
    } else {
        printf("✗ Test 5 FAILED - Should have returned error for undefined function\n");
    }
    
    // Test 6: Error handling - invalid parameters
    printf("\nTest 6: Testing invalid parameters...\n");
    result = ember_call(NULL, "add", 0, NULL);
    if (result != 0) {
        printf("✓ Test 6a PASSED - Correctly handled NULL VM\n");
    } else {
        printf("✗ Test 6a FAILED - Should have returned error for NULL VM\n");
    }
    
    result = ember_call(vm, NULL, 0, NULL);
    if (result != 0) {
        printf("✓ Test 6b PASSED - Correctly handled NULL function name\n");
    } else {
        printf("✗ Test 6b FAILED - Should have returned error for NULL function name\n");
    }
    
    result = ember_call(vm, "", 0, NULL);
    if (result != 0) {
        printf("✓ Test 6c PASSED - Correctly handled empty function name\n");
    } else {
        printf("✗ Test 6c FAILED - Should have returned error for empty function name\n");
    }
    
    printf("\nAll tests completed!\n");
    
    // Clean up
    ember_free_vm(vm);
    
    return 0;
}