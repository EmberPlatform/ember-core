#include <stdio.h>
#include "ember.h"

int main() {
    printf("Testing basic Ember operations...\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Test 1: Basic arithmetic
    printf("\nTest 1: Basic arithmetic (2 + 3)\n");
    int result = ember_eval(vm, "print(2 + 3);");
    printf("Result code: %d\n", result);
    
    // Test 2: Variable assignment
    printf("\nTest 2: Variable assignment\n");
    result = ember_eval(vm, "let x = 10; print(x);");
    printf("Result code: %d\n", result);
    
    // Test 3: Function call
    printf("\nTest 3: Function call\n");
    result = ember_eval(vm, "print(\"Hello, World!\");");
    printf("Result code: %d\n", result);
    
    // Test 4: Multiple operations
    printf("\nTest 4: Multiple operations\n");
    result = ember_eval(vm, "let a = 5; let b = 3; print(a * b);");
    printf("Result code: %d\n", result);
    
    ember_free_vm(vm);
    return 0;
}