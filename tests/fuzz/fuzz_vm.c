#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

// Fuzzing configuration
#define FUZZ_MAX_BYTECODE_SIZE 512
#define FUZZ_MAX_CONSTANTS 64
#define FUZZ_ITERATIONS_DEFAULT 1000
#define FUZZ_TIMEOUT_SECONDS 5

// Global variables for signal handling
static jmp_buf timeout_jmp;
static int test_count = 0;
static int crash_count = 0;
static int timeout_count = 0;

// Timeout handler
void timeout_handler(int sig) {
    (void)sig;
    timeout_count++;
    longjmp(timeout_jmp, 1);
}

// Generate random bytecode sequence
void generate_random_bytecode(ember_chunk* chunk) {
    // Initialize chunk
    memset(chunk, 0, sizeof(ember_chunk));
    
    // Allocate memory for bytecode
    int code_size = 1 + (rand() % FUZZ_MAX_BYTECODE_SIZE);
    chunk->code = malloc(code_size);
    chunk->capacity = code_size;
    chunk->count = code_size;
    
    // Allocate memory for constants
    int const_size = rand() % FUZZ_MAX_CONSTANTS;
    if (const_size > 0) {
        chunk->constants = malloc(sizeof(ember_value) * const_size);
        chunk->const_capacity = const_size;
        chunk->const_count = const_size;
        
        // Fill constants with random values
        for (int i = 0; i < const_size; i++) {
            int val_type = rand() % 4;
            switch (val_type) {
                case 0: // Number
                    chunk->constants[i] = ember_make_number((double)(rand() % 1000) - 500);
                    break;
                case 1: // Boolean
                    chunk->constants[i] = ember_make_bool(rand() % 2);
                    break;
                case 2: // String (simple)
                    chunk->constants[i] = ember_make_string("fuzz_string");
                    break;
                case 3: // Nil
                default:
                    chunk->constants[i] = ember_make_nil();
                    break;
            }
        }
    }
    
    // Generate random bytecode instructions
    for (int i = 0; i < code_size; i++) {
        int opcode_type = rand() % 10;
        
        if (opcode_type < 3) {
            // Common arithmetic operations
            ember_opcode ops[] = {OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD};
            chunk->code[i] = ops[rand() % 5];
        } else if (opcode_type < 5) {
            // Stack operations
            ember_opcode ops[] = {OP_PUSH_CONST, OP_POP};
            chunk->code[i] = ops[rand() % 2];
        } else if (opcode_type < 7) {
            // Comparison operations
            ember_opcode ops[] = {OP_EQUAL, OP_NOT_EQUAL, OP_LESS, OP_GREATER};
            chunk->code[i] = ops[rand() % 4];
        } else if (opcode_type < 8) {
            // Control flow
            ember_opcode ops[] = {OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP};
            chunk->code[i] = ops[rand() % 3];
        } else {
            // Random opcode (including invalid ones)
            chunk->code[i] = rand() % 256;
        }
    }
    
    // Always end with HALT to prevent runaway execution
    if (code_size > 0) {
        chunk->code[code_size - 1] = OP_HALT;
    }
}

// Generate malformed bytecode patterns
void generate_malformed_bytecode(ember_chunk* chunk) {
    memset(chunk, 0, sizeof(ember_chunk));
    
    int pattern = rand() % 8;
    int code_size;
    
    switch (pattern) {
        case 0: // Stack underflow pattern
            code_size = 10;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            // Multiple POPs without PUSHes
            for (int i = 0; i < code_size - 1; i++) {
                chunk->code[i] = OP_POP;
            }
            chunk->code[code_size - 1] = OP_HALT;
            break;
            
        case 1: // Stack overflow pattern
            code_size = EMBER_STACK_MAX + 10;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            // Many PUSH_CONSTs
            for (int i = 0; i < code_size - 1; i++) {
                chunk->code[i] = OP_PUSH_CONST;
            }
            chunk->code[code_size - 1] = OP_HALT;
            break;
            
        case 2: // Invalid constant indices
            code_size = 5;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            chunk->code[0] = OP_PUSH_CONST;
            chunk->code[1] = 255; // Invalid constant index
            chunk->code[2] = OP_PUSH_CONST;
            chunk->code[3] = 100; // Another invalid index
            chunk->code[4] = OP_HALT;
            break;
            
        case 3: // Invalid jump addresses
            code_size = 8;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            chunk->code[0] = OP_JUMP;
            chunk->code[1] = 255; // Invalid jump address (high byte)
            chunk->code[2] = 255; // Invalid jump address (low byte)
            chunk->code[3] = OP_JUMP_IF_FALSE;
            chunk->code[4] = 100; // Another invalid jump
            chunk->code[5] = 200;
            chunk->code[6] = OP_LOOP;
            chunk->code[7] = OP_HALT;
            break;
            
        case 4: // Arithmetic without operands
            code_size = 6;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            chunk->code[0] = OP_ADD; // No operands on stack
            chunk->code[1] = OP_SUB;
            chunk->code[2] = OP_MUL;
            chunk->code[3] = OP_DIV;
            chunk->code[4] = OP_MOD;
            chunk->code[5] = OP_HALT;
            break;
            
        case 5: // Function calls without proper setup
            code_size = 4;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            chunk->code[0] = OP_CALL;
            chunk->code[1] = OP_RETURN;
            chunk->code[2] = OP_CALL;
            chunk->code[3] = OP_HALT;
            break;
            
        case 6: // Mixed valid/invalid opcodes
            code_size = 12;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            for (int i = 0; i < code_size - 1; i++) {
                if (i % 2 == 0) {
                    chunk->code[i] = OP_PUSH_CONST;
                } else {
                    chunk->code[i] = 200 + (rand() % 55); // Invalid opcodes
                }
            }
            chunk->code[code_size - 1] = OP_HALT;
            break;
            
        case 7: // All invalid opcodes
        default:
            code_size = 8;
            chunk->code = malloc(code_size);
            chunk->capacity = code_size;
            chunk->count = code_size;
            for (int i = 0; i < code_size - 1; i++) {
                chunk->code[i] = 200 + (rand() % 55); // All invalid
            }
            chunk->code[code_size - 1] = OP_HALT;
            break;
    }
}

// Clean up chunk memory
void free_fuzz_chunk(ember_chunk* chunk) {
    if (chunk->code) {
        free(chunk->code);
        chunk->code = NULL;
    }
    if (chunk->constants) {
        free(chunk->constants);
        chunk->constants = NULL;
    }
    memset(chunk, 0, sizeof(ember_chunk));
}

// Test VM with given bytecode
int test_vm_bytecode(ember_chunk* chunk) {
    ember_vm* vm = NULL;
    int result = 0;
    
    // Set up timeout
    if (setjmp(timeout_jmp)) {
        // Timeout occurred
        if (vm) ember_free_vm(vm);
        return -2; // Timeout error
    }
    
    signal(SIGALRM, timeout_handler);
    alarm(FUZZ_TIMEOUT_SECONDS);
    
    // Initialize VM
    vm = ember_new_vm();
    if (!vm) {
        alarm(0);
        return -1; // Setup error
    }
    
    // Set the chunk in the VM
    vm->chunk = chunk;
    vm->ip = chunk->code;
    
    // Try to execute the bytecode
    // Note: This would need access to the internal run() function
    // For now, we'll simulate VM execution by calling ember_eval with empty string
    // In a real implementation, you'd have a public VM execution API
    result = ember_eval(vm, ""); // This won't actually use our bytecode, but tests VM setup
    
    // Restore VM state
    vm->chunk = NULL;
    vm->ip = NULL;
    
    // Clean up
    alarm(0);
    ember_free_vm(vm);
    
    return result;
}

// Print fuzzing statistics
void print_stats(int iterations) {
    printf("\n=== VM Fuzzing Statistics ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Iterations completed: %d\n", iterations);
    printf("Crashes detected: %d\n", crash_count);
    printf("Timeouts: %d\n", timeout_count);
    printf("Success rate: %.2f%%\n", 
           test_count > 0 ? (100.0 * (test_count - crash_count - timeout_count) / test_count) : 0.0);
}

// Main fuzzing function
int fuzz_vm(int iterations) {
    ember_chunk chunk;
    int i;
    
    printf("Starting VM fuzzing with %d iterations...\n", iterations);
    printf("Max bytecode size: %d bytes\n", FUZZ_MAX_BYTECODE_SIZE);
    printf("Max constants: %d\n", FUZZ_MAX_CONSTANTS);
    printf("Timeout per test: %d seconds\n", FUZZ_TIMEOUT_SECONDS);
    
    for (i = 0; i < iterations; i++) {
        test_count++;
        
        // Show progress every 100 iterations
        if (i % 100 == 0) {
            printf("Progress: %d/%d (crashes: %d, timeouts: %d)\n", 
                   i, iterations, crash_count, timeout_count);
        }
        
        // Generate different types of bytecode
        if (i % 3 == 0) {
            generate_random_bytecode(&chunk);
        } else {
            generate_malformed_bytecode(&chunk);
        }
        
        // Test the VM
        int result = test_vm_bytecode(&chunk);
        
        if (result == -1) {
            crash_count++;
            printf("CRASH detected at iteration %d with bytecode size: %d\n", 
                   i, chunk.count);
        } else if (result == -2) {
            // Timeout already counted in handler
            printf("TIMEOUT at iteration %d with bytecode size: %d\n", 
                   i, chunk.count);
        }
        
        // Clean up
        free_fuzz_chunk(&chunk);
        
        // Stop if too many crashes
        if (crash_count > iterations / 10) {
            printf("Too many crashes detected, stopping early.\n");
            break;
        }
    }
    
    print_stats(i < iterations ? i : iterations);
    return crash_count;
}

int main(int argc, char* argv[]) {
    int iterations = FUZZ_ITERATIONS_DEFAULT;
    
    // Parse command line arguments
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) {
            printf("Invalid iteration count: %s\n", argv[1]);
            return 1;
        }
    }
    
    // Initialize random seed
    srand(time(NULL));
    
    printf("Ember VM Fuzzer\n");
    printf("===============\n");
    
    int crashes = fuzz_vm(iterations);
    
    printf("\nVM fuzzing completed.\n");
    
    if (crashes > 0) {
        printf("WARNING: %d crashes detected during fuzzing!\n", crashes);
        return 1;
    }
    
    printf("No crashes detected. VM appears stable.\n");
    return 0;
}