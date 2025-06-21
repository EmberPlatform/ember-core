#include "ember.h"
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include "../unit/test_ember_internal.h"

static ember_value native_print_mock(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    (void)argc;
    (void)argv;
    // Mock function to test registration and calling
    return ember_make_bool(1);
}

void test_native_function_registration(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Store the initial global count (built-in functions)
    int initial_count = vm->global_count;
    
    // Register a function with a new name (this will add to globals)
    ember_register_func(vm, "test_print", native_print_mock);
    
    // Global count should increase by 1 (function added)
    assert(vm->global_count == initial_count + 1);
    
    // Find the test_print function in globals and verify it
    bool found_test_print = false;
    for (int i = 0; i < vm->global_count; i++) {
        if (strcmp(vm->globals[i].key, "test_print") == 0) {
            assert(vm->globals[i].value.type == EMBER_VAL_NATIVE);
            found_test_print = true;
            break;
        }
    }
    assert(found_test_print);
    printf("Native function registration test passed\n");
    
    ember_free_vm(vm);
}

void test_native_function_call(void) {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    ember_register_func(vm, "test_func", native_print_mock);
    
    ember_value arg = ember_make_number(42.0);
    int result = ember_call(vm, "test_func", 1, &arg);
    (void)result;
    assert(result == 0);
    printf("Native function call test passed\n");
    
    ember_free_vm(vm);
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    printf("Running Ember Native Function Tests...\n");
    test_native_function_registration();
    test_native_function_call();
    printf("All native function tests passed!\n");
    return 0;
}