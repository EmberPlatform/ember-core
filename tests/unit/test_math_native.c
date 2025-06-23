#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include "ember.h"
#include "../../src/stdlib/stdlib.h"

// Test helper to compare floating point values with tolerance
static int double_equals(double a, double b, double tolerance) {
    return fabs(a - b) < tolerance;
}

// Test ember_native_abs function
static void test_math_abs(ember_vm* vm) {
    printf("Testing absolute value function...\n");
    
    // Test positive number
    ember_value pos = ember_make_number(42.5);

    UNUSED(pos);
    ember_value result1 = ember_native_abs(vm, 1, &pos);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 42.5);
    printf("  ✓ Positive number abs test passed\n");
    
    // Test negative number
    ember_value neg = ember_make_number(-42.5);

    UNUSED(neg);
    ember_value result2 = ember_native_abs(vm, 1, &neg);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 42.5);
    printf("  ✓ Negative number abs test passed\n");
    
    // Test zero
    ember_value zero = ember_make_number(0.0);

    UNUSED(zero);
    ember_value result3 = ember_native_abs(vm, 1, &zero);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 0.0);
    printf("  ✓ Zero abs test passed\n");
    
    // Test very small positive number
    ember_value small_pos = ember_make_number(0.000001);

    UNUSED(small_pos);
    ember_value result4 = ember_native_abs(vm, 1, &small_pos);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 0.000001);
    printf("  ✓ Small positive abs test passed\n");
    
    // Test very small negative number
    ember_value small_neg = ember_make_number(-0.000001);

    UNUSED(small_neg);
    ember_value result5 = ember_native_abs(vm, 1, &small_neg);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 0.000001);
    printf("  ✓ Small negative abs test passed\n");
    
    // Test large positive number
    ember_value large_pos = ember_make_number(1000000.0);

    UNUSED(large_pos);
    ember_value result6 = ember_native_abs(vm, 1, &large_pos);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 1000000.0);
    printf("  ✓ Large positive abs test passed\n");
    
    // Test large negative number
    ember_value large_neg = ember_make_number(-1000000.0);

    UNUSED(large_neg);
    ember_value result7 = ember_native_abs(vm, 1, &large_neg);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NUMBER);
    assert(result7.as.number_val == 1000000.0);
    printf("  ✓ Large negative abs test passed\n");
    
    // Test with non-number argument
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value result8 = ember_native_abs(vm, 1, &str);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Non-number argument handling test passed\n");
    
    // Test with no arguments
    ember_value result9 = ember_native_abs(vm, 0, NULL);

    UNUSED(result9);
    assert(result9.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments handling test passed\n");
    
    printf("Absolute value tests completed successfully!\n\n");
}

// Test ember_native_sqrt function
static void test_math_sqrt(ember_vm* vm) {
    printf("Testing square root function...\n");
    
    // Test perfect square
    ember_value perfect = ember_make_number(16.0);

    UNUSED(perfect);
    ember_value result1 = ember_native_sqrt(vm, 1, &perfect);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 4.0);
    printf("  ✓ Perfect square sqrt test passed\n");
    
    // Test non-perfect square
    ember_value non_perfect = ember_make_number(2.0);

    UNUSED(non_perfect);
    ember_value result2 = ember_native_sqrt(vm, 1, &non_perfect);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(double_equals(result2.as.number_val, 1.414213562373095, 0.000000000000001));
    printf("  ✓ Non-perfect square sqrt test passed\n");
    
    // Test zero
    ember_value zero = ember_make_number(0.0);

    UNUSED(zero);
    ember_value result3 = ember_native_sqrt(vm, 1, &zero);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 0.0);
    printf("  ✓ Zero sqrt test passed\n");
    
    // Test one
    ember_value one = ember_make_number(1.0);

    UNUSED(one);
    ember_value result4 = ember_native_sqrt(vm, 1, &one);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 1.0);
    printf("  ✓ One sqrt test passed\n");
    
    // Test large number
    ember_value large = ember_make_number(1000000.0);

    UNUSED(large);
    ember_value result5 = ember_native_sqrt(vm, 1, &large);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 1000.0);
    printf("  ✓ Large number sqrt test passed\n");
    
    // Test small fractional number
    ember_value small_frac = ember_make_number(0.25);

    UNUSED(small_frac);
    ember_value result6 = ember_native_sqrt(vm, 1, &small_frac);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 0.5);
    printf("  ✓ Small fractional sqrt test passed\n");
    
    // Test negative number (should return nil)
    ember_value negative = ember_make_number(-4.0);

    UNUSED(negative);
    ember_value result7 = ember_native_sqrt(vm, 1, &negative);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Negative number sqrt handling test passed\n");
    
    // Test with non-number argument
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value result8 = ember_native_sqrt(vm, 1, &str);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Non-number argument handling test passed\n");
    
    printf("Square root tests completed successfully!\n\n");
}

// Test ember_native_max function
static void test_math_max(ember_vm* vm) {
    printf("Testing max function...\n");
    
    // Test first number larger
    ember_value a1 = ember_make_number(10.0);

    UNUSED(a1);
    ember_value b1 = ember_make_number(5.0);

    UNUSED(b1);
    ember_value args1[2] = {a1, b1};
    ember_value result1 = ember_native_max(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 10.0);
    printf("  ✓ First number larger max test passed\n");
    
    // Test second number larger
    ember_value a2 = ember_make_number(3.0);

    UNUSED(a2);
    ember_value b2 = ember_make_number(7.0);

    UNUSED(b2);
    ember_value args2[2] = {a2, b2};
    ember_value result2 = ember_native_max(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 7.0);
    printf("  ✓ Second number larger max test passed\n");
    
    // Test equal numbers
    ember_value a3 = ember_make_number(5.0);

    UNUSED(a3);
    ember_value b3 = ember_make_number(5.0);

    UNUSED(b3);
    ember_value args3[2] = {a3, b3};
    ember_value result3 = ember_native_max(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 5.0);
    printf("  ✓ Equal numbers max test passed\n");
    
    // Test negative numbers
    ember_value a4 = ember_make_number(-10.0);

    UNUSED(a4);
    ember_value b4 = ember_make_number(-5.0);

    UNUSED(b4);
    ember_value args4[2] = {a4, b4};
    ember_value result4 = ember_native_max(vm, 2, args4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == -5.0);
    printf("  ✓ Negative numbers max test passed\n");
    
    // Test positive and negative
    ember_value a5 = ember_make_number(-3.0);

    UNUSED(a5);
    ember_value b5 = ember_make_number(2.0);

    UNUSED(b5);
    ember_value args5[2] = {a5, b5};
    ember_value result5 = ember_native_max(vm, 2, args5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 2.0);
    printf("  ✓ Positive and negative max test passed\n");
    
    // Test fractional numbers
    ember_value a6 = ember_make_number(3.14);

    UNUSED(a6);
    ember_value b6 = ember_make_number(2.71);

    UNUSED(b6);
    ember_value args6[2] = {a6, b6};
    ember_value result6 = ember_native_max(vm, 2, args6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 3.14);
    printf("  ✓ Fractional numbers max test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value args7[2] = {str, b1};
    ember_value result7 = ember_native_max(vm, 2, args7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments max handling test passed\n");
    
    printf("Max function tests completed successfully!\n\n");
}

// Test ember_native_min function
static void test_math_min(ember_vm* vm) {
    printf("Testing min function...\n");
    
    // Test first number smaller
    ember_value a1 = ember_make_number(3.0);

    UNUSED(a1);
    ember_value b1 = ember_make_number(8.0);

    UNUSED(b1);
    ember_value args1[2] = {a1, b1};
    ember_value result1 = ember_native_min(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 3.0);
    printf("  ✓ First number smaller min test passed\n");
    
    // Test second number smaller
    ember_value a2 = ember_make_number(9.0);

    UNUSED(a2);
    ember_value b2 = ember_make_number(4.0);

    UNUSED(b2);
    ember_value args2[2] = {a2, b2};
    ember_value result2 = ember_native_min(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 4.0);
    printf("  ✓ Second number smaller min test passed\n");
    
    // Test equal numbers
    ember_value a3 = ember_make_number(5.0);

    UNUSED(a3);
    ember_value b3 = ember_make_number(5.0);

    UNUSED(b3);
    ember_value args3[2] = {a3, b3};
    ember_value result3 = ember_native_min(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 5.0);
    printf("  ✓ Equal numbers min test passed\n");
    
    // Test negative numbers
    ember_value a4 = ember_make_number(-5.0);

    UNUSED(a4);
    ember_value b4 = ember_make_number(-10.0);

    UNUSED(b4);
    ember_value args4[2] = {a4, b4};
    ember_value result4 = ember_native_min(vm, 2, args4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == -10.0);
    printf("  ✓ Negative numbers min test passed\n");
    
    // Test positive and negative
    ember_value a5 = ember_make_number(2.0);

    UNUSED(a5);
    ember_value b5 = ember_make_number(-3.0);

    UNUSED(b5);
    ember_value args5[2] = {a5, b5};
    ember_value result5 = ember_native_min(vm, 2, args5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == -3.0);
    printf("  ✓ Positive and negative min test passed\n");
    
    // Test fractional numbers
    ember_value a6 = ember_make_number(2.71);

    UNUSED(a6);
    ember_value b6 = ember_make_number(3.14);

    UNUSED(b6);
    ember_value args6[2] = {a6, b6};
    ember_value result6 = ember_native_min(vm, 2, args6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 2.71);
    printf("  ✓ Fractional numbers min test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value args7[2] = {a1, str};
    ember_value result7 = ember_native_min(vm, 2, args7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments min handling test passed\n");
    
    printf("Min function tests completed successfully!\n\n");
}

// Test ember_native_floor function
static void test_math_floor(ember_vm* vm) {
    printf("Testing floor function...\n");
    
    // Test positive decimal
    ember_value pos_dec = ember_make_number(3.7);

    UNUSED(pos_dec);
    ember_value result1 = ember_native_floor(vm, 1, &pos_dec);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 3.0);
    printf("  ✓ Positive decimal floor test passed\n");
    
    // Test negative decimal
    ember_value neg_dec = ember_make_number(-3.7);

    UNUSED(neg_dec);
    ember_value result2 = ember_native_floor(vm, 1, &neg_dec);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == -4.0);
    printf("  ✓ Negative decimal floor test passed\n");
    
    // Test whole number
    ember_value whole = ember_make_number(5.0);

    UNUSED(whole);
    ember_value result3 = ember_native_floor(vm, 1, &whole);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 5.0);
    printf("  ✓ Whole number floor test passed\n");
    
    // Test zero
    ember_value zero = ember_make_number(0.0);

    UNUSED(zero);
    ember_value result4 = ember_native_floor(vm, 1, &zero);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 0.0);
    printf("  ✓ Zero floor test passed\n");
    
    // Test small positive fraction
    ember_value small_pos = ember_make_number(0.1);

    UNUSED(small_pos);
    ember_value result5 = ember_native_floor(vm, 1, &small_pos);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 0.0);
    printf("  ✓ Small positive fraction floor test passed\n");
    
    // Test small negative fraction
    ember_value small_neg = ember_make_number(-0.1);

    UNUSED(small_neg);
    ember_value result6 = ember_native_floor(vm, 1, &small_neg);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == -1.0);
    printf("  ✓ Small negative fraction floor test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value result7 = ember_native_floor(vm, 1, &str);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments floor handling test passed\n");
    
    printf("Floor function tests completed successfully!\n\n");
}

// Test ember_native_ceil function
static void test_math_ceil(ember_vm* vm) {
    printf("Testing ceil function...\n");
    
    // Test positive decimal
    ember_value pos_dec = ember_make_number(3.2);

    UNUSED(pos_dec);
    ember_value result1 = ember_native_ceil(vm, 1, &pos_dec);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 4.0);
    printf("  ✓ Positive decimal ceil test passed\n");
    
    // Test negative decimal
    ember_value neg_dec = ember_make_number(-3.2);

    UNUSED(neg_dec);
    ember_value result2 = ember_native_ceil(vm, 1, &neg_dec);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == -3.0);
    printf("  ✓ Negative decimal ceil test passed\n");
    
    // Test whole number
    ember_value whole = ember_make_number(5.0);

    UNUSED(whole);
    ember_value result3 = ember_native_ceil(vm, 1, &whole);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 5.0);
    printf("  ✓ Whole number ceil test passed\n");
    
    // Test zero
    ember_value zero = ember_make_number(0.0);

    UNUSED(zero);
    ember_value result4 = ember_native_ceil(vm, 1, &zero);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 0.0);
    printf("  ✓ Zero ceil test passed\n");
    
    // Test small positive fraction
    ember_value small_pos = ember_make_number(0.1);

    UNUSED(small_pos);
    ember_value result5 = ember_native_ceil(vm, 1, &small_pos);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 1.0);
    printf("  ✓ Small positive fraction ceil test passed\n");
    
    // Test small negative fraction
    ember_value small_neg = ember_make_number(-0.1);

    UNUSED(small_neg);
    ember_value result6 = ember_native_ceil(vm, 1, &small_neg);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 0.0);
    printf("  ✓ Small negative fraction ceil test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value result7 = ember_native_ceil(vm, 1, &str);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments ceil handling test passed\n");
    
    printf("Ceil function tests completed successfully!\n\n");
}

// Test ember_native_round function
static void test_math_round(ember_vm* vm) {
    printf("Testing round function...\n");
    
    // Test positive round up
    ember_value pos_up = ember_make_number(3.6);

    UNUSED(pos_up);
    ember_value result1 = ember_native_round(vm, 1, &pos_up);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 4.0);
    printf("  ✓ Positive round up test passed\n");
    
    // Test positive round down
    ember_value pos_down = ember_make_number(3.4);

    UNUSED(pos_down);
    ember_value result2 = ember_native_round(vm, 1, &pos_down);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 3.0);
    printf("  ✓ Positive round down test passed\n");
    
    // Test negative round up (towards zero)
    ember_value neg_up = ember_make_number(-3.4);

    UNUSED(neg_up);
    ember_value result3 = ember_native_round(vm, 1, &neg_up);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == -3.0);
    printf("  ✓ Negative round up test passed\n");
    
    // Test negative round down (away from zero)
    ember_value neg_down = ember_make_number(-3.6);

    UNUSED(neg_down);
    ember_value result4 = ember_native_round(vm, 1, &neg_down);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == -4.0);
    printf("  ✓ Negative round down test passed\n");
    
    // Test exact half (round to even)
    ember_value half = ember_make_number(3.5);

    UNUSED(half);
    ember_value result5 = ember_native_round(vm, 1, &half);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 4.0);
    printf("  ✓ Half value round test passed\n");
    
    // Test whole number
    ember_value whole = ember_make_number(5.0);

    UNUSED(whole);
    ember_value result6 = ember_native_round(vm, 1, &whole);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 5.0);
    printf("  ✓ Whole number round test passed\n");
    
    // Test zero
    ember_value zero = ember_make_number(0.0);

    UNUSED(zero);
    ember_value result7 = ember_native_round(vm, 1, &zero);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NUMBER);
    assert(result7.as.number_val == 0.0);
    printf("  ✓ Zero round test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value result8 = ember_native_round(vm, 1, &str);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments round handling test passed\n");
    
    printf("Round function tests completed successfully!\n\n");
}

// Test ember_native_pow function
static void test_math_pow(ember_vm* vm) {
    printf("Testing power function...\n");
    
    // Test positive base and exponent
    ember_value base1 = ember_make_number(2.0);

    UNUSED(base1);
    ember_value exp1 = ember_make_number(3.0);

    UNUSED(exp1);
    ember_value args1[2] = {base1, exp1};
    ember_value result1 = ember_native_pow(vm, 2, args1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 8.0);
    printf("  ✓ Positive base and exponent pow test passed\n");
    
    // Test base to power of zero
    ember_value base2 = ember_make_number(5.0);

    UNUSED(base2);
    ember_value exp2 = ember_make_number(0.0);

    UNUSED(exp2);
    ember_value args2[2] = {base2, exp2};
    ember_value result2 = ember_native_pow(vm, 2, args2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 1.0);
    printf("  ✓ Base to power of zero test passed\n");
    
    // Test base to power of one
    ember_value base3 = ember_make_number(7.0);

    UNUSED(base3);
    ember_value exp3 = ember_make_number(1.0);

    UNUSED(exp3);
    ember_value args3[2] = {base3, exp3};
    ember_value result3 = ember_native_pow(vm, 2, args3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 7.0);
    printf("  ✓ Base to power of one test passed\n");
    
    // Test negative base with even exponent
    ember_value base4 = ember_make_number(-3.0);

    UNUSED(base4);
    ember_value exp4 = ember_make_number(2.0);

    UNUSED(exp4);
    ember_value args4[2] = {base4, exp4};
    ember_value result4 = ember_native_pow(vm, 2, args4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 9.0);
    printf("  ✓ Negative base with even exponent test passed\n");
    
    // Test negative base with odd exponent
    ember_value base5 = ember_make_number(-2.0);

    UNUSED(base5);
    ember_value exp5 = ember_make_number(3.0);

    UNUSED(exp5);
    ember_value args5[2] = {base5, exp5};
    ember_value result5 = ember_native_pow(vm, 2, args5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == -8.0);
    printf("  ✓ Negative base with odd exponent test passed\n");
    
    // Test fractional exponent (square root)
    ember_value base6 = ember_make_number(9.0);

    UNUSED(base6);
    ember_value exp6 = ember_make_number(0.5);

    UNUSED(exp6);
    ember_value args6[2] = {base6, exp6};
    ember_value result6 = ember_native_pow(vm, 2, args6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 3.0);
    printf("  ✓ Fractional exponent test passed\n");
    
    // Test negative exponent (reciprocal)
    ember_value base7 = ember_make_number(2.0);

    UNUSED(base7);
    ember_value exp7 = ember_make_number(-2.0);

    UNUSED(exp7);
    ember_value args7[2] = {base7, exp7};
    ember_value result7 = ember_native_pow(vm, 2, args7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NUMBER);
    assert(result7.as.number_val == 0.25);
    printf("  ✓ Negative exponent test passed\n");
    
    // Test zero base with positive exponent
    ember_value base8 = ember_make_number(0.0);

    UNUSED(base8);
    ember_value exp8 = ember_make_number(5.0);

    UNUSED(exp8);
    ember_value args8[2] = {base8, exp8};
    ember_value result8 = ember_native_pow(vm, 2, args8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NUMBER);
    assert(result8.as.number_val == 0.0);
    printf("  ✓ Zero base with positive exponent test passed\n");
    
    // Test one as base
    ember_value base9 = ember_make_number(1.0);

    UNUSED(base9);
    ember_value exp9 = ember_make_number(100.0);

    UNUSED(exp9);
    ember_value args9[2] = {base9, exp9};
    ember_value result9 = ember_native_pow(vm, 2, args9);

    UNUSED(result9);
    assert(result9.type == EMBER_VAL_NUMBER);
    assert(result9.as.number_val == 1.0);
    printf("  ✓ One as base test passed\n");
    
    // Test with invalid arguments
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);
    ember_value args10[2] = {str, exp1};
    ember_value result10 = ember_native_pow(vm, 2, args10);

    UNUSED(result10);
    assert(result10.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments pow handling test passed\n");
    
    printf("Power function tests completed successfully!\n\n");
}

// Test edge cases and special values
static void test_math_edge_cases(ember_vm* vm) {
    printf("Testing mathematical edge cases...\n");
    
    // Test with very large numbers
    ember_value large1 = ember_make_number(1e100);

    UNUSED(large1);
    ember_value large2 = ember_make_number(2e100);

    UNUSED(large2);
    ember_value large_args[2] = {large1, large2};
    ember_value large_max = ember_native_max(vm, 2, large_args);

    UNUSED(large_max);
    assert(large_max.type == EMBER_VAL_NUMBER);
    assert(large_max.as.number_val == 2e100);
    printf("  ✓ Very large numbers test passed\n");
    
    // Test with very small numbers
    ember_value small1 = ember_make_number(1e-100);

    UNUSED(small1);
    ember_value small2 = ember_make_number(2e-100);

    UNUSED(small2);
    ember_value small_args[2] = {small1, small2};
    ember_value small_min = ember_native_min(vm, 2, small_args);

    UNUSED(small_min);
    assert(small_min.type == EMBER_VAL_NUMBER);
    assert(small_min.as.number_val == 1e-100);
    printf("  ✓ Very small numbers test passed\n");
    
    // Test precision with multiple operations
    ember_value val = ember_make_number(0.1);

    UNUSED(val);
    ember_value result = ember_native_floor(vm, 1, &val);

    UNUSED(result);
    result = ember_native_abs(vm, 1, &result);
    assert(result.type == EMBER_VAL_NUMBER);
    assert(result.as.number_val == 0.0);
    printf("  ✓ Multiple operations precision test passed\n");
    
    // Test mathematical constants approximations
    ember_value pi_approx = ember_make_number(3.14159265359);

    UNUSED(pi_approx);
    ember_value floor_pi = ember_native_floor(vm, 1, &pi_approx);

    UNUSED(floor_pi);
    assert(floor_pi.type == EMBER_VAL_NUMBER);
    assert(floor_pi.as.number_val == 3.0);
    printf("  ✓ Mathematical constants test passed\n");
    
    printf("Mathematical edge cases tests completed successfully!\n\n");
}

int main() {
    printf("Starting comprehensive math native function tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Run all test suites
    test_math_abs(vm);
    test_math_sqrt(vm);
    test_math_max(vm);
    test_math_min(vm);
    test_math_floor(vm);
    test_math_ceil(vm);
    test_math_round(vm);
    test_math_pow(vm);
    test_math_edge_cases(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("🎉 All math native function tests passed successfully!\n");
    printf("✅ Absolute value function tested\n");
    printf("✅ Square root function tested\n");
    printf("✅ Maximum function tested\n");
    printf("✅ Minimum function tested\n");
    printf("✅ Floor function tested\n");
    printf("✅ Ceiling function tested\n");
    printf("✅ Round function tested\n");
    printf("✅ Power function tested\n");
    printf("✅ Edge cases and special values tested\n");
    
    return 0;
}