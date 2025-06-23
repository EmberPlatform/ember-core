#include <stdio.h>
#include "ember.h"

int main() {
    printf("Testing parser issues...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Test simple expressions without 'let' which seems to cause issues
    const char* tests[] = {
        "2 + 3",
        "print(5)",
        "print(2 + 3)",
        "5 * 4",
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