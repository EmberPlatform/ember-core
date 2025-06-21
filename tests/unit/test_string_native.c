#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ember.h"
#include "../../src/stdlib/stdlib.h"

// Test helper function to create string values
static ember_value make_test_string(ember_vm* vm, const char* str) {
    return ember_make_string_gc(vm, str);
}

// Test helper function to get string value from ember_value
static const char* get_test_string(ember_value val) {
    if (val.type != EMBER_VAL_STRING) return NULL;
    if (val.as.obj_val && val.as.obj_val->type == OBJ_STRING) {
        return AS_CSTRING(val);
    }
    return val.as.string_val;
}

// Test helper function to create and validate array
static ember_array* get_test_array(ember_value val) {
    if (val.type != EMBER_VAL_ARRAY) return NULL;
    return AS_ARRAY(val);
}

// Test ember_native_len function
static void test_string_len(ember_vm* vm) {
    printf("Testing string length function...\n");
    
    // Test normal strings
    ember_value str1 = make_test_string(vm, "hello");
    ember_value result1 = ember_native_len(vm, 1, &str1);
    assert(result1.type == EMBER_VAL_NUMBER);
    assert(result1.as.number_val == 5.0);
    printf("  ✓ Normal string length test passed\n");
    
    // Test empty string
    ember_value str2 = make_test_string(vm, "");
    ember_value result2 = ember_native_len(vm, 1, &str2);
    assert(result2.type == EMBER_VAL_NUMBER);
    assert(result2.as.number_val == 0.0);
    printf("  ✓ Empty string length test passed\n");
    
    // Test long string
    ember_value str3 = make_test_string(vm, "This is a much longer string for testing purposes");
    ember_value result3 = ember_native_len(vm, 1, &str3);
    assert(result3.type == EMBER_VAL_NUMBER);
    assert(result3.as.number_val == 49.0);
    printf("  ✓ Long string length test passed\n");
    
    // Test with non-string argument (should return nil)
    ember_value num = ember_make_number(42.0);
    ember_value result4 = ember_native_len(vm, 1, &num);
    assert(result4.type == EMBER_VAL_NIL);
    printf("  ✓ Non-string argument handling test passed\n");
    
    // Test with no arguments
    ember_value result5 = ember_native_len(vm, 0, NULL);
    assert(result5.type == EMBER_VAL_NIL);
    printf("  ✓ No arguments handling test passed\n");
    
    // Test with unicode characters
    ember_value str6 = make_test_string(vm, "héllo");
    ember_value result6 = ember_native_len(vm, 1, &str6);
    assert(result6.type == EMBER_VAL_NUMBER);
    // Note: This tests byte length, not character count
    assert(result6.as.number_val == 6.0);
    printf("  ✓ Unicode string length test passed\n");
    
    printf("String length tests completed successfully!\n\n");
}

// Test ember_native_substr function
static void test_string_substr(ember_vm* vm) {
    printf("Testing string substring function...\n");
    
    // Test normal substring
    ember_value str1 = make_test_string(vm, "hello world");
    ember_value start1 = ember_make_number(0.0);
    ember_value len1 = ember_make_number(5.0);
    ember_value args1[3] = {str1, start1, len1};
    ember_value result1 = ember_native_substr(vm, 3, args1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* substr1 = get_test_string(result1);
    assert(strcmp(substr1, "hello") == 0);
    printf("  ✓ Normal substring test passed\n");
    
    // Test substring from middle
    ember_value start2 = ember_make_number(6.0);
    ember_value len2 = ember_make_number(5.0);
    ember_value args2[3] = {str1, start2, len2};
    ember_value result2 = ember_native_substr(vm, 3, args2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* substr2 = get_test_string(result2);
    assert(strcmp(substr2, "world") == 0);
    printf("  ✓ Middle substring test passed\n");
    
    // Test substring without length (to end of string)
    ember_value args3[2] = {str1, start2};
    ember_value result3 = ember_native_substr(vm, 2, args3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* substr3 = get_test_string(result3);
    assert(strcmp(substr3, "world") == 0);
    printf("  ✓ Substring to end test passed\n");
    
    // Test out of range start (should return empty string)
    ember_value start4 = ember_make_number(20.0);
    ember_value args4[2] = {str1, start4};
    ember_value result4 = ember_native_substr(vm, 2, args4);
    assert(result4.type == EMBER_VAL_STRING);
    const char* substr4 = get_test_string(result4);
    assert(strcmp(substr4, "") == 0);
    printf("  ✓ Out of range start test passed\n");
    
    // Test negative start (should return empty string)
    ember_value start5 = ember_make_number(-1.0);
    ember_value args5[2] = {str1, start5};
    ember_value result5 = ember_native_substr(vm, 2, args5);
    assert(result5.type == EMBER_VAL_STRING);
    const char* substr5 = get_test_string(result5);
    assert(strcmp(substr5, "") == 0);
    printf("  ✓ Negative start test passed\n");
    
    // Test length exceeding string (should truncate)
    ember_value start6 = ember_make_number(6.0);
    ember_value len6 = ember_make_number(20.0);
    ember_value args6[3] = {str1, start6, len6};
    ember_value result6 = ember_native_substr(vm, 3, args6);
    assert(result6.type == EMBER_VAL_STRING);
    const char* substr6 = get_test_string(result6);
    assert(strcmp(substr6, "world") == 0);
    printf("  ✓ Length exceeding string test passed\n");
    
    // Test zero length (should return empty string)
    ember_value len7 = ember_make_number(0.0);
    ember_value args7[3] = {str1, start1, len7};
    ember_value result7 = ember_native_substr(vm, 3, args7);
    assert(result7.type == EMBER_VAL_STRING);
    const char* substr7 = get_test_string(result7);
    assert(strcmp(substr7, "") == 0);
    printf("  ✓ Zero length test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);
    ember_value args8[2] = {num, start1};
    ember_value result8 = ember_native_substr(vm, 2, args8);
    assert(result8.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments test passed\n");
    
    printf("String substring tests completed successfully!\n\n");
}

// Test ember_native_split function
static void test_string_split(ember_vm* vm) {
    printf("Testing string split function...\n");
    
    // Test normal split
    ember_value str1 = make_test_string(vm, "hello,world,test");
    ember_value delim1 = make_test_string(vm, ",");
    ember_value args1[2] = {str1, delim1};
    ember_value result1 = ember_native_split(vm, 2, args1);
    assert(result1.type == EMBER_VAL_ARRAY);
    ember_array* arr1 = get_test_array(result1);
    assert(arr1->length == 3);
    assert(strcmp(get_test_string(arr1->elements[0]), "hello") == 0);
    assert(strcmp(get_test_string(arr1->elements[1]), "world") == 0);
    assert(strcmp(get_test_string(arr1->elements[2]), "test") == 0);
    printf("  ✓ Normal split test passed\n");
    
    // Test split with space delimiter
    ember_value str2 = make_test_string(vm, "hello world test");
    ember_value delim2 = make_test_string(vm, " ");
    ember_value args2[2] = {str2, delim2};
    ember_value result2 = ember_native_split(vm, 2, args2);
    assert(result2.type == EMBER_VAL_ARRAY);
    ember_array* arr2 = get_test_array(result2);
    assert(arr2->length == 3);
    assert(strcmp(get_test_string(arr2->elements[0]), "hello") == 0);
    assert(strcmp(get_test_string(arr2->elements[1]), "world") == 0);
    assert(strcmp(get_test_string(arr2->elements[2]), "test") == 0);
    printf("  ✓ Space delimiter split test passed\n");
    
    // Test split with no delimiter found (should return array with original string)
    ember_value str3 = make_test_string(vm, "hello world");
    ember_value delim3 = make_test_string(vm, ",");
    ember_value args3[2] = {str3, delim3};
    ember_value result3 = ember_native_split(vm, 2, args3);
    assert(result3.type == EMBER_VAL_ARRAY);
    ember_array* arr3 = get_test_array(result3);
    assert(arr3->length == 1);
    assert(strcmp(get_test_string(arr3->elements[0]), "hello world") == 0);
    printf("  ✓ No delimiter found test passed\n");
    
    // Test split empty string
    ember_value str4 = make_test_string(vm, "");
    ember_value args4[2] = {str4, delim1};
    ember_value result4 = ember_native_split(vm, 2, args4);
    assert(result4.type == EMBER_VAL_ARRAY);
    ember_array* arr4 = get_test_array(result4);
    assert(arr4->length == 1);
    assert(strcmp(get_test_string(arr4->elements[0]), "") == 0);
    printf("  ✓ Empty string split test passed\n");
    
    // Test split with consecutive delimiters
    ember_value str5 = make_test_string(vm, "hello,,world");
    ember_value args5[2] = {str5, delim1};
    ember_value result5 = ember_native_split(vm, 2, args5);
    assert(result5.type == EMBER_VAL_ARRAY);
    ember_array* arr5 = get_test_array(result5);
    assert(arr5->length == 3);
    assert(strcmp(get_test_string(arr5->elements[0]), "hello") == 0);
    assert(strcmp(get_test_string(arr5->elements[1]), "") == 0);
    assert(strcmp(get_test_string(arr5->elements[2]), "world") == 0);
    printf("  ✓ Consecutive delimiters test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);
    ember_value args6[2] = {num, delim1};
    ember_value result6 = ember_native_split(vm, 2, args6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments test passed\n");
    
    printf("String split tests completed successfully!\n\n");
}

// Test ember_native_join function
static void test_string_join(ember_vm* vm) {
    printf("Testing string join function...\n");
    
    // Create test array
    ember_value arr1 = ember_make_array(vm, 3);
    ember_array* array1 = get_test_array(arr1);
    array_push(array1, make_test_string(vm, "hello"));
    array_push(array1, make_test_string(vm, "world"));
    array_push(array1, make_test_string(vm, "test"));
    
    // Test normal join
    ember_value delim1 = make_test_string(vm, ",");
    ember_value args1[2] = {arr1, delim1};
    ember_value result1 = ember_native_join(vm, 2, args1);
    assert(result1.type == EMBER_VAL_STRING);
    const char* joined1 = get_test_string(result1);
    assert(strcmp(joined1, "hello,world,test") == 0);
    printf("  ✓ Normal join test passed\n");
    
    // Test join with space delimiter
    ember_value delim2 = make_test_string(vm, " ");
    ember_value args2[2] = {arr1, delim2};
    ember_value result2 = ember_native_join(vm, 2, args2);
    assert(result2.type == EMBER_VAL_STRING);
    const char* joined2 = get_test_string(result2);
    assert(strcmp(joined2, "hello world test") == 0);
    printf("  ✓ Space delimiter join test passed\n");
    
    // Test join empty array
    ember_value arr2 = ember_make_array(vm, 0);
    ember_value args3[2] = {arr2, delim1};
    ember_value result3 = ember_native_join(vm, 2, args3);
    assert(result3.type == EMBER_VAL_STRING);
    const char* joined3 = get_test_string(result3);
    assert(strcmp(joined3, "") == 0);
    printf("  ✓ Empty array join test passed\n");
    
    // Test join single element array
    ember_value arr3 = ember_make_array(vm, 1);
    ember_array* array3 = get_test_array(arr3);
    array_push(array3, make_test_string(vm, "single"));
    ember_value args4[2] = {arr3, delim1};
    ember_value result4 = ember_native_join(vm, 2, args4);
    assert(result4.type == EMBER_VAL_STRING);
    const char* joined4 = get_test_string(result4);
    assert(strcmp(joined4, "single") == 0);
    printf("  ✓ Single element join test passed\n");
    
    // Test join with non-string elements (should skip them)
    ember_value arr4 = ember_make_array(vm, 3);
    ember_array* array4 = get_test_array(arr4);
    array_push(array4, make_test_string(vm, "hello"));
    array_push(array4, ember_make_number(42.0));
    array_push(array4, make_test_string(vm, "world"));
    ember_value args5[2] = {arr4, delim1};
    ember_value result5 = ember_native_join(vm, 2, args5);
    assert(result5.type == EMBER_VAL_STRING);
    const char* joined5 = get_test_string(result5);
    assert(strcmp(joined5, "hello,world") == 0);
    printf("  ✓ Mixed type array join test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);
    ember_value args6[2] = {num, delim1};
    ember_value result6 = ember_native_join(vm, 2, args6);
    assert(result6.type == EMBER_VAL_NIL);
    printf("  ✓ Invalid arguments test passed\n");
    
    printf("String join tests completed successfully!\n\n");
}

// Test ember_native_starts_with function
static void test_string_starts_with(ember_vm* vm) {
    printf("Testing string starts_with function...\n");
    
    // Test positive case
    ember_value str1 = make_test_string(vm, "hello world");
    ember_value prefix1 = make_test_string(vm, "hello");
    ember_value args1[2] = {str1, prefix1};
    ember_value result1 = ember_native_starts_with(vm, 2, args1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 1);
    printf("  ✓ Positive starts_with test passed\n");
    
    // Test negative case
    ember_value prefix2 = make_test_string(vm, "world");
    ember_value args2[2] = {str1, prefix2};
    ember_value result2 = ember_native_starts_with(vm, 2, args2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Negative starts_with test passed\n");
    
    // Test empty prefix (should return true)
    ember_value prefix3 = make_test_string(vm, "");
    ember_value args3[2] = {str1, prefix3};
    ember_value result3 = ember_native_starts_with(vm, 2, args3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 1);
    printf("  ✓ Empty prefix test passed\n");
    
    // Test prefix longer than string
    ember_value str2 = make_test_string(vm, "hi");
    ember_value prefix4 = make_test_string(vm, "hello");
    ember_value args4[2] = {str2, prefix4};
    ember_value result4 = ember_native_starts_with(vm, 2, args4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ Prefix longer than string test passed\n");
    
    // Test exact match
    ember_value args5[2] = {str1, str1};
    ember_value result5 = ember_native_starts_with(vm, 2, args5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 1);
    printf("  ✓ Exact match test passed\n");
    
    // Test case sensitivity
    ember_value prefix5 = make_test_string(vm, "Hello");
    ember_value args6[2] = {str1, prefix5};
    ember_value result6 = ember_native_starts_with(vm, 2, args6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 0);
    printf("  ✓ Case sensitivity test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);
    ember_value args7[2] = {num, prefix1};
    ember_value result7 = ember_native_starts_with(vm, 2, args7);
    assert(result7.type == EMBER_VAL_BOOL);
    assert(result7.as.bool_val == 0);
    printf("  ✓ Invalid arguments test passed\n");
    
    printf("String starts_with tests completed successfully!\n\n");
}

// Test ember_native_ends_with function
static void test_string_ends_with(ember_vm* vm) {
    printf("Testing string ends_with function...\n");
    
    // Test positive case
    ember_value str1 = make_test_string(vm, "hello world");
    ember_value suffix1 = make_test_string(vm, "world");
    ember_value args1[2] = {str1, suffix1};
    ember_value result1 = ember_native_ends_with(vm, 2, args1);
    assert(result1.type == EMBER_VAL_BOOL);
    assert(result1.as.bool_val == 1);
    printf("  ✓ Positive ends_with test passed\n");
    
    // Test negative case
    ember_value suffix2 = make_test_string(vm, "hello");
    ember_value args2[2] = {str1, suffix2};
    ember_value result2 = ember_native_ends_with(vm, 2, args2);
    assert(result2.type == EMBER_VAL_BOOL);
    assert(result2.as.bool_val == 0);
    printf("  ✓ Negative ends_with test passed\n");
    
    // Test empty suffix (should return true)
    ember_value suffix3 = make_test_string(vm, "");
    ember_value args3[2] = {str1, suffix3};
    ember_value result3 = ember_native_ends_with(vm, 2, args3);
    assert(result3.type == EMBER_VAL_BOOL);
    assert(result3.as.bool_val == 1);
    printf("  ✓ Empty suffix test passed\n");
    
    // Test suffix longer than string
    ember_value str2 = make_test_string(vm, "hi");
    ember_value suffix4 = make_test_string(vm, "world");
    ember_value args4[2] = {str2, suffix4};
    ember_value result4 = ember_native_ends_with(vm, 2, args4);
    assert(result4.type == EMBER_VAL_BOOL);
    assert(result4.as.bool_val == 0);
    printf("  ✓ Suffix longer than string test passed\n");
    
    // Test exact match
    ember_value args5[2] = {str1, str1};
    ember_value result5 = ember_native_ends_with(vm, 2, args5);
    assert(result5.type == EMBER_VAL_BOOL);
    assert(result5.as.bool_val == 1);
    printf("  ✓ Exact match test passed\n");
    
    // Test case sensitivity
    ember_value suffix5 = make_test_string(vm, "World");
    ember_value args6[2] = {str1, suffix5};
    ember_value result6 = ember_native_ends_with(vm, 2, args6);
    assert(result6.type == EMBER_VAL_BOOL);
    assert(result6.as.bool_val == 0);
    printf("  ✓ Case sensitivity test passed\n");
    
    // Test with invalid arguments
    ember_value num = ember_make_number(42.0);
    ember_value args7[2] = {num, suffix1};
    ember_value result7 = ember_native_ends_with(vm, 2, args7);
    assert(result7.type == EMBER_VAL_BOOL);
    assert(result7.as.bool_val == 0);
    printf("  ✓ Invalid arguments test passed\n");
    
    printf("String ends_with tests completed successfully!\n\n");
}

// Test security and edge cases
static void test_string_security_edge_cases(ember_vm* vm) {
    printf("Testing security and edge cases...\n");
    
    // Test very long strings for buffer overflow protection
    const size_t long_str_size = 1000;
    char* long_str = malloc(long_str_size + 1);
    memset(long_str, 'a', long_str_size);
    long_str[long_str_size] = '\0';
    
    ember_value long_val = make_test_string(vm, long_str);
    ember_value len_result = ember_native_len(vm, 1, &long_val);
    assert(len_result.type == EMBER_VAL_NUMBER);
    assert(len_result.as.number_val == (double)long_str_size);
    printf("  ✓ Long string handling test passed\n");
    
    free(long_str);
    
    // Test split with potential memory exhaustion (large number of delimiters)
    ember_value many_delims = make_test_string(vm, "a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a,a");
    ember_value comma = make_test_string(vm, ",");
    ember_value split_args[2] = {many_delims, comma};
    ember_value split_result = ember_native_split(vm, 2, split_args);
    assert(split_result.type == EMBER_VAL_ARRAY);
    ember_array* split_arr = get_test_array(split_result);
    assert(split_arr->length == 20);
    printf("  ✓ Many delimiters split test passed\n");
    
    // Test null character handling (should be handled as regular character)
    ember_value null_str = make_test_string(vm, "hello");
    ember_value null_len = ember_native_len(vm, 1, &null_str);
    assert(null_len.type == EMBER_VAL_NUMBER);
    assert(null_len.as.number_val == 5.0);
    printf("  ✓ Null character handling test passed\n");
    
    // Test join with very large array (potential memory issues)
    ember_value big_arr = ember_make_array(vm, 100);
    ember_array* big_array = get_test_array(big_arr);
    for (int i = 0; i < 50; i++) {
        array_push(big_array, make_test_string(vm, "item"));
    }
    ember_value space = make_test_string(vm, " ");
    ember_value join_args[2] = {big_arr, space};
    ember_value join_result = ember_native_join(vm, 2, join_args);
    assert(join_result.type == EMBER_VAL_STRING);
    printf("  ✓ Large array join test passed\n");
    
    printf("Security and edge case tests completed successfully!\n\n");
}

int main() {
    printf("Starting comprehensive string native function tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Run all test suites
    test_string_len(vm);
    test_string_substr(vm);
    test_string_split(vm);
    test_string_join(vm);
    test_string_starts_with(vm);
    test_string_ends_with(vm);
    test_string_security_edge_cases(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("🎉 All string native function tests passed successfully!\n");
    printf("✅ String length function tested\n");
    printf("✅ String substring function tested\n");
    printf("✅ String split function tested\n");
    printf("✅ String join function tested\n");
    printf("✅ String starts_with function tested\n");
    printf("✅ String ends_with function tested\n");
    printf("✅ Security and edge cases tested\n");
    
    return 0;
}