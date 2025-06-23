#include <stdio.h>
#include "ember.h"
#include "../../src/vm.h"

int main() {
    printf("Testing VM dispatch table...\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Create a simple bytecode chunk manually
    ember_chunk chunk;
    init_chunk(&chunk);
    
    // Push constant 5
    int const_idx = add_constant(&chunk, ember_make_number(5));

    UNUSED(const_idx);
    write_chunk(&chunk, OP_PUSH_CONST);
    write_chunk(&chunk, const_idx);
    
    // Push constant 3
    const_idx = add_constant(&chunk, ember_make_number(3));
    write_chunk(&chunk, OP_PUSH_CONST);
    write_chunk(&chunk, const_idx);
    
    // Add them
    write_chunk(&chunk, OP_ADD);
    
    // Pop result (for testing)
    write_chunk(&chunk, OP_POP);
    
    // Halt
    write_chunk(&chunk, OP_HALT);
    
    // Set up VM to run this chunk
    vm->chunk = &chunk;
    vm->ip = chunk.code;
    
    printf("Running bytecode...\n");
    printf("Bytecode: ");
    for (int i = 0; i < chunk.count; i++) {
        printf("%02x ", chunk.code[i]);
    }
    printf("\n");
    
    int result = ember_run(vm);

    
    UNUSED(result);
    printf("Result: %d\n", result);
    printf("Stack top: %d\n", vm->stack_top);
    
    if (vm->stack_top > 0) {
        printf("Top of stack: ");
        ember_value top = vm->stack[vm->stack_top - 1];

        UNUSED(top);
        if (top.type == EMBER_VAL_NUMBER) {
            printf("%.2f\n", top.as.number_val);
        } else {
            printf("(type %d)\n", top.type);
        }
    }
    
    free_chunk(&chunk);
    ember_free_vm(vm);
    return 0;
}