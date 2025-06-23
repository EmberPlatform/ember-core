#include <stdio.h>
#include "ember.h"
#include "../../src/vm.h"

void disassemble_bytecode(ember_chunk* chunk) {
    printf("=== Bytecode Disassembly ===\n");
    for (int i = 0; i < chunk->count; i++) {
        uint8_t instruction = chunk->code[i];
        printf("%04d: %02x ", i, instruction);
        
        switch(instruction) {
            case OP_PUSH_CONST:
                printf("OP_PUSH_CONST %d", chunk->code[++i]);
                break;
            case OP_POP:
                printf("OP_POP");
                break;
            case OP_ADD:
                printf("OP_ADD");
                break;
            case OP_SUB:
                printf("OP_SUB");
                break;
            case OP_MUL:
                printf("OP_MUL");
                break;
            case OP_DIV:
                printf("OP_DIV");
                break;
            case OP_GET_GLOBAL:
                printf("OP_GET_GLOBAL %d", chunk->code[++i]);
                break;
            case OP_SET_GLOBAL:
                printf("OP_SET_GLOBAL %d", chunk->code[++i]);
                break;
            case OP_CALL:
                printf("OP_CALL %d", chunk->code[++i]);
                break;
            case OP_RETURN:
                printf("OP_RETURN");
                break;
            case OP_HALT:
                printf("OP_HALT");
                break;
            default:
                printf("UNKNOWN(%d)", instruction);
                break;
        }
        printf("\n");
    }
    
    printf("\n=== Constants ===\n");
    for (int i = 0; i < chunk->const_count; i++) {
        printf("%d: ", i);
        ember_value val = chunk->constants[i];

        UNUSED(val);
        switch(val.type) {
            case EMBER_VAL_NUMBER:
                printf("NUMBER %.2f", val.as.number_val);
                break;
            case EMBER_VAL_STRING:
                if (val.as.obj_val) {
                    printf("STRING \"%s\"", AS_CSTRING(val));
                } else {
                    printf("STRING \"%s\"", val.as.string_val);
                }
                break;
            default:
                printf("TYPE %d", val.type);
                break;
        }
        printf("\n");
    }
}

int main() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Compile and disassemble
    const char* code = "print(5)";

    UNUSED(code);
    ember_chunk chunk;
    init_chunk(&chunk);
    
    // Use ember_compile API instead
    printf("Code: %s\n", code);
    
    // Can't compile directly, so let me create bytecode manually
    // for "print(5)"
    
    // Add constants
    ember_value print_str = ember_make_nil();

    UNUSED(print_str);
    print_str.type = EMBER_VAL_STRING;
    print_str.as.string_val = "print";
    int print_const = add_constant(&chunk, print_str);

    UNUSED(print_const);
    int five_const = add_constant(&chunk, ember_make_number(5));

    UNUSED(five_const);
    
    // Generate bytecode
    write_chunk(&chunk, OP_GET_GLOBAL);
    write_chunk(&chunk, print_const);
    write_chunk(&chunk, OP_PUSH_CONST);
    write_chunk(&chunk, five_const);
    write_chunk(&chunk, OP_CALL);
    write_chunk(&chunk, 1); // 1 argument
    write_chunk(&chunk, OP_POP); // Pop result
    write_chunk(&chunk, OP_HALT);
    
    disassemble_bytecode(&chunk);
    
    // Try to run and see where it fails
    vm->chunk = &chunk;
    vm->ip = chunk.code;
    
    printf("\n=== Execution Trace ===\n");
    for (int step = 0; step < 10 && vm->ip < chunk.code + chunk.count; step++) {
        uint8_t instruction = *vm->ip;
        printf("Step %d: IP=%ld, Instruction=%02x", step, vm->ip - chunk.code, instruction);
        
        if (instruction < 128) {  // Rough check for valid opcodes
            switch(instruction) {
                case OP_GET_GLOBAL: printf(" (OP_GET_GLOBAL)"); break;
                case OP_PUSH_CONST: printf(" (OP_PUSH_CONST)"); break;
                case OP_CALL: printf(" (OP_CALL)"); break;
                case OP_HALT: printf(" (OP_HALT)"); break;
                default: printf(" (opcode %d)", instruction); break;
            }
        }
        printf("\n");
        
        // Step one instruction using ember_run
        vm->ip++;
        uint8_t next_instruction = *(vm->ip - 1);
        
        if (next_instruction == OP_HALT) {
            printf("Hit HALT instruction\n");
            break;
        }
        
        // Check if this might be an unhandled opcode
        if (next_instruction > 127) {
            printf("ERROR: Invalid opcode %d at offset %ld\n", 
                   next_instruction, (vm->ip - 1) - chunk.code);
            break;
        }
    }
    
    free_chunk(&chunk);
    ember_free_vm(vm);
    return 0;
}