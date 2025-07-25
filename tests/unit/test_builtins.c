#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdarg.h>
#include "ember.h"
#include "../../src/runtime/runtime.h"
#include "../../src/runtime/value/value.h"
#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))

// Forward declarations for builtin functions
ember_value ember_native_print(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_type(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_not(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_str(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_num(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_int(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_bool(ember_vm* vm, int argc, ember_value* argv);

// Test helper to compare floating point values with tolerance
__attribute__((unused)) static int double_equals(double a, double b, double tolerance) {
    return fabs(a - b) < tolerance;
}

// Test ember_native_print function
static void test_print_function(ember_vm* vm) {
    printf("Testing print function...\n");
    
    // Test printing number
    ember_value num = ember_make_number(42.5);

    UNUSED(num);

    UNUSED(num);
    ember_value result1 = ember_native_print(vm, 1, &num);

    UNUSED(result1);

    UNUSED(result1);
    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NIL);
    printf("  ✓ Number printing test passed\n");
    
    // Test printing string
    ember_value str = ember_make_string_gc(vm, "Hello, World!");

    UNUSED(str);

    UNUSED(str);
    ember_value result2 = ember_native_print(vm, 1, &str);

    UNUSED(result2);

    UNUSED(result2);
    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NIL);
    printf("  ✓ String printing test passed\n");
    
    // Test printing boolean true
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result3 = ember_native_print(vm, 1, &bool_true);

    UNUSED(result3);

    UNUSED(result3);
    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NIL);
    printf("  ✓ Boolean true printing test passed\n");
    
    // Test printing boolean false
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);
    ember_value result4 = ember_native_print(vm, 1, &bool_false);

    UNUSED(result4);

    UNUSED(result4);
    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NIL);
    printf("  ✓ Boolean false printing test passed\n");
    
    // Test printing nil
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result5 = ember_native_print(vm, 1, &nil);

    UNUSED(result5);

    UNUSED(result5);
    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NIL);
    printf("  ✓ Nil printing test passed\n");
    
    // Test printing array
    ember_value array = ember_make_array(vm, 3);

    UNUSED(array);

    UNUSED(array);
    ember_array* arr = AS_ARRAY(array);

    UNUSED(arr);
    array_push(arr, ember_make_number(1.0));
    array_push(arr, ember_make_number(2.0));
    array_push(arr, ember_make_number(3.0));
    ember_value result6 = ember_native_print(vm, 1, &array);

    UNUSED(result6);

    UNUSED(result6);
    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ Array printing test passed\n");
    
    // Test printing hash map
    ember_value hash_map = ember_make_hash_map(vm, 8);

    UNUSED(hash_map);

    UNUSED(hash_map);
    ember_hash_map* map = AS_HASH_MAP(hash_map);

    UNUSED(map);
    hash_map_set(map, ember_make_string_gc(vm, "key1"), ember_make_number(100.0));
    hash_map_set(map, ember_make_string_gc(vm, "key2"), ember_make_string_gc(vm, "value"));
    ember_value result7 = ember_native_print(vm, 1, &hash_map);

    UNUSED(result7);

    UNUSED(result7);
    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Hash map printing test passed\n");
    
    // Test printing multiple arguments
    ember_value args[3] = {
        ember_make_string_gc(vm, "Hello"),
        ember_make_number(42),
        ember_make_bool(1)
    };
    ember_value result8 = ember_native_print(vm, 3, args);

    UNUSED(result8);

    UNUSED(result8);
    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Multiple arguments printing test passed\n");
    
    // Test printing with no arguments
    ember_value result9 = ember_native_print(vm, 0, NULL);

    UNUSED(result9);

    UNUSED(result9);
    UNUSED(result9);
    assert(result9.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments printing test passed\n");
    
    printf("Print function tests completed successfully!\n\n");
}

// Test ember_native_type function
static void test_type_function(ember_vm* vm) {
    printf("Testing type function...\n");
    
    // Test number type
    ember_value num = ember_make_number(42.5);

    UNUSED(num);

    UNUSED(num);
    ember_value result1 = ember_native_type(vm, 1, &num);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* type_str1 = AS_CSTRING(result1);

    UNUSED(type_str1);

    UNUSED(type_str1);
    assert(strcmp(type_str1, "number") == 0);
    printf("  ✓ Number type test passed\n");
    
    // Test boolean type
    ember_value bool_val = ember_make_bool(1);

    UNUSED(bool_val);

    UNUSED(bool_val);
    ember_value result2 = ember_native_type(vm, 1, &bool_val);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* type_str2 = AS_CSTRING(result2);

    UNUSED(type_str2);

    UNUSED(type_str2);
    assert(strcmp(type_str2, "bool") == 0);
    printf("  ✓ Boolean type test passed\n");
    
    // Test string type
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);

    UNUSED(str);
    ember_value result3 = ember_native_type(vm, 1, &str);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* type_str3 = AS_CSTRING(result3);

    UNUSED(type_str3);

    UNUSED(type_str3);
    assert(strcmp(type_str3, "string") == 0);
    printf("  ✓ String type test passed\n");
    
    // Test nil type
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result4 = ember_native_type(vm, 1, &nil);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_STRING);
    const char* type_str4 = AS_CSTRING(result4);

    UNUSED(type_str4);

    UNUSED(type_str4);
    assert(strcmp(type_str4, "nil") == 0);
    printf("  ✓ Nil type test passed\n");
    
    // Test array type
    ember_value array = ember_make_array(vm, 5);

    UNUSED(array);

    UNUSED(array);
    ember_value result5 = ember_native_type(vm, 1, &array);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_STRING);
    const char* type_str5 = AS_CSTRING(result5);

    UNUSED(type_str5);

    UNUSED(type_str5);
    assert(strcmp(type_str5, "array") == 0);
    printf("  ✓ Array type test passed\n");
    
    // Test hash map type
    ember_value hash_map = ember_make_hash_map(vm, 8);

    UNUSED(hash_map);

    UNUSED(hash_map);
    ember_value result6 = ember_native_type(vm, 1, &hash_map);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_STRING);
    const char* type_str6 = AS_CSTRING(result6);

    UNUSED(type_str6);

    UNUSED(type_str6);
    assert(strcmp(type_str6, "hash_map") == 0);
    printf("  ✓ Hash map type test passed\n");
    
    // Test with no arguments
    ember_value result7 = ember_native_type(vm, 0, NULL);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_STRING);
    const char* type_str7 = AS_CSTRING(result7);

    UNUSED(type_str7);

    UNUSED(type_str7);
    assert(strcmp(type_str7, "nil") == 0);
    printf("  ✓ No arguments type test passed\n");
    
    // Test with multiple arguments (should only consider first)
    ember_value args[2] = {ember_make_number(123), ember_make_string_gc(vm, "test")};
    ember_value result8 = ember_native_type(vm, 2, args);

    UNUSED(result8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_STRING);
    const char* type_str8 = AS_CSTRING(result8);

    UNUSED(type_str8);

    UNUSED(type_str8);
    assert(strcmp(type_str8, "number") == 0);
    printf("  ✓ Multiple arguments type test passed\n");
    
    printf("Type function tests completed successfully!\n\n");
}

// Test ember_native_not function
static void test_not_function(ember_vm* vm) {
    printf("Testing not function...\n");
    
    // Test with true boolean
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result1 = ember_native_not(vm, 1, &bool_true);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 0);
    printf("  ✓ True boolean not test passed\n");
    
    // Test with false boolean
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);
    ember_value result2 = ember_native_not(vm, 1, &bool_false);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 1);
    printf("  ✓ False boolean not test passed\n");
    
    // Test with non-zero number (truthy)
    ember_value nonzero_num = ember_make_number(42.5);

    UNUSED(nonzero_num);

    UNUSED(nonzero_num);
    ember_value result3 = ember_native_not(vm, 1, &nonzero_num);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 0);
    printf("  ✓ Non-zero number not test passed\n");
    
    // Test with zero number (falsy)
    ember_value zero_num = ember_make_number(0.0);

    UNUSED(zero_num);

    UNUSED(zero_num);
    ember_value result4 = ember_native_not(vm, 1, &zero_num);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 1);
    printf("  ✓ Zero number not test passed\n");
    
    // Test with nil (falsy)
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result5 = ember_native_not(vm, 1, &nil);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 1);
    printf("  ✓ Nil not test passed\n");
    
    // Test with string (truthy)
    ember_value str = ember_make_string_gc(vm, "hello");

    UNUSED(str);

    UNUSED(str);
    ember_value result6 = ember_native_not(vm, 1, &str);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 0);
    printf("  ✓ String not test passed\n");
    
    // Test with no arguments
    ember_value result7 = ember_native_not(vm, 0, NULL);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments not test passed\n");
    
    printf("Not function tests completed successfully!\n\n");
}

// Test ember_native_str function
static void test_str_function(ember_vm* vm) {
    printf("Testing str function...\n");
    
    // Test number to string conversion
    ember_value num = ember_make_number(42.5);

    UNUSED(num);

    UNUSED(num);
    ember_value result1 = ember_native_str(vm, 1, &num);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* str1 = AS_CSTRING(result1);

    UNUSED(str1);

    UNUSED(str1);
    assert(strcmp(str1, "42.5") == 0);
    printf("  ✓ Number to string conversion test passed\n");
    
    // Test integer to string conversion
    ember_value int_num = ember_make_number(123.0);

    UNUSED(int_num);

    UNUSED(int_num);
    ember_value result2 = ember_native_str(vm, 1, &int_num);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* str2 = AS_CSTRING(result2);

    UNUSED(str2);

    UNUSED(str2);
    assert(strcmp(str2, "123") == 0);
    printf("  ✓ Integer to string conversion test passed\n");
    
    // Test boolean true to string conversion
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result3 = ember_native_str(vm, 1, &bool_true);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* str3 = AS_CSTRING(result3);

    UNUSED(str3);

    UNUSED(str3);
    assert(strcmp(str3, "true") == 0);
    printf("  ✓ Boolean true to string conversion test passed\n");
    
    // Test boolean false to string conversion
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);
    ember_value result4 = ember_native_str(vm, 1, &bool_false);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_STRING);
    const char* str4 = AS_CSTRING(result4);

    UNUSED(str4);

    UNUSED(str4);
    assert(strcmp(str4, "false") == 0);
    printf("  ✓ Boolean false to string conversion test passed\n");
    
    // Test string to string conversion (copy)
    ember_value str_orig = ember_make_string_gc(vm, "hello world");

    UNUSED(str_orig);

    UNUSED(str_orig);
    ember_value result5 = ember_native_str(vm, 1, &str_orig);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_STRING);
    const char* str5 = AS_CSTRING(result5);

    UNUSED(str5);

    UNUSED(str5);
    assert(strcmp(str5, "hello world") == 0);
    printf("  ✓ String to string conversion test passed\n");
    
    // Test nil to string conversion
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result6 = ember_native_str(vm, 1, &nil);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_STRING);
    const char* str6 = AS_CSTRING(result6);

    UNUSED(str6);

    UNUSED(str6);
    assert(strcmp(str6, "nil") == 0);
    printf("  ✓ Nil to string conversion test passed\n");
    
    // Test array to string conversion
    ember_value array = ember_make_array(vm, 3);

    UNUSED(array);

    UNUSED(array);
    ember_array* arr = AS_ARRAY(array);

    UNUSED(arr);
    array_push(arr, ember_make_number(1.0));
    array_push(arr, ember_make_number(2.0));
    ember_value result7 = ember_native_str(vm, 1, &array);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_STRING);
    const char* str7 = AS_CSTRING(result7);

    UNUSED(str7);

    UNUSED(str7);
    assert(strstr(str7, "array[2]") != NULL);
    printf("  ✓ Array to string conversion test passed\n");
    
    // Test with no arguments
    ember_value result8 = ember_native_str(vm, 0, NULL);

    UNUSED(result8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments str test passed\n");
    
    printf("Str function tests completed successfully!\n\n");
}

// Test ember_native_num function
static void test_num_function(ember_vm* vm) {
    printf("Testing num function...\n");
    
    // Test number to number conversion (identity)
    ember_value num = ember_make_number(42.5);

    UNUSED(num);

    UNUSED(num);
    ember_value result1 = ember_native_num(vm, 1, &num);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 42.5);
    printf("  ✓ Number to number conversion test passed\n");
    
    // Test valid string to number conversion
    ember_value str_num = ember_make_string_gc(vm, "123.45");

    UNUSED(str_num);

    UNUSED(str_num);
    ember_value result2 = ember_native_num(vm, 1, &str_num);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(double_equals(result2.as.number_val, 123.45, 0.00001));
    printf("  ✓ Valid string to number conversion test passed\n");
    
    // Test integer string to number conversion
    ember_value str_int = ember_make_string_gc(vm, "789");

    UNUSED(str_int);

    UNUSED(str_int);
    ember_value result3 = ember_native_num(vm, 1, &str_int);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 789.0);
    printf("  ✓ Integer string to number conversion test passed\n");
    
    // Test negative number string
    ember_value str_neg = ember_make_string_gc(vm, "-456.78");

    UNUSED(str_neg);

    UNUSED(str_neg);
    ember_value result4 = ember_native_num(vm, 1, &str_neg);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(double_equals(result4.as.number_val, -456.78, 0.00001));
    printf("  ✓ Negative number string conversion test passed\n");
    
    // Test string with leading/trailing whitespace
    ember_value str_ws = ember_make_string_gc(vm, "  42.5  ");

    UNUSED(str_ws);

    UNUSED(str_ws);
    ember_value result5 = ember_native_num(vm, 1, &str_ws);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == 42.5);
    printf("  ✓ Whitespace string to number conversion test passed\n");
    
    // Test invalid string to number conversion
    ember_value str_invalid = ember_make_string_gc(vm, "not a number");

    UNUSED(str_invalid);

    UNUSED(str_invalid);
    ember_value result6 = ember_native_num(vm, 1, &str_invalid);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid string to number conversion test passed\n");
    
    // Test empty string
    ember_value str_empty = ember_make_string_gc(vm, "");

    UNUSED(str_empty);

    UNUSED(str_empty);
    ember_value result7 = ember_native_num(vm, 1, &str_empty);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Empty string to number conversion test passed\n");
    
    // Test whitespace-only string
    ember_value str_ws_only = ember_make_string_gc(vm, "   ");

    UNUSED(str_ws_only);

    UNUSED(str_ws_only);
    ember_value result8 = ember_native_num(vm, 1, &str_ws_only);

    UNUSED(result8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Whitespace-only string to number conversion test passed\n");
    
    // Test boolean true to number conversion
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result9 = ember_native_num(vm, 1, &bool_true);

    UNUSED(result9);

    UNUSED(result9);
    assert(result9.type == EMBER_VAL_NUMBER);
    assert(result9.as.number_val == 1.0);
    printf("  ✓ Boolean true to number conversion test passed\n");
    
    // Test boolean false to number conversion
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);
    ember_value result10 = ember_native_num(vm, 1, &bool_false);

    UNUSED(result10);

    UNUSED(result10);
    assert(result10.type == EMBER_VAL_NUMBER);
    assert(result10.as.number_val == 0.0);
    printf("  ✓ Boolean false to number conversion test passed\n");
    
    // Test nil to number conversion
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result11 = ember_native_num(vm, 1, &nil);

    UNUSED(result11);

    UNUSED(result11);
    assert(result11.type == EMBER_VAL_NIL);
    printf("  ✓ Nil to number conversion test passed\n");
    
    // Test with no arguments
    ember_value result12 = ember_native_num(vm, 0, NULL);

    UNUSED(result12);

    UNUSED(result12);
    assert(result12.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments num test passed\n");
    
    printf("Num function tests completed successfully!\n\n");
}

// Test ember_native_int function
static void test_int_function(ember_vm* vm) {
    printf("Testing int function...\n");
    
    // Test number to integer conversion
    ember_value num = ember_make_number(42.9);

    UNUSED(num);

    UNUSED(num);
    ember_value result1 = ember_native_int(vm, 1, &num);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 42.0);
    printf("  ✓ Number to integer conversion test passed\n");
    
    // Test negative number to integer conversion
    ember_value neg_num = ember_make_number(-42.9);

    UNUSED(neg_num);

    UNUSED(neg_num);
    ember_value result2 = ember_native_int(vm, 1, &neg_num);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == -42.0);
    printf("  ✓ Negative number to integer conversion test passed\n");
    
    // Test integer to integer conversion (identity)
    ember_value int_num = ember_make_number(123.0);

    UNUSED(int_num);

    UNUSED(int_num);
    ember_value result3 = ember_native_int(vm, 1, &int_num);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 123.0);
    printf("  ✓ Integer to integer conversion test passed\n");
    
    // Test valid string to integer conversion
    ember_value str_int = ember_make_string_gc(vm, "456");

    UNUSED(str_int);

    UNUSED(str_int);
    ember_value result4 = ember_native_int(vm, 1, &str_int);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_NUMBER);
    assert(result4.as.number_val == 456.0);
    printf("  ✓ Valid string to integer conversion test passed\n");
    
    // Test negative string to integer conversion
    ember_value str_neg = ember_make_string_gc(vm, "-789");

    UNUSED(str_neg);

    UNUSED(str_neg);
    ember_value result5 = ember_native_int(vm, 1, &str_neg);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NUMBER);
    assert(result5.as.number_val == -789.0);
    printf("  ✓ Negative string to integer conversion test passed\n");
    
    // Test string with leading/trailing whitespace
    ember_value str_ws = ember_make_string_gc(vm, "  42  ");

    UNUSED(str_ws);

    UNUSED(str_ws);
    ember_value result6 = ember_native_int(vm, 1, &str_ws);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NUMBER);
    assert(result6.as.number_val == 42.0);
    printf("  ✓ Whitespace string to integer conversion test passed\n");
    
    // Test invalid string to integer conversion
    ember_value str_invalid = ember_make_string_gc(vm, "not a number");

    UNUSED(str_invalid);

    UNUSED(str_invalid);
    ember_value result7 = ember_native_int(vm, 1, &str_invalid);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid string to integer conversion test passed\n");
    
    // Test boolean true to integer conversion
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result8 = ember_native_int(vm, 1, &bool_true);

    UNUSED(result8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_NUMBER);
    assert(result8.as.number_val == 1.0);
    printf("  ✓ Boolean true to integer conversion test passed\n");
    
    // Test boolean false to integer conversion
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);
    ember_value result9 = ember_native_int(vm, 1, &bool_false);

    UNUSED(result9);

    UNUSED(result9);
    assert(result9.type == EMBER_VAL_NUMBER);
    assert(result9.as.number_val == 0.0);
    printf("  ✓ Boolean false to integer conversion test passed\n");
    
    // Test nil to integer conversion
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result10 = ember_native_int(vm, 1, &nil);

    UNUSED(result10);

    UNUSED(result10);
    assert(result10.type == EMBER_VAL_NIL);
    printf("  ✓ Nil to integer conversion test passed\n");
    
    // Test with no arguments
    ember_value result11 = ember_native_int(vm, 0, NULL);

    UNUSED(result11);

    UNUSED(result11);
    assert(result11.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments int test passed\n");
    
    printf("Int function tests completed successfully!\n\n");
}

// Test ember_native_bool function
static void test_bool_function(ember_vm* vm) {
    printf("Testing bool function...\n");
    
    // Test boolean to boolean conversion (identity)
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value result1 = ember_native_bool(vm, 1, &bool_true);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 1);
    printf("  ✓ Boolean true to boolean conversion test passed\n");
    
    ember_value bool_false = ember_make_bool(0);

    
    UNUSED(bool_false);

    
    UNUSED(bool_false);
    ember_value result2 = ember_native_bool(vm, 1, &bool_false);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Boolean false to boolean conversion test passed\n");
    
    // Test non-zero number to boolean conversion
    ember_value nonzero_num = ember_make_number(42.5);

    UNUSED(nonzero_num);

    UNUSED(nonzero_num);
    ember_value result3 = ember_native_bool(vm, 1, &nonzero_num);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 1);
    printf("  ✓ Non-zero number to boolean conversion test passed\n");
    
    // Test zero number to boolean conversion
    ember_value zero_num = ember_make_number(0.0);

    UNUSED(zero_num);

    UNUSED(zero_num);
    ember_value result4 = ember_native_bool(vm, 1, &zero_num);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ Zero number to boolean conversion test passed\n");
    
    // Test "true" string to boolean conversion
    ember_value str_true = ember_make_string_gc(vm, "true");

    UNUSED(str_true);

    UNUSED(str_true);
    ember_value result5 = ember_native_bool(vm, 1, &str_true);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 1);
    printf("  ✓ 'true' string to boolean conversion test passed\n");
    
    // Test "TRUE" string to boolean conversion (case insensitive)
    ember_value str_true_upper = ember_make_string_gc(vm, "TRUE");

    UNUSED(str_true_upper);

    UNUSED(str_true_upper);
    ember_value result6 = ember_native_bool(vm, 1, &str_true_upper);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 1);
    printf("  ✓ 'TRUE' string to boolean conversion test passed\n");
    
    // Test "1" string to boolean conversion
    ember_value str_one = ember_make_string_gc(vm, "1");

    UNUSED(str_one);

    UNUSED(str_one);
    ember_value result7 = ember_native_bool(vm, 1, &str_one);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_BOOL);
    assert(result7.as.bool_val == 1);
    printf("  ✓ '1' string to boolean conversion test passed\n");
    
    // Test "false" string to boolean conversion
    ember_value str_false = ember_make_string_gc(vm, "false");

    UNUSED(str_false);

    UNUSED(str_false);
    ember_value result8 = ember_native_bool(vm, 1, &str_false);

    UNUSED(result8);

    UNUSED(result8);
    assert(result8.type == EMBER_VAL_BOOL);
    assert(result8.as.bool_val == 0);
    printf("  ✓ 'false' string to boolean conversion test passed\n");
    
    // Test "0" string to boolean conversion
    ember_value str_zero = ember_make_string_gc(vm, "0");

    UNUSED(str_zero);

    UNUSED(str_zero);
    ember_value result9 = ember_native_bool(vm, 1, &str_zero);

    UNUSED(result9);

    UNUSED(result9);
    assert(result9.type == EMBER_VAL_BOOL);
    assert(result9.as.bool_val == 0);
    printf("  ✓ '0' string to boolean conversion test passed\n");
    
    // Test empty string to boolean conversion
    ember_value str_empty = ember_make_string_gc(vm, "");

    UNUSED(str_empty);

    UNUSED(str_empty);
    ember_value result10 = ember_native_bool(vm, 1, &str_empty);

    UNUSED(result10);

    UNUSED(result10);
    assert(result10.type == EMBER_VAL_BOOL);
    assert(result10.as.bool_val == 0);
    printf("  ✓ Empty string to boolean conversion test passed\n");
    
    // Test non-empty string to boolean conversion (truthy)
    ember_value str_nonempty = ember_make_string_gc(vm, "hello");

    UNUSED(str_nonempty);

    UNUSED(str_nonempty);
    ember_value result11 = ember_native_bool(vm, 1, &str_nonempty);

    UNUSED(result11);

    UNUSED(result11);
    assert(result11.type == EMBER_VAL_BOOL);
    assert(result11.as.bool_val == 1);
    printf("  ✓ Non-empty string to boolean conversion test passed\n");
    
    // Test nil to boolean conversion
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);
    ember_value result12 = ember_native_bool(vm, 1, &nil);

    UNUSED(result12);

    UNUSED(result12);
    assert(result12.type == EMBER_VAL_BOOL);
    assert(result12.as.bool_val == 0);
    printf("  ✓ Nil to boolean conversion test passed\n");
    
    // Test array to boolean conversion (truthy)
    ember_value array = ember_make_array(vm, 5);

    UNUSED(array);

    UNUSED(array);
    ember_value result13 = ember_native_bool(vm, 1, &array);

    UNUSED(result13);

    UNUSED(result13);
    assert(result13.type == EMBER_VAL_BOOL);
    assert(result13.as.bool_val == 1);
    printf("  ✓ Array to boolean conversion test passed\n");
    
    // Test with no arguments
    ember_value result14 = ember_native_bool(vm, 0, NULL);

    UNUSED(result14);

    UNUSED(result14);
    assert(result14.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments bool test passed\n");
    
    printf("Bool function tests completed successfully!\n\n");
}

// Test edge cases and error conditions
static void test_edge_cases(ember_vm* vm) {
    printf("Testing edge cases and error conditions...\n");
    
    // Test very large numbers
    ember_value large_num = ember_make_number(1e100);

    UNUSED(large_num);

    UNUSED(large_num);
    ember_value result1 = ember_native_str(vm, 1, &large_num);

    UNUSED(result1);

    UNUSED(result1);
    assert(result1.type == EMBER_VAL_STRING);
    printf("  ✓ Very large number to string test passed\n");
    
    // Test very small numbers
    ember_value small_num = ember_make_number(1e-100);

    UNUSED(small_num);

    UNUSED(small_num);
    ember_value result2 = ember_native_str(vm, 1, &small_num);

    UNUSED(result2);

    UNUSED(result2);
    assert(result2.type == EMBER_VAL_STRING);
    printf("  ✓ Very small number to string test passed\n");
    
    // Test infinity
    ember_value inf_num = ember_make_number(INFINITY);

    UNUSED(inf_num);

    UNUSED(inf_num);
    ember_value result3 = ember_native_str(vm, 1, &inf_num);

    UNUSED(result3);

    UNUSED(result3);
    assert(result3.type == EMBER_VAL_STRING);
    printf("  ✓ Infinity to string test passed\n");
    
    // Test NaN
    ember_value nan_num = ember_make_number(NAN);

    UNUSED(nan_num);

    UNUSED(nan_num);
    ember_value result4 = ember_native_str(vm, 1, &nan_num);

    UNUSED(result4);

    UNUSED(result4);
    assert(result4.type == EMBER_VAL_STRING);
    printf("  ✓ NaN to string test passed\n");
    
    // Test long strings
    char long_str[1024];
    memset(long_str, 'A', 1023);
    long_str[1023] = '\0';
    ember_value long_string = ember_make_string_gc(vm, long_str);

    UNUSED(long_string);

    UNUSED(long_string);
    ember_value result5 = ember_native_print(vm, 1, &long_string);

    UNUSED(result5);

    UNUSED(result5);
    assert(result5.type == EMBER_VAL_NIL);
    printf("  ✓ Long string printing test passed\n");
    
    // Test NULL string handling in legacy mode
    ember_value null_str;
    null_str.type = EMBER_VAL_STRING;
    null_str.as.string_val = NULL;
    null_str.as.obj_val = NULL;
    ember_value result6 = ember_native_print(vm, 1, &null_str);

    UNUSED(result6);

    UNUSED(result6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ NULL string handling test passed\n");
    
    // Test mixed argument types
    ember_value mixed_args[5] = {
        ember_make_number(42),
        ember_make_string_gc(vm, "hello"),
        ember_make_bool(1),
        ember_make_nil(),
        ember_make_array(vm, 2)
    };
    ember_value result7 = ember_native_print(vm, 5, mixed_args);

    UNUSED(result7);

    UNUSED(result7);
    assert(result7.type == EMBER_VAL_NIL);
    printf("  ✓ Mixed argument types test passed\n");
    
    printf("Edge cases and error conditions tests completed successfully!\n\n");
}

// Test memory management and cleanup
static void test_memory_management(ember_vm* vm) {
    printf("Testing memory management...\n");
    
    // Create many values and let GC handle them
    for (int i = 0; i < 100; i++) {
        ember_value str = ember_make_string_gc(vm, "test string");

        UNUSED(str);

        UNUSED(str);
        ember_value result = ember_native_str(vm, 1, &str);

        UNUSED(result);

        UNUSED(result);
        (void)result; // Suppress unused variable warning
    }
    printf("  ✓ String allocation and GC test passed\n");
    
    // Create arrays and manipulate them
    for (int i = 0; i < 50; i++) {
        ember_value array = ember_make_array(vm, 10);

        UNUSED(array);

        UNUSED(array);
        ember_array* arr = AS_ARRAY(array);

        UNUSED(arr);
        for (int j = 0; j < 10; j++) {
            array_push(arr, ember_make_number(j));
        }
        ember_value result = ember_native_print(vm, 1, &array);

        UNUSED(result);

        UNUSED(result);
        (void)result; // Suppress unused variable warning
    }
    printf("  ✓ Array allocation and GC test passed\n");
    
    // Create hash maps
    for (int i = 0; i < 25; i++) {
        ember_value hash_map = ember_make_hash_map(vm, 8);

        UNUSED(hash_map);

        UNUSED(hash_map);
        ember_hash_map* map = AS_HASH_MAP(hash_map);

        UNUSED(map);
        for (int j = 0; j < 5; j++) {
            char key[16];
            snprintf(key, sizeof(key), "key%d", j);
            hash_map_set(map, ember_make_string_gc(vm, key), ember_make_number(j));
        }
        ember_value result = ember_native_print(vm, 1, &hash_map);

        UNUSED(result);

        UNUSED(result);
        (void)result; // Suppress unused variable warning
    }
    printf("  ✓ Hash map allocation and GC test passed\n");
    
    printf("Memory management tests completed successfully!\n\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember built-in functions tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    // Run all tests
    test_print_function(vm);
    test_type_function(vm);
    test_not_function(vm);
    test_str_function(vm);
    test_num_function(vm);
    test_int_function(vm);
    test_bool_function(vm);
    test_edge_cases(vm);
    test_memory_management(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("All built-in function tests passed successfully!\n");
    return 0;
}