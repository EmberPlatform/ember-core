#include <stdio.h>
#include <string.h>
#include "ember.h"

int main() {
    printf("Testing simple print statement...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Enable verbose error reporting
    const char* code = "print(5)";

    UNUSED(code);
    printf("\nExecuting: %s\n", code);
    
    // Add debugging to see VM state
    printf("VM globals count before: %d\n", vm->global_count);
    
    // Check if print function is registered
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->globals[i].key, "print") == 0) {
            printf("Found 'print' function at global index %d\n", i);
            break;
        }
    }
    
    int result = ember_eval(vm, code);

    
    UNUSED(result);
    printf("Result: %d\n", result);
    
    if (result != 0) {
        printf("Error occurred during execution\n");
    }
    
    ember_free_vm(vm);
    return 0;
}