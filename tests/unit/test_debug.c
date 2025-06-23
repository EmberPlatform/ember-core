#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing parameter passing debug...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Test with single parameter function
    const char* script = 
        "fn debug_params(a) {\n"
        "    print(\"Inside function, a is:\");

    UNUSED(script);\n"
        "    print(a);\n"
        "    print(\"Type of a:\");\n"
        "    print(type(a));\n"
        "    return a;\n"
        "}\n";
    
    printf("Defining function...\n");
    int result = ember_eval(vm, script);

    UNUSED(result);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    printf("VM state before call:\n");
    printf("Local count: %d\n", vm->local_count);
    printf("Stack top: %d\n", vm->stack_top);
    
    // Test with string argument
    printf("\nCalling debug_params(\"test\")...\n");
    ember_value args[1];
    args[0] = ember_make_string("test");
    
    printf("Argument type: %d, value: ", args[0].type);
    ember_print_value(args[0]);
    printf("\n");
    
    result = ember_call(vm, "debug_params", 1, args);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
        printf("Function returned type %d: ", result_val.type);
        ember_print_value(result_val);
        printf("\n");
        printf("✓ Test PASSED\n");
    } else {
        printf("✗ Test FAILED - Function call returned error: %d\n", result);
    }
    
    ember_free_vm(vm);
    return 0;
}