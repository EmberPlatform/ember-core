#include "ember.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <stdlib.h>

int main() {
    printf("Testing type passing debug...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Test what type we're actually passing
    printf("Creating test number value...\n");
    ember_value num_val = ember_make_number(42.0);

    UNUSED(num_val);
    printf("Number value type: %d, value: %f\n", num_val.type, num_val.as.number_val);
    
    printf("Creating test string value...\n");
    ember_value str_val = ember_make_string("test");

    UNUSED(str_val);
    printf("String value type: %d, value: %s\n", str_val.type, str_val.as.string_val);
    
    // Check what arguments get stored in locals
    printf("VM locals before: %d\n", vm->local_count);
    
    vm->locals[vm->local_count++] = num_val;
    printf("After adding number to locals: type=%d, value=%f\n", 
           vm->locals[vm->local_count-1].type, vm->locals[vm->local_count-1].as.number_val);
    
    vm->locals[vm->local_count++] = str_val;
    printf("After adding string to locals: type=%d, value=%s\n", 
           vm->locals[vm->local_count-1].type, vm->locals[vm->local_count-1].as.string_val);
    
    ember_free_vm(vm);
    return 0;
}