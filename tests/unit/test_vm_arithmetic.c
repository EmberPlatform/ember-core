/*
 * Unit tests for refactored VM arithmetic operations
 * 
 * This file demonstrates how the refactored architecture improves testability
 * by allowing individual operation testing in isolation.
 */

#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include <math.h>
#include "../../src/core/vm_operations.h"
#include "../../src/vm.h"
#include "../../src/runtime/value/value.h"

// Test helper macros
#define ASSERT_EQ(expected, actual) \
    do { \
        if ((expected) != (actual)) { \
            fprintf(stderr, "ASSERTION FAILED: %s:%d - Expected %d, got %d\n", \
                    __FILE__, __LINE__, (expected), (actual)); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_DOUBLE_EQ(expected, actual, tolerance) \
    do { \
        if (fabs((expected) - (actual)) > (tolerance)) { \
            fprintf(stderr, "ASSERTION FAILED: %s:%d - Expected %.6f, got %.6f\n", \
                    __FILE__, __LINE__, (expected), (actual)); \
            exit(1); \
        } \
    } while(0)

#define ASSERT_TRUE(condition) \
    do { \
        if (!(condition)) { \
            fprintf(stderr, "ASSERTION FAILED: %s:%d - Condition failed: %s\n", \
                    __FILE__, __LINE__, #condition); \
            exit(1); \
        } \
    } while(0)

// Test fixture setup
static ember_vm* setup_test_vm() {
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    return vm;
}

static void teardown_test_vm(ember_vm* vm) {
    ember_free_vm(vm);
}

// Test: Basic addition of two numbers
void test_addition_basic() {
    printf("Testing basic addition...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push two numbers onto stack
    push(vm, ember_make_number(5.0));
    push(vm, ember_make_number(3.0));
    
    // Execute addition
    vm_operation_result result = vm_handle_add(vm);
    
    // Verify result
    ASSERT_EQ(VM_RESULT_OK, result);
    ASSERT_EQ(1, vm->stack_top); // Should have one result on stack
    ASSERT_EQ(EMBER_VAL_NUMBER, vm->stack[0].type);
    ASSERT_DOUBLE_EQ(8.0, vm->stack[0].as.number_val, 0.001);
    
    teardown_test_vm(vm);
    printf("✓ Basic addition test passed\n");
}

// Test: String concatenation
void test_addition_string_concatenation() {
    printf("Testing string concatenation...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push two strings onto stack
    push(vm, ember_make_string("Hello "));
    push(vm, ember_make_string("World"));
    
    // Execute addition (should concatenate)
    vm_operation_result result = vm_handle_add(vm);
    
    // Verify result
    ASSERT_EQ(VM_RESULT_OK, result);
    ASSERT_EQ(1, vm->stack_top);
    ASSERT_EQ(EMBER_VAL_STRING, vm->stack[0].type);
    
    teardown_test_vm(vm);
    printf("✓ String concatenation test passed\n");
}

// Test: Addition type mismatch error
void test_addition_type_mismatch() {
    printf("Testing addition type mismatch...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push incompatible types onto stack
    push(vm, ember_make_number(5.0));
    push(vm, ember_make_bool(1));
    
    // Execute addition (should fail)
    vm_operation_result result = vm_handle_add(vm);
    
    // Verify error result
    ASSERT_EQ(VM_RESULT_ERROR, result);
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    teardown_test_vm(vm);
    printf("✓ Addition type mismatch test passed\n");
}

// Test: Addition stack underflow
void test_addition_stack_underflow() {
    printf("Testing addition stack underflow...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push only one value (need two for addition)
    push(vm, ember_make_number(5.0));
    
    // Execute addition (should fail due to stack underflow)
    vm_operation_result result = vm_handle_add(vm);
    
    // Verify error result
    ASSERT_EQ(VM_RESULT_ERROR, result);
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    teardown_test_vm(vm);
    printf("✓ Addition stack underflow test passed\n");
}

// Test: Division by zero
void test_division_by_zero() {
    printf("Testing division by zero...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push dividend and zero divisor
    push(vm, ember_make_number(10.0));
    push(vm, ember_make_number(0.0));
    
    // Execute division (should fail)
    vm_operation_result result = vm_handle_divide(vm);
    
    // Verify error result
    ASSERT_EQ(VM_RESULT_ERROR, result);
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    teardown_test_vm(vm);
    printf("✓ Division by zero test passed\n");
}

// Test: Modulo by zero
void test_modulo_by_zero() {
    printf("Testing modulo by zero...\n");
    ember_vm* vm = setup_test_vm();
    
    // Push dividend and zero divisor
    push(vm, ember_make_number(10.0));
    push(vm, ember_make_number(0.0));
    
    // Execute modulo (should fail)
    vm_operation_result result = vm_handle_modulo(vm);
    
    // Verify error result
    ASSERT_EQ(VM_RESULT_ERROR, result);
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    teardown_test_vm(vm);
    printf("✓ Modulo by zero test passed\n");
}

// Test: All arithmetic operations with valid inputs
void test_all_arithmetic_operations() {
    printf("Testing all arithmetic operations...\n");
    ember_vm* vm = setup_test_vm();
    
    // Test subtraction
    push(vm, ember_make_number(10.0));
    push(vm, ember_make_number(3.0));
    ASSERT_EQ(VM_RESULT_OK, vm_handle_subtract(vm));
    ASSERT_DOUBLE_EQ(7.0, vm->stack[0].as.number_val, 0.001);
    pop(vm);
    
    // Test multiplication
    push(vm, ember_make_number(6.0));
    push(vm, ember_make_number(7.0));
    ASSERT_EQ(VM_RESULT_OK, vm_handle_multiply(vm));
    ASSERT_DOUBLE_EQ(42.0, vm->stack[0].as.number_val, 0.001);
    pop(vm);
    
    // Test division
    push(vm, ember_make_number(15.0));
    push(vm, ember_make_number(3.0));
    ASSERT_EQ(VM_RESULT_OK, vm_handle_divide(vm));
    ASSERT_DOUBLE_EQ(5.0, vm->stack[0].as.number_val, 0.001);
    pop(vm);
    
    // Test modulo
    push(vm, ember_make_number(17.0));
    push(vm, ember_make_number(5.0));
    ASSERT_EQ(VM_RESULT_OK, vm_handle_modulo(vm));
    ASSERT_DOUBLE_EQ(2.0, vm->stack[0].as.number_val, 0.001);
    pop(vm);
    
    teardown_test_vm(vm);
    printf("✓ All arithmetic operations test passed\n");
}

// Test: Stack overflow protection
void test_stack_overflow_protection() {
    printf("Testing stack overflow protection...\n");
    ember_vm* vm = setup_test_vm();
    
    // Fill stack to maximum
    for (int i = 0; i < EMBER_STACK_MAX; i++) {
        push(vm, ember_make_number(i));
    }
    
    // Try to push one more value (should fail)
    vm_operation_result result = vm_safe_push(vm, ember_make_number(999), "test");
    
    // Verify error result
    ASSERT_EQ(VM_RESULT_ERROR, result);
    ASSERT_TRUE(ember_vm_has_error(vm));
    
    teardown_test_vm(vm);
    printf("✓ Stack overflow protection test passed\n");
}

// Main test runner
int main() {
    printf("Running VM arithmetic operation tests...\n\n");
    
    test_addition_basic();
    test_addition_string_concatenation();
    test_addition_type_mismatch();
    test_addition_stack_underflow();
    test_division_by_zero();
    test_modulo_by_zero();
    test_all_arithmetic_operations();
    test_stack_overflow_protection();
    
    printf("\n✅ All arithmetic operation tests passed!\n");
    printf("\nTEST COVERAGE ACHIEVED:\n");
    printf("- Basic arithmetic operations (add, sub, mul, div, mod)\n");
    printf("- String concatenation via addition\n");
    printf("- Type validation and error handling\n");
    printf("- Stack underflow/overflow protection\n");
    printf("- Division/modulo by zero protection\n");
    printf("- Consistent error reporting\n");
    
    return 0;
}

/*
 * BENEFITS DEMONSTRATED BY THESE TESTS:
 * 
 * 1. ISOLATED TESTING: Each operation can be tested independently
 * 2. COMPREHENSIVE COVERAGE: Easy to test all edge cases and error conditions
 * 3. FAST EXECUTION: Tests run quickly without full VM initialization
 * 4. CLEAR FAILURE POINTS: Test failures pinpoint exact operation issues
 * 5. REGRESSION DETECTION: Changes to operations can be quickly validated
 * 6. DOCUMENTATION: Tests serve as examples of correct operation usage
 * 
 * This level of testing granularity was not possible with the monolithic
 * ember_run() function, demonstrating a key benefit of the refactoring.
 */