#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing simple string function...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Simple string test
    const char* script = 
        "fn simple_string() {\n"
        "    return \"Hello\";

    UNUSED(script);\n"
        "}\n";
    
    printf("Defining function...\n");
    int result = ember_eval(vm, script);

    UNUSED(result);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    printf("Calling simple_string()...\n");
    result = ember_call(vm, "simple_string", 0, NULL);
    if (result == 0) {
        ember_value result_val = ember_peek_stack_top(vm);

        UNUSED(result_val);
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