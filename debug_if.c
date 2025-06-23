#include "include/ember.h"
#include <stdio.h>

int main() {
    ember_vm* vm = ember_new_vm();
    
    // Simple test: if (1) {}
    char* code = "if (1) {}";
    
    int result = ember_run_code(vm, code, strlen(code));
    
    printf("Result: %d\n", result);
    printf("Stack top: %d\n", vm->stack_top);
    
    ember_free_vm(vm);
    return 0;
}