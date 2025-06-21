#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing simple string parameter...\n");
    
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Test simple string return without concatenation
    const char* script = 
        "fn return_param(s) {\n"
        "    return s;\n"
        "}\n";
    
    printf("Defining function...\n");
    int result = ember_eval(vm, script);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    printf("Calling return_param(\"test\")...\n");
    ember_value args[1];
    args[0] = ember_make_string("test");
    
    result = ember_call(vm, "return_param", 1, args);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);
        printf("Result type: %d\n", result_val.type);
        printf("Result: ");
        ember_print_value(result_val);
        printf("\n");
        printf("✓ Test PASSED\n");
    } else {
        printf("✗ Test FAILED - Function call returned error: %d\n", result);
    }
    
    ember_free_vm(vm);
    return 0;
}