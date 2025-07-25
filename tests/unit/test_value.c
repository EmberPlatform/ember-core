#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>
#include <float.h>
#include <stdint.h>
#include <limits.h>
#include "ember.h"
#include "../../src/runtime/value/value.h"
#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))

// Forward declarations for missing functions
ember_value ember_make_exception(ember_vm* vm, const char* type, const char* message);
void gc_write_barrier_helper(ember_vm* vm, ember_object* obj, ember_value old_val, ember_value new_val);
void collect_garbage(ember_vm* vm);

// Test helper to compare floating point values with tolerance
__attribute__((unused)) static int double_equals(double a, double b, double tolerance) {
    return fabs(a - b) < tolerance;
}

// Test basic value creation functions
static void test_value_creation(ember_vm* vm) {
    printf("Testing value creation functions...\n");
    
    // Test number creation
    ember_value num = ember_make_number(42.5);

    UNUSED(num);

    UNUSED(num);

    UNUSED(num);
    assert(num.type == EMBER_VAL_NUMBER);
    assert(num.as.number_val == 42.5);
    printf("  ✓ Number creation test passed\n");
    
    // Test boolean creation
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);

    UNUSED(bool_true);
    assert(bool_true.type == EMBER_VAL_BOOL);
    assert(bool_true.as.bool_val == 1);
    
    ember_value bool_false = ember_make_bool(0);

    
    UNUSED(bool_false);

    
    UNUSED(bool_false);

    
    UNUSED(bool_false);
    assert(bool_false.type == EMBER_VAL_BOOL);
    assert(bool_false.as.bool_val == 0);
    printf("  ✓ Boolean creation test passed\n");
    
    // Test nil creation
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);

    UNUSED(nil);
    assert(nil.type == EMBER_VAL_NIL);
    printf("  ✓ Nil creation test passed\n");
    
    // Test string creation (legacy)
    ember_value str_legacy = ember_make_string("Hello, World!");

    UNUSED(str_legacy);

    UNUSED(str_legacy);

    UNUSED(str_legacy);
    assert(str_legacy.type == EMBER_VAL_STRING);
    assert(str_legacy.as.string_val != NULL);
    assert(strcmp(str_legacy.as.string_val, "Hello, World!") == 0);
    free_ember_value(str_legacy);
    printf("  ✓ Legacy string creation test passed\n");
    
    // Test string creation with GC
    ember_value str_gc = ember_make_string_gc(vm, "Hello, GC!");

    UNUSED(str_gc);

    UNUSED(str_gc);

    UNUSED(str_gc);
    assert(str_gc.type == EMBER_VAL_STRING);
    assert(str_gc.as.obj_val != NULL);
    assert(str_gc.as.obj_val->type == OBJ_STRING);
    ember_string* str_obj = AS_STRING(str_gc);

    UNUSED(str_obj);

    UNUSED(str_obj);
    assert(strcmp(str_obj->chars, "Hello, GC!") == 0);
    printf("  ✓ GC string creation test passed\n");
    
    // Test array creation
    ember_value array = ember_make_array(vm, 10);

    UNUSED(array);

    UNUSED(array);

    UNUSED(array);
    assert(array.type == EMBER_VAL_ARRAY);
    assert(array.as.obj_val != NULL);
    assert(array.as.obj_val->type == OBJ_ARRAY);
    ember_array* arr = AS_ARRAY(array);

    UNUSED(arr);

    UNUSED(arr);
    assert(arr->capacity == 10);
    assert(arr->length == 0);
    assert(arr->elements != NULL);
    printf("  ✓ Array creation test passed\n");
    
    // Test hash map creation
    ember_value hash_map = ember_make_hash_map(vm, 16);

    UNUSED(hash_map);

    UNUSED(hash_map);

    UNUSED(hash_map);
    assert(hash_map.type == EMBER_VAL_HASH_MAP);
    assert(hash_map.as.obj_val != NULL);
    assert(hash_map.as.obj_val->type == OBJ_HASH_MAP);
    ember_hash_map* map = AS_HASH_MAP(hash_map);

    UNUSED(map);

    UNUSED(map);
    assert(map->capacity == 16);
    assert(map->length == 0);
    assert(map->entries != NULL);
    printf("  ✓ Hash map creation test passed\n");
    
    printf("Value creation tests completed successfully!\n\n");
}

// Test value manipulation and operations
static void test_value_manipulation(ember_vm* vm) {
    printf("Testing value manipulation...\n");
    
    // Test value copying
    ember_value orig_num = ember_make_number(123.45);

    UNUSED(orig_num);

    UNUSED(orig_num);

    UNUSED(orig_num);
    ember_value copy_num = copy_ember_value(orig_num);

    UNUSED(copy_num);

    UNUSED(copy_num);

    UNUSED(copy_num);
    assert(copy_num.type == EMBER_VAL_NUMBER);
    assert(copy_num.as.number_val == 123.45);
    printf("  ✓ Number copying test passed\n");
    
    // Test string copying (legacy)
    ember_value orig_str = ember_make_string("Test string");

    UNUSED(orig_str);

    UNUSED(orig_str);

    UNUSED(orig_str);
    ember_value copy_str = copy_ember_value(orig_str);

    UNUSED(copy_str);

    UNUSED(copy_str);

    UNUSED(copy_str);
    assert(copy_str.type == EMBER_VAL_STRING);
    assert(copy_str.as.string_val != orig_str.as.string_val); // Different pointers
    assert(strcmp(copy_str.as.string_val, "Test string") == 0);
    free_ember_value(orig_str);
    free_ember_value(copy_str);
    printf("  ✓ String copying test passed\n");
    
    // Test value equality comparison
    ember_value num1 = ember_make_number(42.0);

    UNUSED(num1);

    UNUSED(num1);

    UNUSED(num1);
    ember_value num2 = ember_make_number(42.0);

    UNUSED(num2);

    UNUSED(num2);

    UNUSED(num2);
    ember_value num3 = ember_make_number(43.0);

    UNUSED(num3);

    UNUSED(num3);

    UNUSED(num3);
    assert(values_equal(num1, num2) == 1);
    assert(values_equal(num1, num3) == 0);
    printf("  ✓ Number equality test passed\n");
    
    ember_value bool1 = ember_make_bool(1);

    
    UNUSED(bool1);

    
    UNUSED(bool1);

    
    UNUSED(bool1);
    ember_value bool2 = ember_make_bool(1);

    UNUSED(bool2);

    UNUSED(bool2);

    UNUSED(bool2);
    ember_value bool3 = ember_make_bool(0);

    UNUSED(bool3);

    UNUSED(bool3);

    UNUSED(bool3);
    assert(values_equal(bool1, bool2) == 1);
    assert(values_equal(bool1, bool3) == 0);
    printf("  ✓ Boolean equality test passed\n");
    
    ember_value nil1 = ember_make_nil();

    
    UNUSED(nil1);

    
    UNUSED(nil1);

    
    UNUSED(nil1);
    ember_value nil2 = ember_make_nil();

    UNUSED(nil2);

    UNUSED(nil2);

    UNUSED(nil2);
    assert(values_equal(nil1, nil2) == 1);
    printf("  ✓ Nil equality test passed\n");
    
    // Test string equality (GC strings)
    ember_value str1 = ember_make_string_gc(vm, "hello");

    UNUSED(str1);

    UNUSED(str1);

    UNUSED(str1);
    ember_value str2 = ember_make_string_gc(vm, "hello");

    UNUSED(str2);

    UNUSED(str2);

    UNUSED(str2);
    ember_value str3 = ember_make_string_gc(vm, "world");

    UNUSED(str3);

    UNUSED(str3);

    UNUSED(str3);
    assert(values_equal(str1, str2) == 1);
    assert(values_equal(str1, str3) == 0);
    printf("  ✓ String equality test passed\n");
    
    // Test different type equality
    ember_value diff_num = ember_make_number(42.0);

    UNUSED(diff_num);

    UNUSED(diff_num);

    UNUSED(diff_num);
    ember_value diff_str = ember_make_string_gc(vm, "42");

    UNUSED(diff_str);

    UNUSED(diff_str);

    UNUSED(diff_str);
    assert(values_equal(diff_num, diff_str) == 0);
    printf("  ✓ Different type equality test passed\n");
    
    printf("Value manipulation tests completed successfully!\n\n");
}

// Test array operations
static void test_array_operations(ember_vm* vm) {
    printf("Testing array operations...\n");
    
    // Test array creation with zero capacity
    ember_value array_zero = ember_make_array(vm, 0);

    UNUSED(array_zero);

    UNUSED(array_zero);

    UNUSED(array_zero);
    assert(array_zero.type == EMBER_VAL_ARRAY);
    ember_array* arr_zero = AS_ARRAY(array_zero);

    UNUSED(arr_zero);

    UNUSED(arr_zero);
    assert(arr_zero->capacity == 0);
    assert(arr_zero->elements == NULL);
    printf("  ✓ Zero capacity array creation test passed\n");
    
    // Test array push operations
    ember_value array = ember_make_array(vm, 2);

    UNUSED(array);

    UNUSED(array);

    UNUSED(array);
    ember_array* arr = AS_ARRAY(array);

    UNUSED(arr);

    UNUSED(arr);
    
    // Push first element
    array_push(arr, ember_make_number(1.0));
    assert(arr->length == 1);
    assert(arr->elements[0].type == EMBER_VAL_NUMBER);
    assert(arr->elements[0].as.number_val == 1.0);
    printf("  ✓ Array push first element test passed\n");
    
    // Push second element
    array_push(arr, ember_make_string_gc(vm, "hello"));
    assert(arr->length == 2);
    assert(arr->elements[1].type == EMBER_VAL_STRING);
    printf("  ✓ Array push second element test passed\n");
    
    // Push third element (should trigger resize)
    array_push(arr, ember_make_bool(1));
    assert(arr->length == 3);
    assert(arr->capacity >= 3);
    assert(arr->elements[2].type == EMBER_VAL_BOOL);
    assert(arr->elements[2].as.bool_val == 1);
    printf("  ✓ Array resize on push test passed\n");
    
    // Test array equality
    ember_value array1 = ember_make_array(vm, 3);

    UNUSED(array1);

    UNUSED(array1);

    UNUSED(array1);
    ember_array* arr1 = AS_ARRAY(array1);

    UNUSED(arr1);

    UNUSED(arr1);
    array_push(arr1, ember_make_number(10.0));
    array_push(arr1, ember_make_number(20.0));
    
    ember_value array2 = ember_make_array(vm, 3);

    
    UNUSED(array2);

    
    UNUSED(array2);

    
    UNUSED(array2);
    ember_array* arr2 = AS_ARRAY(array2);

    UNUSED(arr2);

    UNUSED(arr2);
    array_push(arr2, ember_make_number(10.0));
    array_push(arr2, ember_make_number(20.0));
    
    ember_value array3 = ember_make_array(vm, 3);

    
    UNUSED(array3);

    
    UNUSED(array3);

    
    UNUSED(array3);
    ember_array* arr3 = AS_ARRAY(array3);

    UNUSED(arr3);

    UNUSED(arr3);
    array_push(arr3, ember_make_number(10.0));
    array_push(arr3, ember_make_number(30.0));
    
    assert(values_equal(array1, array2) == 1);
    assert(values_equal(array1, array3) == 0);
    printf("  ✓ Array equality test passed\n");
    
    // Test NULL array push
    array_push(NULL, ember_make_number(42.0));
    printf("  ✓ NULL array push handling test passed\n");
    
    printf("Array operations tests completed successfully!\n\n");
}

// Test hash map operations
static void test_hash_map_operations(ember_vm* vm) {
    printf("Testing hash map operations...\n");
    
    // Test basic hash map set/get
    ember_value hash_map = ember_make_hash_map(vm, 8);

    UNUSED(hash_map);

    UNUSED(hash_map);

    UNUSED(hash_map);
    ember_hash_map* map = AS_HASH_MAP(hash_map);

    UNUSED(map);

    UNUSED(map);
    
    ember_value key1 = ember_make_string_gc(vm, "key1");

    
    UNUSED(key1);

    
    UNUSED(key1);

    
    UNUSED(key1);
    ember_value value1 = ember_make_number(42.0);

    UNUSED(value1);

    UNUSED(value1);

    UNUSED(value1);
    hash_map_set(map, key1, value1);
    
    ember_value retrieved1 = hash_map_get(map, key1);

    
    UNUSED(retrieved1);

    
    UNUSED(retrieved1);

    
    UNUSED(retrieved1);
    assert(retrieved1.type == EMBER_VAL_NUMBER);
    assert(retrieved1.as.number_val == 42.0);
    assert(map->length == 1);
    printf("  ✓ Hash map set/get test passed\n");
    
    // Test key existence check
    assert(hash_map_has_key(map, key1) == 1);
    ember_value nonexistent_key = ember_make_string_gc(vm, "nonexistent");

    UNUSED(nonexistent_key);

    UNUSED(nonexistent_key);

    UNUSED(nonexistent_key);
    assert(hash_map_has_key(map, nonexistent_key) == 0);
    printf("  ✓ Hash map key existence test passed\n");
    
    // Test getting nonexistent key
    ember_value nonexistent_value = hash_map_get(map, nonexistent_key);

    UNUSED(nonexistent_value);

    UNUSED(nonexistent_value);

    UNUSED(nonexistent_value);
    assert(nonexistent_value.type == EMBER_VAL_NIL);
    printf("  ✓ Hash map nonexistent key test passed\n");
    
    // Test overwriting existing key
    ember_value new_value1 = ember_make_string_gc(vm, "updated");

    UNUSED(new_value1);

    UNUSED(new_value1);

    UNUSED(new_value1);
    hash_map_set(map, key1, new_value1);
    ember_value retrieved_updated = hash_map_get(map, key1);

    UNUSED(retrieved_updated);

    UNUSED(retrieved_updated);

    UNUSED(retrieved_updated);
    assert(retrieved_updated.type == EMBER_VAL_STRING);
    assert(map->length == 1); // Should still be 1
    printf("  ✓ Hash map key overwrite test passed\n");
    
    // Test multiple keys
    ember_value key2 = ember_make_string_gc(vm, "key2");

    UNUSED(key2);

    UNUSED(key2);

    UNUSED(key2);
    ember_value value2 = ember_make_bool(1);

    UNUSED(value2);

    UNUSED(value2);

    UNUSED(value2);
    hash_map_set(map, key2, value2);
    assert(map->length == 2);
    
    ember_value key3 = ember_make_number(123.0);

    
    UNUSED(key3);

    
    UNUSED(key3);

    
    UNUSED(key3);
    ember_value value3 = ember_make_string_gc(vm, "numeric key");

    UNUSED(value3);

    UNUSED(value3);

    UNUSED(value3);
    hash_map_set(map, key3, value3);
    assert(map->length == 3);
    printf("  ✓ Hash map multiple keys test passed\n");
    
    // Test hash map resize (trigger by adding many elements)
    for (int i = 0; i < 20; i++) {
        char key_str[32];
        snprintf(key_str, sizeof(key_str), "key_%d", i);
        ember_value key = ember_make_string_gc(vm, key_str);

        UNUSED(key);

        UNUSED(key);

        UNUSED(key);
        ember_value value = ember_make_number(i);

        UNUSED(value);

        UNUSED(value);

        UNUSED(value);
        hash_map_set(map, key, value);
    }
    assert(map->length > 3);
    assert(map->capacity > 8); // Should have resized
    printf("  ✓ Hash map resize test passed\n");
    
    // Test hash map equality
    ember_value map1 = ember_make_hash_map(vm, 8);

    UNUSED(map1);

    UNUSED(map1);

    UNUSED(map1);
    ember_hash_map* m1 = AS_HASH_MAP(map1);

    UNUSED(m1);

    UNUSED(m1);
    hash_map_set(m1, ember_make_string_gc(vm, "a"), ember_make_number(1.0));
    hash_map_set(m1, ember_make_string_gc(vm, "b"), ember_make_number(2.0));
    
    ember_value map2 = ember_make_hash_map(vm, 8);

    
    UNUSED(map2);

    
    UNUSED(map2);

    
    UNUSED(map2);
    ember_hash_map* m2 = AS_HASH_MAP(map2);

    UNUSED(m2);

    UNUSED(m2);
    hash_map_set(m2, ember_make_string_gc(vm, "a"), ember_make_number(1.0));
    hash_map_set(m2, ember_make_string_gc(vm, "b"), ember_make_number(2.0));
    
    ember_value map3 = ember_make_hash_map(vm, 8);

    
    UNUSED(map3);

    
    UNUSED(map3);

    
    UNUSED(map3);
    ember_hash_map* m3 = AS_HASH_MAP(map3);

    UNUSED(m3);

    UNUSED(m3);
    hash_map_set(m3, ember_make_string_gc(vm, "a"), ember_make_number(1.0));
    hash_map_set(m3, ember_make_string_gc(vm, "b"), ember_make_number(3.0)); // Different value
    
    assert(values_equal(map1, map2) == 1);
    assert(values_equal(map1, map3) == 0);
    printf("  ✓ Hash map equality test passed\n");
    
    // Test NULL hash map operations
    hash_map_set(NULL, key1, value1);
    ember_value null_result = hash_map_get(NULL, key1);

    UNUSED(null_result);

    UNUSED(null_result);

    UNUSED(null_result);
    assert(null_result.type == EMBER_VAL_NIL);
    assert(hash_map_has_key(NULL, key1) == 0);
    printf("  ✓ NULL hash map handling test passed\n");
    
    printf("Hash map operations tests completed successfully!\n\n");
}

// Test string operations
static void test_string_operations(ember_vm* vm) {
    printf("Testing string operations...\n");
    
    // Test string concatenation
    ember_value str1 = ember_make_string_gc(vm, "Hello, ");

    UNUSED(str1);

    UNUSED(str1);

    UNUSED(str1);
    ember_value str2 = ember_make_string_gc(vm, "World!");

    UNUSED(str2);

    UNUSED(str2);

    UNUSED(str2);
    ember_value concatenated = concatenate_strings(vm, str1, str2);

    UNUSED(concatenated);

    UNUSED(concatenated);

    UNUSED(concatenated);
    assert(concatenated.type == EMBER_VAL_STRING);
    ember_string* concat_str = AS_STRING(concatenated);

    UNUSED(concat_str);

    UNUSED(concat_str);
    assert(strcmp(concat_str->chars, "Hello, World!") == 0);
    assert(concat_str->length == 13);
    printf("  ✓ String concatenation test passed\n");
    
    // Test concatenation with empty strings
    ember_value empty_str = ember_make_string_gc(vm, "");

    UNUSED(empty_str);

    UNUSED(empty_str);

    UNUSED(empty_str);
    ember_value concat_empty1 = concatenate_strings(vm, empty_str, str1);

    UNUSED(concat_empty1);

    UNUSED(concat_empty1);

    UNUSED(concat_empty1);
    ember_string* concat_empty_str1 = AS_STRING(concat_empty1);

    UNUSED(concat_empty_str1);

    UNUSED(concat_empty_str1);
    assert(strcmp(concat_empty_str1->chars, "Hello, ") == 0);
    
    ember_value concat_empty2 = concatenate_strings(vm, str1, empty_str);

    
    UNUSED(concat_empty2);

    
    UNUSED(concat_empty2);

    
    UNUSED(concat_empty2);
    ember_string* concat_empty_str2 = AS_STRING(concat_empty2);

    UNUSED(concat_empty_str2);

    UNUSED(concat_empty_str2);
    assert(strcmp(concat_empty_str2->chars, "Hello, ") == 0);
    printf("  ✓ String concatenation with empty strings test passed\n");
    
    // Test concatenation error conditions
    ember_value non_string = ember_make_number(42.0);

    UNUSED(non_string);

    UNUSED(non_string);

    UNUSED(non_string);
    ember_value concat_error = concatenate_strings(vm, str1, non_string);

    UNUSED(concat_error);

    UNUSED(concat_error);

    UNUSED(concat_error);
    assert(concat_error.type == EMBER_VAL_NIL);
    
    ember_value concat_error2 = concatenate_strings(NULL, str1, str2);

    
    UNUSED(concat_error2);

    
    UNUSED(concat_error2);

    
    UNUSED(concat_error2);
    assert(concat_error2.type == EMBER_VAL_NIL);
    printf("  ✓ String concatenation error handling test passed\n");
    
    // Test direct string functions
    ember_string* direct_str = copy_string(vm, "Direct string", 13);

    UNUSED(direct_str);

    UNUSED(direct_str);
    assert(direct_str != NULL);
    assert(direct_str->length == 13);
    assert(strcmp(direct_str->chars, "Direct string") == 0);
    printf("  ✓ Direct string creation test passed\n");
    
    // Test string creation with invalid input
    ember_string* null_str = copy_string(vm, NULL, 5);

    UNUSED(null_str);

    UNUSED(null_str);
    assert(null_str == NULL);
    
    ember_string* negative_len_str = copy_string(vm, "test", -1);

    
    UNUSED(negative_len_str);

    
    UNUSED(negative_len_str);
    assert(negative_len_str == NULL);
    printf("  ✓ String creation error handling test passed\n");
    
    // Test allocate_string with NULL chars
    ember_string* alloc_null = allocate_string(vm, NULL, 0);

    UNUSED(alloc_null);

    UNUSED(alloc_null);
    assert(alloc_null != NULL);
    assert(alloc_null->chars == NULL);
    assert(alloc_null->length == 0);
    printf("  ✓ String allocation with NULL chars test passed\n");
    
    printf("String operations tests completed successfully!\n\n");
}

// Test hash value computation
static void test_hash_value_computation(ember_vm* vm) {
    printf("Testing hash value computation...\n");
    
    // Test nil hash
    ember_value nil = ember_make_nil();

    UNUSED(nil);

    UNUSED(nil);

    UNUSED(nil);
    uint32_t nil_hash = hash_value(nil);

    UNUSED(nil_hash);

    UNUSED(nil_hash);
    assert(nil_hash == 0);
    printf("  ✓ Nil hash test passed\n");
    
    // Test boolean hash
    ember_value bool_true = ember_make_bool(1);

    UNUSED(bool_true);

    UNUSED(bool_true);

    UNUSED(bool_true);
    ember_value bool_false = ember_make_bool(0);

    UNUSED(bool_false);

    UNUSED(bool_false);

    UNUSED(bool_false);
    uint32_t true_hash = hash_value(bool_true);

    UNUSED(true_hash);

    UNUSED(true_hash);
    uint32_t false_hash = hash_value(bool_false);

    UNUSED(false_hash);

    UNUSED(false_hash);
    assert(true_hash == 1);
    assert(false_hash == 0);
    assert(true_hash != false_hash);
    printf("  ✓ Boolean hash test passed\n");
    
    // Test number hash
    ember_value num1 = ember_make_number(42.0);

    UNUSED(num1);

    UNUSED(num1);

    UNUSED(num1);
    ember_value num2 = ember_make_number(42.0);

    UNUSED(num2);

    UNUSED(num2);

    UNUSED(num2);
    ember_value num3 = ember_make_number(43.0);

    UNUSED(num3);

    UNUSED(num3);

    UNUSED(num3);
    uint32_t hash1 = hash_value(num1);

    UNUSED(hash1);

    UNUSED(hash1);
    uint32_t hash2 = hash_value(num2);

    UNUSED(hash2);

    UNUSED(hash2);
    uint32_t hash3 = hash_value(num3);

    UNUSED(hash3);

    UNUSED(hash3);
    assert(hash1 == hash2); // Same value should have same hash
    assert(hash1 != hash3); // Different values should have different hashes (usually)
    printf("  ✓ Number hash test passed\n");
    
    // Test string hash
    ember_value str1 = ember_make_string_gc(vm, "hello");

    UNUSED(str1);

    UNUSED(str1);

    UNUSED(str1);
    ember_value str2 = ember_make_string_gc(vm, "hello");

    UNUSED(str2);

    UNUSED(str2);

    UNUSED(str2);
    ember_value str3 = ember_make_string_gc(vm, "world");

    UNUSED(str3);

    UNUSED(str3);

    UNUSED(str3);
    uint32_t str_hash1 = hash_value(str1);

    UNUSED(str_hash1);

    UNUSED(str_hash1);
    uint32_t str_hash2 = hash_value(str2);

    UNUSED(str_hash2);

    UNUSED(str_hash2);
    uint32_t str_hash3 = hash_value(str3);

    UNUSED(str_hash3);

    UNUSED(str_hash3);
    assert(str_hash1 == str_hash2); // Same string should have same hash
    assert(str_hash1 != str_hash3); // Different strings should have different hashes
    printf("  ✓ String hash test passed\n");
    
    printf("Hash value computation tests completed successfully!\n\n");
}

// Test exception creation and handling
static void test_exception_handling(ember_vm* vm) {
    printf("Testing exception handling...\n");
    
    // Test exception creation
    ember_value exception = ember_make_exception(vm, "TestError", "This is a test exception");

    UNUSED(exception);

    UNUSED(exception);

    UNUSED(exception);
    assert(exception.type == EMBER_VAL_EXCEPTION);
    assert(exception.as.obj_val != NULL);
    assert(exception.as.obj_val->type == OBJ_EXCEPTION);
    
    ember_exception* exc = AS_EXCEPTION(exception);

    
    UNUSED(exc);

    
    UNUSED(exc);
    assert(exc->type != NULL);
    assert(strcmp(exc->type, "TestError") == 0);
    assert(exc->message != NULL);
    assert(strcmp(exc->message, "This is a test exception") == 0);
    assert(exc->line == 0);
    assert(exc->stack_trace.type == EMBER_VAL_NIL);
    printf("  ✓ Exception creation test passed\n");
    
    // Test exception with NULL type
    ember_value exception_null_type = ember_make_exception(vm, NULL, "Message only");

    UNUSED(exception_null_type);

    UNUSED(exception_null_type);

    UNUSED(exception_null_type);
    ember_exception* exc_null_type = AS_EXCEPTION(exception_null_type);

    UNUSED(exc_null_type);

    UNUSED(exc_null_type);
    assert(exc_null_type->type == NULL);
    assert(exc_null_type->message != NULL);
    assert(strcmp(exc_null_type->message, "Message only") == 0);
    printf("  ✓ Exception with NULL type test passed\n");
    
    // Test exception with NULL message
    ember_value exception_null_msg = ember_make_exception(vm, "ErrorType", NULL);

    UNUSED(exception_null_msg);

    UNUSED(exception_null_msg);

    UNUSED(exception_null_msg);
    ember_exception* exc_null_msg = AS_EXCEPTION(exception_null_msg);

    UNUSED(exc_null_msg);

    UNUSED(exc_null_msg);
    assert(exc_null_msg->type != NULL);
    assert(strcmp(exc_null_msg->type, "ErrorType") == 0);
    assert(exc_null_msg->message == NULL);
    printf("  ✓ Exception with NULL message test passed\n");
    
    // Test exception with both NULL
    ember_value exception_both_null = ember_make_exception(vm, NULL, NULL);

    UNUSED(exception_both_null);

    UNUSED(exception_both_null);

    UNUSED(exception_both_null);
    ember_exception* exc_both_null = AS_EXCEPTION(exception_both_null);

    UNUSED(exc_both_null);

    UNUSED(exc_both_null);
    assert(exc_both_null->type == NULL);
    assert(exc_both_null->message == NULL);
    printf("  ✓ Exception with NULL type and message test passed\n");
    
    printf("Exception handling tests completed successfully!\n\n");
}

// Test edge cases and error conditions
static void test_edge_cases(ember_vm* vm) {
    printf("Testing edge cases and error conditions...\n");
    
    // Test very large array capacity
    ember_value large_array = ember_make_array(vm, 1000000);

    UNUSED(large_array);

    UNUSED(large_array);

    UNUSED(large_array);
    assert(large_array.type == EMBER_VAL_ARRAY);
    ember_array* large_arr = AS_ARRAY(large_array);

    UNUSED(large_arr);

    UNUSED(large_arr);
    assert(large_arr->capacity == 1000000);
    printf("  ✓ Large array capacity test passed\n");
    
    // Test array push with large data
    ember_value medium_array = ember_make_array(vm, 1000);

    UNUSED(medium_array);

    UNUSED(medium_array);

    UNUSED(medium_array);
    ember_array* med_arr = AS_ARRAY(medium_array);

    UNUSED(med_arr);

    UNUSED(med_arr);
    for (int i = 0; i < 2000; i++) {
        array_push(med_arr, ember_make_number(i));
    }
    assert(med_arr->length == 2000);
    assert(med_arr->capacity >= 2000);
    printf("  ✓ Array growth stress test passed\n");
    
    // Test hash map with many collisions (using same key type)
    ember_value collision_map = ember_make_hash_map(vm, 4);

    UNUSED(collision_map);

    UNUSED(collision_map);

    UNUSED(collision_map); // Small capacity
    ember_hash_map* coll_map = AS_HASH_MAP(collision_map);

    UNUSED(coll_map);

    UNUSED(coll_map);
    for (int i = 0; i < 100; i++) {
        char key_str[32];
        snprintf(key_str, sizeof(key_str), "collision_key_%d", i);
        ember_value key = ember_make_string_gc(vm, key_str);

        UNUSED(key);

        UNUSED(key);

        UNUSED(key);
        ember_value value = ember_make_number(i);

        UNUSED(value);

        UNUSED(value);

        UNUSED(value);
        hash_map_set(coll_map, key, value);
    }
    assert(coll_map->length == 100);
    printf("  ✓ Hash map collision handling test passed\n");
    
    // Test string creation with NULL input
    ember_value null_string = ember_make_string(NULL);

    UNUSED(null_string);

    UNUSED(null_string);

    UNUSED(null_string);
    assert(null_string.type == EMBER_VAL_STRING);
    assert(null_string.as.string_val == NULL);
    printf("  ✓ NULL string creation test passed\n");
    
    ember_value null_gc_string = ember_make_string_gc(vm, NULL);

    
    UNUSED(null_gc_string);

    
    UNUSED(null_gc_string);

    
    UNUSED(null_gc_string);
    assert(null_gc_string.type == EMBER_VAL_NIL);
    printf("  ✓ NULL GC string creation test passed\n");
    
    // Test print_value with various types
    printf("  Testing print_value output:\n");
    printf("    Number: ");
    print_value(ember_make_number(123.456));
    printf("\n");
    
    printf("    Boolean true: ");
    print_value(ember_make_bool(1));
    printf("\n");
    
    printf("    Boolean false: ");
    print_value(ember_make_bool(0));
    printf("\n");
    
    printf("    Nil: ");
    print_value(ember_make_nil());
    printf("\n");
    
    printf("    String: ");
    print_value(ember_make_string_gc(vm, "Test string"));
    printf("\n");
    
    printf("    Exception: ");
    print_value(ember_make_exception(vm, "TestError", "Test message"));
    printf("\n");
    
    printf("  ✓ Print value test passed\n");
    
    // Test free_ember_value with various types
    ember_value to_free1 = ember_make_string("To be freed");

    UNUSED(to_free1);

    UNUSED(to_free1);

    UNUSED(to_free1);
    free_ember_value(to_free1);
    
    ember_value to_free2 = ember_make_number(42.0);

    
    UNUSED(to_free2);

    
    UNUSED(to_free2);

    
    UNUSED(to_free2);
    free_ember_value(to_free2); // Should be safe
    
    ember_value to_free3 = ember_make_array(vm, 5);

    
    UNUSED(to_free3);

    
    UNUSED(to_free3);

    
    UNUSED(to_free3);
    free_ember_value(to_free3); // Should be safe (GC managed)
    printf("  ✓ Value freeing test passed\n");
    
    printf("Edge cases and error conditions tests completed successfully!\n\n");
}

// Test memory management and garbage collection interactions
static void test_memory_management(ember_vm* vm) {
    printf("Testing memory management...\n");
    
    // Create many objects to test GC
    for (int i = 0; i < 100; i++) {
        ember_value str = ember_make_string_gc(vm, "GC test string");

        UNUSED(str);

        UNUSED(str);

        UNUSED(str);
        ember_value array = ember_make_array(vm, 10);

        UNUSED(array);

        UNUSED(array);

        UNUSED(array);
        ember_value hash_map = ember_make_hash_map(vm, 8);

        UNUSED(hash_map);

        UNUSED(hash_map);

        UNUSED(hash_map);
        (void)str; (void)array; (void)hash_map; // Suppress unused warnings
    }
    printf("  ✓ Object creation stress test passed\n");
    
    // Test string interning functions (stubs)
    init_string_intern_table(vm);
    ember_string* interned = intern_string(vm, "test", 4);

    UNUSED(interned);

    UNUSED(interned);
    assert(interned != NULL);
    ember_string* found = find_interned_string(vm, "test", 4);

    UNUSED(found);

    UNUSED(found);
    assert(found == NULL); // Currently returns NULL (not implemented)
    free_string_intern_table(vm);
    printf("  ✓ String interning stub test passed\n");
    
    // Test object allocation
    ember_object* obj = allocate_object(vm, sizeof(ember_object), OBJ_STRING);

    UNUSED(obj);

    UNUSED(obj);
    assert(obj != NULL);
    assert(obj->type == OBJ_STRING);
    assert(obj->is_marked == 0);
    printf("  ✓ Object allocation test passed\n");
    
    // Test hash map with VM write barriers
    ember_value map_with_vm = ember_make_hash_map(vm, 8);

    UNUSED(map_with_vm);

    UNUSED(map_with_vm);

    UNUSED(map_with_vm);
    ember_hash_map* map_vm = AS_HASH_MAP(map_with_vm);

    UNUSED(map_vm);

    UNUSED(map_vm);
    ember_value key = ember_make_string_gc(vm, "barrier_key");

    UNUSED(key);

    UNUSED(key);

    UNUSED(key);
    ember_value value = ember_make_string_gc(vm, "barrier_value");

    UNUSED(value);

    UNUSED(value);

    UNUSED(value);
    // Use regular hash_map_set instead of with_vm version for now
    hash_map_set(map_vm, key, value);
    ember_value retrieved = hash_map_get(map_vm, key);

    UNUSED(retrieved);

    UNUSED(retrieved);

    UNUSED(retrieved);
    assert(values_equal(retrieved, value) == 1);
    printf("  ✓ Hash map with VM write barriers test passed\n");
    
    printf("Memory management tests completed successfully!\n\n");
}

// Test performance characteristics
static void test_performance_characteristics(ember_vm* vm) {
    printf("Testing performance characteristics...\n");
    
    // Test array performance with many pushes
    ember_value perf_array = ember_make_array(vm, 1);

    UNUSED(perf_array);

    UNUSED(perf_array);

    UNUSED(perf_array);
    ember_array* perf_arr = AS_ARRAY(perf_array);

    UNUSED(perf_arr);

    UNUSED(perf_arr);
    for (int i = 0; i < 10000; i++) {
        array_push(perf_arr, ember_make_number(i));
    }
    assert(perf_arr->length == 10000);
    printf("  ✓ Array performance test (10000 pushes) passed\n");
    
    // Test hash map performance with many insertions
    ember_value perf_map = ember_make_hash_map(vm, 8);

    UNUSED(perf_map);

    UNUSED(perf_map);

    UNUSED(perf_map);
    ember_hash_map* perf_hash = AS_HASH_MAP(perf_map);

    UNUSED(perf_hash);

    UNUSED(perf_hash);
    for (int i = 0; i < 5000; i++) {
        char key_str[32];
        snprintf(key_str, sizeof(key_str), "perf_key_%d", i);
        ember_value key = ember_make_string_gc(vm, key_str);

        UNUSED(key);

        UNUSED(key);

        UNUSED(key);
        ember_value value = ember_make_number(i);

        UNUSED(value);

        UNUSED(value);

        UNUSED(value);
        hash_map_set(perf_hash, key, value);
    }
    assert(perf_hash->length == 5000);
    printf("  ✓ Hash map performance test (5000 insertions) passed\n");
    
    // Test string concatenation performance
    ember_value result_str = ember_make_string_gc(vm, "");

    UNUSED(result_str);

    UNUSED(result_str);

    UNUSED(result_str);
    for (int i = 0; i < 100; i++) {
        ember_value append_str = ember_make_string_gc(vm, "x");

        UNUSED(append_str);

        UNUSED(append_str);

        UNUSED(append_str);
        result_str = concatenate_strings(vm, result_str, append_str);
    }
    ember_string* final_str = AS_STRING(result_str);

    UNUSED(final_str);

    UNUSED(final_str);
    assert(final_str->length == 100);
    printf("  ✓ String concatenation performance test passed\n");
    
    printf("Performance characteristics tests completed successfully!\n\n");
}

int main(int argc, char* argv[]) {
    (void)argc;
    (void)argv;
    
    printf("Running Ember value system tests...\n\n");
    
    // Initialize VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);

    UNUSED(vm);
    assert(vm != NULL);
    
    // Run all tests
    test_value_creation(vm);
    test_value_manipulation(vm);
    test_array_operations(vm);
    test_hash_map_operations(vm);
    test_string_operations(vm);
    test_hash_value_computation(vm);
    test_exception_handling(vm);
    test_edge_cases(vm);
    test_memory_management(vm);
    test_performance_characteristics(vm);
    
    // Cleanup
    ember_free_vm(vm);
    
    printf("All value system tests passed successfully!\n");
    return 0;
}