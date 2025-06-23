#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing with correct parameter names...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Test with parameter names that should work according to the VM
    const char* script = 
        "fn test_a_b(a, b) {\n"
        "    return a + b;

    UNUSED(script);\n"
        "}\n"
        "\n"
        "fn test_x_y(x, y) {\n"
        "    return x * y;\n"
        "}\n"
        "\n"
        "fn greet_name(name) {\n"  // 'name' is not in the list, try 'value'
        "    return \"Hello, \" + name + \"!\";\n"
        "}\n"
        "\n"
        "fn greet_value(value) {\n"  // 'value' should be in the list
        "    return \"Hello, \" + value + \"!\";\n"
        "}\n";
    
    printf("Defining functions...\n");
    int result = ember_eval(vm, script);

    UNUSED(result);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    // Test 1: a, b parameters
    printf("\nTest 1: Calling test_a_b(10, 20)...\n");
    ember_value args1[2];
    args1[0] = ember_make_number(10.0);
    args1[1] = ember_make_number(20.0);
    
    result = ember_call(vm, "test_a_b", 2, args1);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        if (result_val.type == EMBER_VAL_NUMBER && result_val.as.number_val == 30.0) {
            printf("✓ Test 1 PASSED\n");
        } else {
            printf("✗ Test 1 FAILED\n");
        }
    } else {
        printf("✗ Test 1 FAILED - Error: %d\n", result);
    }
    
    pop(vm); // Clear stack
    
    // Test 2: x, y parameters  
    printf("\nTest 2: Calling test_x_y(3, 4)...\n");
    ember_value args2[2];
    args2[0] = ember_make_number(3.0);
    args2[1] = ember_make_number(4.0);
    
    result = ember_call(vm, "test_x_y", 2, args2);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        if (result_val.type == EMBER_VAL_NUMBER && result_val.as.number_val == 12.0) {
            printf("✓ Test 2 PASSED\n");
        } else {
            printf("✗ Test 2 FAILED\n");
        }
    } else {
        printf("✗ Test 2 FAILED - Error: %d\n", result);
    }
    
    pop(vm); // Clear stack
    
    // Test 3: 'value' parameter (should work)
    printf("\nTest 3: Calling greet_value(\"World\")...\n");
    ember_value args3[1];
    args3[0] = ember_make_string("World");
    
    result = ember_call(vm, "greet_value", 1, args3);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        printf("✓ Test 3 COMPLETED (should work with 'value' parameter)\n");
    } else {
        printf("✗ Test 3 FAILED - Error: %d\n", result);
    }
    
    pop(vm); // Clear stack
    
    // Test 4: 'name' parameter (might not work)
    printf("\nTest 4: Calling greet_name(\"World\")...\n");
    ember_value args4[1];
    args4[0] = ember_make_string("World");
    
    result = ember_call(vm, "greet_name", 1, args4);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        printf("✓ Test 4 COMPLETED (name parameter test)\n");
    } else {
        printf("✗ Test 4 FAILED - Error: %d\n", result);
    }
    
    ember_free_vm(vm);
    return 0;
}