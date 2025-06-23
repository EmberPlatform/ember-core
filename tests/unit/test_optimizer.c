#include "ember.h"
#include "../../src/core/optimizer.h"
#include "../../src/vm.h"
#include <stdio.h>
#include <assert.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>
#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))

// Helper function to create a test chunk
ember_chunk* create_test_chunk(void) {
    ember_chunk* chunk = malloc(sizeof(ember_chunk));

    UNUSED(chunk);
    if (!chunk) return NULL;
    
    init_chunk(chunk);
    return chunk;
}

// Helper function to free a test chunk
void free_test_chunk(ember_chunk* chunk) {
    if (!chunk) return;
    free_chunk(chunk);
    free(chunk);
}

// Helper function to add instructions to chunk
void add_instruction(ember_chunk* chunk, uint8_t op) {
    write_chunk(chunk, op);
}

// Helper function to add instruction with parameter
void add_instruction_with_param(ember_chunk* chunk, uint8_t op, uint8_t param) {
    write_chunk(chunk, op);
    write_chunk(chunk, param);
}

// Helper function to verify chunk contents
int verify_chunk_pattern(ember_chunk* chunk, uint8_t* expected_pattern, int pattern_length) {
    if (chunk->count != pattern_length) {
        return 0;
    }
    
    for (int i = 0; i < pattern_length; i++) {
        if (chunk->code[i] != expected_pattern[i]) {
            return 0;
        }
    }
    
    return 1;
}

// Test initialization of optimization statistics
void test_optimization_stats_init(void) {
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    assert(stats.total_instructions == 0);
    assert(stats.optimized_instructions == 0);
    assert(stats.removed_instructions == 0);
    assert(stats.constant_folded == 0);
    assert(stats.redundant_push_pop == 0);
    assert(stats.combined_arithmetic == 0);
    assert(stats.eliminated_jumps == 0);
    assert(stats.load_store_optimized == 0);
    assert(stats.instruction_fused == 0);
    assert(stats.strength_reduced == 0);
    assert(stats.loop_optimized == 0);
    assert(stats.control_flow_optimized == 0);
    assert(stats.register_allocated == 0);
    assert(stats.optimization_passes == 0);
    
    printf("Optimization statistics initialization test passed\n");
}

// Test constant folding optimization for addition
void test_constant_folding_addition(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constants 5.0 and 3.0
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 3.0}};

    UNUSED(val2);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    
    // Add pattern: PUSH_CONST 5, PUSH_CONST 3, ADD
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_ADD);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_constant_folding(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.constant_folded > 0);
    assert(stats.removed_instructions > 0);
    
    // Should be optimized to single PUSH_CONST 8.0
    assert(chunk->count == 2); // OP_PUSH_CONST + constant index
    assert(chunk->code[0] == OP_PUSH_CONST);
    
    // Check the constant value is 8.0
    uint8_t result_idx = chunk->code[1];
    assert(result_idx < chunk->const_count);
    assert(chunk->constants[result_idx].type == EMBER_VAL_NUMBER);
    assert(chunk->constants[result_idx].as.number_val == 8.0);
    
    free_test_chunk(chunk);
    printf("Constant folding addition test passed\n");
}

// Test constant folding optimization for multiplication
void test_constant_folding_multiplication(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constants 4.0 and 2.5
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 4.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 2.5}};

    UNUSED(val2);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    
    // Add pattern: PUSH_CONST 4, PUSH_CONST 2.5, MUL
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_MUL);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_constant_folding(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.constant_folded > 0);
    
    // Should be optimized to single PUSH_CONST 10.0
    assert(chunk->count == 2);
    assert(chunk->code[0] == OP_PUSH_CONST);
    
    uint8_t result_idx = chunk->code[1];
    assert(result_idx < chunk->const_count);
    assert(chunk->constants[result_idx].type == EMBER_VAL_NUMBER);
    assert(chunk->constants[result_idx].as.number_val == 10.0);
    
    free_test_chunk(chunk);
    printf("Constant folding multiplication test passed\n");
}

// Test constant folding with division by zero (should not optimize)
void test_constant_folding_division_by_zero(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constants 5.0 and 0.0
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val1);  
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 0.0}};
  
    UNUSED(val2);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    
    // Add pattern: PUSH_CONST 5, PUSH_CONST 0, DIV
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_DIV);
    
    int original_count = chunk->count;

    
    UNUSED(original_count);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_constant_folding(chunk, &stats);

    
    UNUSED(optimizations);
    
    // Should not optimize division by zero
    assert(chunk->count == original_count);
    
    free_test_chunk(chunk);
    printf("Constant folding division by zero test passed\n");
}

// Test constant folding for comparison operations
void test_constant_folding_comparison(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constants 5.0 and 3.0
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 3.0}};

    UNUSED(val2);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    
    // Add pattern: PUSH_CONST 5, PUSH_CONST 3, GREATER
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_GREATER);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_constant_folding(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.constant_folded > 0);
    
    // Should be optimized to single PUSH_CONST 1.0 (true)
    assert(chunk->count == 2);
    assert(chunk->code[0] == OP_PUSH_CONST);
    
    uint8_t result_idx = chunk->code[1];
    assert(result_idx < chunk->const_count);
    assert(chunk->constants[result_idx].type == EMBER_VAL_NUMBER);
    assert(chunk->constants[result_idx].as.number_val == 1.0);
    
    free_test_chunk(chunk);
    printf("Constant folding comparison test passed\n");
}

// Test redundant push/pop optimization
void test_redundant_pushpop_optimization(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create a constant
    ember_value val = {EMBER_VAL_NUMBER, {.number_val = 42.0}};

    UNUSED(val);
    int const_idx = add_constant(chunk, val);

    UNUSED(const_idx);
    
    // Add pattern: PUSH_CONST, POP (dead code)
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx);
    add_instruction(chunk, OP_POP);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_redundant_pushpop(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.redundant_push_pop > 0);
    assert(stats.removed_instructions > 0);
    
    // Should be completely removed
    assert(chunk->count == 0);
    
    free_test_chunk(chunk);
    printf("Redundant push/pop optimization test passed\n");
}

// Test jump optimization - jump to next instruction
void test_jump_optimization_next_instruction(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Add pattern: JUMP 0 (jump to next instruction)
    add_instruction_with_param(chunk, OP_JUMP, 0);
    add_instruction(chunk, OP_HALT);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_jump_optimization(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.eliminated_jumps > 0);
    assert(stats.removed_instructions > 0);
    
    // JUMP should be removed, only HALT remains
    assert(chunk->count == 1);
    assert(chunk->code[0] == OP_HALT);
    
    free_test_chunk(chunk);
    printf("Jump optimization next instruction test passed\n");
}

// Test jump optimization - conditional jump to next instruction
void test_jump_optimization_conditional_next(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Add pattern: JUMP_IF_FALSE 0 (conditional jump to next instruction)
    add_instruction_with_param(chunk, OP_JUMP_IF_FALSE, 0);
    add_instruction(chunk, OP_HALT);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_jump_optimization(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.eliminated_jumps > 0);
    assert(stats.removed_instructions > 0);
    
    // JUMP_IF_FALSE should be replaced with POP, then HALT
    assert(chunk->count == 2);
    assert(chunk->code[0] == OP_POP);
    assert(chunk->code[1] == OP_HALT);
    
    free_test_chunk(chunk);
    printf("Jump optimization conditional next test passed\n");
}

// Test strength reduction - multiplication by power of 2
void test_strength_reduction_multiplication(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constant 8.0 (power of 2)
    ember_value val = {EMBER_VAL_NUMBER, {.number_val = 8.0}};

    UNUSED(val);
    int const_idx = add_constant(chunk, val);

    UNUSED(const_idx);
    
    // Add some operand first
    ember_value operand = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(operand);
    int operand_idx = add_constant(chunk, operand);

    UNUSED(operand_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, operand_idx);
    
    // Add pattern: PUSH_CONST 8, MUL
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx);
    add_instruction(chunk, OP_MUL);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_strength_reduction(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.strength_reduced > 0);
    
    free_test_chunk(chunk);
    printf("Strength reduction multiplication test passed\n");
}

// Test strength reduction - multiplication by zero
void test_strength_reduction_multiplication_by_zero(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constant 0.0
    ember_value zero_val = {EMBER_VAL_NUMBER, {.number_val = 0.0}};

    UNUSED(zero_val);
    int zero_idx = add_constant(chunk, zero_val);

    UNUSED(zero_idx);
    
    // Add some operand first
    ember_value operand = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(operand);
    int operand_idx = add_constant(chunk, operand);

    UNUSED(operand_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, operand_idx);
    
    // Add pattern: PUSH_CONST 0, MUL
    add_instruction_with_param(chunk, OP_PUSH_CONST, zero_idx);
    add_instruction(chunk, OP_MUL);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_strength_reduction(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.strength_reduced > 0);
    
    // Should be optimized to: POP, PUSH_CONST 0
    assert(chunk->count == 3);
    assert(chunk->code[0] == OP_POP);
    assert(chunk->code[1] == OP_PUSH_CONST);
    
    free_test_chunk(chunk);
    printf("Strength reduction multiplication by zero test passed\n");
}

// Test instruction fusion - increment pattern
void test_instruction_fusion_increment(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create constant 1.0
    ember_value one_val = {EMBER_VAL_NUMBER, {.number_val = 1.0}};

    UNUSED(one_val);
    int one_idx = add_constant(chunk, one_val);

    UNUSED(one_idx);
    
    // Add pattern: GET_LOCAL 0, PUSH_CONST 1, ADD, SET_LOCAL 0
    add_instruction_with_param(chunk, OP_GET_LOCAL, 0);
    add_instruction_with_param(chunk, OP_PUSH_CONST, one_idx);
    add_instruction(chunk, OP_ADD);
    add_instruction_with_param(chunk, OP_SET_LOCAL, 0);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_instruction_fusion(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.instruction_fused > 0);
    
    free_test_chunk(chunk);
    printf("Instruction fusion increment test passed\n");
}

// Test pattern matching utility
void test_pattern_matching(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create a simple pattern
    add_instruction(chunk, OP_ADD);
    add_instruction(chunk, OP_MUL);
    add_instruction(chunk, OP_SUB);
    
    // Create pattern to match
    uint8_t pattern_instructions[] = {OP_ADD, OP_MUL};
    ember_instruction_pattern pattern = {
        .instructions = pattern_instructions,
        .length = 2,
        .parameter_count = 0
    };
    
    // Should match at position 0
    int match_result = ember_match_pattern(chunk->code, 0, chunk->count, &pattern);

    UNUSED(match_result);
    assert(match_result == 1);
    
    // Should not match at position 1 (MUL, SUB != ADD, MUL)
    match_result = ember_match_pattern(chunk->code, 1, chunk->count, &pattern);
    assert(match_result == 0);
    
    free_test_chunk(chunk);
    printf("Pattern matching test passed\n");
}

// Test pattern matching with wildcards
void test_pattern_matching_wildcards(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create bytecode: PUSH_CONST 5, ADD
    ember_value val = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val);
    int const_idx = add_constant(chunk, val);

    UNUSED(const_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx);
    add_instruction(chunk, OP_ADD);
    
    // Create pattern with wildcard: PUSH_CONST *, ADD
    uint8_t pattern_instructions[] = {OP_PUSH_CONST, 0xFF, OP_ADD}; // 0xFF = wildcard
    ember_instruction_pattern pattern = {
        .instructions = pattern_instructions,
        .length = 3,
        .parameter_count = 0
    };
    
    // Should match at position 0
    int match_result = ember_match_pattern(chunk->code, 0, chunk->count, &pattern);

    UNUSED(match_result);
    assert(match_result == 1);
    
    free_test_chunk(chunk);
    printf("Pattern matching with wildcards test passed\n");
}

// Test loop optimization detection
void test_loop_optimization(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create a simple loop pattern with constant inside
    ember_value val = {EMBER_VAL_NUMBER, {.number_val = 10.0}};

    UNUSED(val);
    int const_idx = add_constant(chunk, val);

    UNUSED(const_idx);
    
    // Simple loop: some instructions then LOOP back
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx); // Loop invariant
    add_instruction(chunk, OP_ADD);
    add_instruction_with_param(chunk, OP_LOOP, 3); // Loop back 3 instructions
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_loop_optimization(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.loop_optimized > 0);
    
    free_test_chunk(chunk);
    printf("Loop optimization test passed\n");
}

// Test control flow optimization - jump chains
void test_control_flow_optimization(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create jump chain: JUMP to JUMP
    add_instruction_with_param(chunk, OP_JUMP, 2); // Jump to position 4
    add_instruction(chunk, OP_HALT);
    add_instruction_with_param(chunk, OP_JUMP, 2); // Another jump at position 4
    add_instruction(chunk, OP_HALT);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_control_flow(chunk, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.control_flow_optimized > 0);
    
    free_test_chunk(chunk);
    printf("Control flow optimization test passed\n");
}

// Test register allocation simulation
void test_register_allocation(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create simple stack operations
    ember_value val = {EMBER_VAL_NUMBER, {.number_val = 42.0}};

    UNUSED(val);
    int const_idx = add_constant(chunk, val);

    UNUSED(const_idx);
    
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const_idx);
    add_instruction(chunk, OP_ADD);
    add_instruction(chunk, OP_POP);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_register_allocation(chunk, &stats);

    
    UNUSED(optimizations);
    
    // Should succeed as we don't exceed stack limits
    assert(optimizations > 0);
    assert(stats.register_allocated > 0);
    
    free_test_chunk(chunk);
    printf("Register allocation test passed\n");
}

// Test comprehensive optimization with all flags
void test_comprehensive_optimization(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create complex bytecode with multiple optimization opportunities
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 3.0}};

    UNUSED(val2);
    ember_value val3 = {EMBER_VAL_NUMBER, {.number_val = 1.0}};

    UNUSED(val3);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    int const3_idx = add_constant(chunk, val3);

    UNUSED(const3_idx);
    
    // Constant folding opportunity
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_ADD);
    
    // Redundant push/pop
    add_instruction_with_param(chunk, OP_PUSH_CONST, const3_idx);
    add_instruction(chunk, OP_POP);
    
    // Jump to next instruction
    add_instruction_with_param(chunk, OP_JUMP, 0);
    
    add_instruction(chunk, OP_HALT);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_chunk(chunk, OPT_ALL, &stats);

    
    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.optimization_passes > 0);
    assert(stats.total_instructions > 0);
    
    // Multiple types of optimizations should have been applied
    int total_specific_optimizations = stats.constant_folded + 
                                      stats.redundant_push_pop + 
                                      stats.eliminated_jumps;

    UNUSED(total_specific_optimizations);
    assert(total_specific_optimizations > 0);
    
    free_test_chunk(chunk);
    printf("Comprehensive optimization test passed\n");
}

// Test optimization with no opportunities
void test_no_optimization_opportunities(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create bytecode with no optimization opportunities
    add_instruction(chunk, OP_HALT);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_chunk(chunk, OPT_ALL, &stats);

    
    UNUSED(optimizations);
    
    // Should complete but find no optimizations
    assert(optimizations == 0);
    assert(stats.optimization_passes == 1); // Still makes one pass
    
    free_test_chunk(chunk);
    printf("No optimization opportunities test passed\n");
}

// Test edge case - empty chunk optimization
void test_empty_chunk_optimization(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Empty chunk
    assert(chunk->count == 0);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    int optimizations = ember_optimize_chunk(chunk, OPT_ALL, &stats);

    
    UNUSED(optimizations);
    
    // Should handle empty chunk gracefully
    assert(optimizations == 0);
    assert(stats.total_instructions == 0);
    
    free_test_chunk(chunk);
    printf("Empty chunk optimization test passed\n");
}

// Test specific optimization flags
void test_specific_optimization_flags(void) {
    ember_chunk* chunk = create_test_chunk();

    UNUSED(chunk);
    assert(chunk != NULL);
    
    // Create bytecode with constant folding opportunity
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 2.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 3.0}};

    UNUSED(val2);
    
    int const1_idx = add_constant(chunk, val1);

    
    UNUSED(const1_idx);
    int const2_idx = add_constant(chunk, val2);

    UNUSED(const2_idx);
    
    add_instruction_with_param(chunk, OP_PUSH_CONST, const1_idx);
    add_instruction_with_param(chunk, OP_PUSH_CONST, const2_idx);
    add_instruction(chunk, OP_MUL);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    // Only enable constant folding
    int optimizations = ember_optimize_chunk(chunk, OPT_CONSTANT_FOLDING, &stats);

    UNUSED(optimizations);
    
    assert(optimizations > 0);
    assert(stats.constant_folded > 0);
    
    free_test_chunk(chunk);
    printf("Specific optimization flags test passed\n");
}

// Test optimization performance improvements
void test_optimization_performance_verification(void) {
    ember_chunk* original_chunk = create_test_chunk();

    UNUSED(original_chunk);
    ember_chunk* optimized_chunk = create_test_chunk();

    UNUSED(optimized_chunk);
    assert(original_chunk != NULL && optimized_chunk != NULL);
    
    // Create identical complex bytecode in both chunks
    ember_value val1 = {EMBER_VAL_NUMBER, {.number_val = 10.0}};

    UNUSED(val1);
    ember_value val2 = {EMBER_VAL_NUMBER, {.number_val = 5.0}};

    UNUSED(val2);
    
    int const1_idx_orig = add_constant(original_chunk, val1);

    
    UNUSED(const1_idx_orig);
    int const2_idx_orig = add_constant(original_chunk, val2);

    UNUSED(const2_idx_orig);
    int const1_idx_opt = add_constant(optimized_chunk, val1);

    UNUSED(const1_idx_opt);
    int const2_idx_opt = add_constant(optimized_chunk, val2);

    UNUSED(const2_idx_opt);
    
    // Add multiple constant folding opportunities
    for (int i = 0; i < 3; i++) {
        // Original chunk
        add_instruction_with_param(original_chunk, OP_PUSH_CONST, const1_idx_orig);
        add_instruction_with_param(original_chunk, OP_PUSH_CONST, const2_idx_orig);
        add_instruction(original_chunk, OP_ADD);
        
        // Optimized chunk (same pattern)
        add_instruction_with_param(optimized_chunk, OP_PUSH_CONST, const1_idx_opt);
        add_instruction_with_param(optimized_chunk, OP_PUSH_CONST, const2_idx_opt);
        add_instruction(optimized_chunk, OP_ADD);
    }
    
    int original_count = original_chunk->count;

    
    UNUSED(original_count);
    
    ember_optimization_stats stats;
    ember_init_optimization_stats(&stats);
    
    // Optimize only the second chunk
    int optimizations = ember_optimize_chunk(optimized_chunk, OPT_ALL, &stats);

    UNUSED(optimizations);
    
    // Verify performance improvement
    assert(optimizations > 0);
    assert(optimized_chunk->count < original_count);
    assert(stats.removed_instructions > 0);
    
    // Calculate improvement percentage
    double improvement = (double)(original_count - optimized_chunk->count) / original_count * 100.0;
    assert(improvement > 0.0);
    
    printf("Optimization achieved %.1f%% code size reduction\n", improvement);
    
    free_test_chunk(original_chunk);
    free_test_chunk(optimized_chunk);
    printf("Optimization performance verification test passed\n");
}

// Main test runner
int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember Optimizer tests...\n\n");
    
    // Basic functionality tests
    test_optimization_stats_init();
    test_pattern_matching();
    test_pattern_matching_wildcards();
    
    // Constant folding tests
    test_constant_folding_addition();
    test_constant_folding_multiplication();
    test_constant_folding_division_by_zero();
    test_constant_folding_comparison();
    
    // Individual optimization pass tests
    test_redundant_pushpop_optimization();
    test_jump_optimization_next_instruction();
    test_jump_optimization_conditional_next();
    test_strength_reduction_multiplication();
    test_strength_reduction_multiplication_by_zero();
    test_instruction_fusion_increment();
    test_loop_optimization();
    test_control_flow_optimization();
    test_register_allocation();
    
    // Comprehensive tests
    test_comprehensive_optimization();
    test_specific_optimization_flags();
    test_optimization_performance_verification();
    
    // Edge case tests
    test_no_optimization_opportunities();
    test_empty_chunk_optimization();
    
    printf("\nAll Ember Optimizer tests passed!\n");
    printf("Total tests run: 20\n");
    
    return 0;
}