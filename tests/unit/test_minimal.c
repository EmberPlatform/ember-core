#include "ember.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing minimal user function call...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Register a simple native function for comparison
    ember_register_func(vm, "test_native", ember_native_print);
    
    // Test calling the native function first
    printf("Testing native function call...\n");
    ember_value args[1];
    args[0] = ember_make_string("Hello from native!");
    
    int result = ember_call(vm, "test_native", 1, args);

    
    UNUSED(result);
    printf("Native function result: %d\n", result);
    
    // Now test with a very simple user function
    const char* script = "fn simple() { return 123;

    UNUSED(script); }";
    
    printf("Defining user function...\n");
    result = ember_eval(vm, script);
    if (result != 0) {
        fprintf(stderr, "Failed to execute script with error code: %d\n", result);
        ember_free_vm(vm);
        return 1;
    }
    
    printf("Testing user function call...\n");
    result = ember_call(vm, "simple", 0, NULL);
    printf("User function result: %d\n", result);
    
    ember_free_vm(vm);
    return 0;
}