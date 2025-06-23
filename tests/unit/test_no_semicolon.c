#include <stdio.h>
#include "ember.h"

int main() {
    printf("Testing expressions without semicolons...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Test expressions without semicolons
    const char* tests[] = {
        "print(2 + 3)",
        "2 + 3",
        "print(\"Hello, World!\")",
        NULL
    };
    
    for (int i = 0; tests[i] != NULL; i++) {
        printf("\nTest %d: %s\n", i + 1, tests[i]);
        int result = ember_eval(vm, tests[i]);

        UNUSED(result);
        printf("Result code: %d\n", result);
    }
    
    ember_free_vm(vm);
    return 0;
}