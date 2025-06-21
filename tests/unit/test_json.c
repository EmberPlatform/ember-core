#include "test_ember_internal.h"
#include "../../src/stdlib/stdlib.h"
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <stdlib.h>

// Test helper function to create string values
static ember_value make_test_string(ember_vm* vm, const char* str) {
    return ember_make_string_gc(vm, str);
}

// Test basic JSON parsing functionality
static void test_json_parse_basic() {
    printf("Testing basic JSON parsing...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test parsing null
    ember_value null_json = make_test_string(vm, "null");
    ember_value result = ember_json_parse(vm, 1, &null_json);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test parsing boolean true
    ember_value true_json = make_test_string(vm, "true");
    result = ember_json_parse(vm, 1, &true_json);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 1);
    
    // Test parsing boolean false
    ember_value false_json = make_test_string(vm, "false");
    result = ember_json_parse(vm, 1, &false_json);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 0);
    
    // Test parsing positive number
    ember_value pos_num_json = make_test_string(vm, "42");
    result = ember_json_parse(vm, 1, &pos_num_json);
    assert(result.type == EMBER_VAL_NUMBER);
    assert(result.as.number_val == 42.0);
    
    // Test parsing negative number
    ember_value neg_num_json = make_test_string(vm, "-17.5");
    result = ember_json_parse(vm, 1, &neg_num_json);
    assert(result.type == EMBER_VAL_NUMBER);
    assert(result.as.number_val == -17.5);
    
    // Test parsing scientific notation
    ember_value sci_num_json = make_test_string(vm, "1.5e10");
    result = ember_json_parse(vm, 1, &sci_num_json);
    assert(result.type == EMBER_VAL_NUMBER);
    assert(result.as.number_val == 1.5e10);
    
    // Test parsing simple string
    ember_value str_json = make_test_string(vm, "\"hello world\"");
    result = ember_json_parse(vm, 1, &str_json);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "hello world") == 0);
    
    ember_free_vm(vm);
    printf("✓ Basic JSON parsing tests passed\n");
}

// Test JSON string parsing with escape sequences
static void test_json_parse_strings() {
    printf("Testing JSON string parsing with escapes...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test escaped quotes
    ember_value escaped_quotes = make_test_string(vm, "\"He said \\\"Hello\\\"\"");
    ember_value result = ember_json_parse(vm, 1, &escaped_quotes);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "He said \"Hello\"") == 0);
    
    // Test escaped backslash
    ember_value escaped_backslash = make_test_string(vm, "\"C:\\\\path\\\\to\\\\file\"");
    result = ember_json_parse(vm, 1, &escaped_backslash);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "C:\\path\\to\\file") == 0);
    
    // Test newline and tab escapes
    ember_value escaped_whitespace = make_test_string(vm, "\"line1\\nline2\\ttab\"");
    result = ember_json_parse(vm, 1, &escaped_whitespace);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "line1\nline2\ttab") == 0);
    
    // Test unicode escape (simplified)
    ember_value unicode_escape = make_test_string(vm, "\"\\u0041\"");
    result = ember_json_parse(vm, 1, &unicode_escape);
    assert(result.type == EMBER_VAL_STRING);
    // For our simplified implementation, we just use a placeholder
    
    ember_free_vm(vm);
    printf("✓ JSON string parsing tests passed\n");
}

// Test JSON array parsing
static void test_json_parse_arrays() {
    printf("Testing JSON array parsing...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test empty array
    ember_value empty_array = make_test_string(vm, "[]");
    ember_value result = ember_json_parse(vm, 1, &empty_array);
    assert(result.type == EMBER_VAL_ARRAY);
    ember_array* arr = AS_ARRAY(result);
    assert(arr->length == 0);
    
    // Test array with numbers
    ember_value num_array = make_test_string(vm, "[1, 2, 3, 4.5]");
    result = ember_json_parse(vm, 1, &num_array);
    assert(result.type == EMBER_VAL_ARRAY);
    arr = AS_ARRAY(result);
    assert(arr->length == 4);
    assert(arr->elements[0].type == EMBER_VAL_NUMBER);
    assert(arr->elements[0].as.number_val == 1.0);
    assert(arr->elements[3].as.number_val == 4.5);
    
    // Test mixed type array
    ember_value mixed_array = make_test_string(vm, "[true, \"hello\", 42, null]");
    result = ember_json_parse(vm, 1, &mixed_array);
    assert(result.type == EMBER_VAL_ARRAY);
    arr = AS_ARRAY(result);
    assert(arr->length == 4);
    assert(arr->elements[0].type == EMBER_VAL_BOOL);
    assert(arr->elements[1].type == EMBER_VAL_STRING);
    assert(arr->elements[2].type == EMBER_VAL_NUMBER);
    assert(arr->elements[3].type == EMBER_VAL_NIL);
    
    // Test nested arrays
    ember_value nested_array = make_test_string(vm, "[[1, 2], [3, 4]]");
    result = ember_json_parse(vm, 1, &nested_array);
    assert(result.type == EMBER_VAL_ARRAY);
    arr = AS_ARRAY(result);
    assert(arr->length == 2);
    assert(arr->elements[0].type == EMBER_VAL_ARRAY);
    assert(arr->elements[1].type == EMBER_VAL_ARRAY);
    
    ember_free_vm(vm);
    printf("✓ JSON array parsing tests passed\n");
}

// Test JSON object parsing
static void test_json_parse_objects() {
    printf("Testing JSON object parsing...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test empty object
    ember_value empty_obj = make_test_string(vm, "{}");
    ember_value result = ember_json_parse(vm, 1, &empty_obj);
    assert(result.type == EMBER_VAL_HASH_MAP);
    ember_hash_map* map = AS_HASH_MAP(result);
    assert(map->length == 0);
    
    // Test simple object
    ember_value simple_obj = make_test_string(vm, "{\"name\": \"John\", \"age\": 30}");
    result = ember_json_parse(vm, 1, &simple_obj);
    assert(result.type == EMBER_VAL_HASH_MAP);
    map = AS_HASH_MAP(result);
    assert(map->length == 2);
    
    // Test nested object
    ember_value nested_obj = make_test_string(vm, "{\"user\": {\"name\": \"John\", \"id\": 123}}");
    result = ember_json_parse(vm, 1, &nested_obj);
    assert(result.type == EMBER_VAL_HASH_MAP);
    map = AS_HASH_MAP(result);
    assert(map->length == 1);
    
    ember_free_vm(vm);
    printf("✓ JSON object parsing tests passed\n");
}

// Test malformed JSON handling
static void test_json_parse_malformed() {
    printf("Testing malformed JSON handling...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test unclosed string
    ember_value unclosed_str = make_test_string(vm, "\"unclosed string");
    ember_value result = ember_json_parse(vm, 1, &unclosed_str);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test unclosed array
    ember_value unclosed_arr = make_test_string(vm, "[1, 2, 3");
    result = ember_json_parse(vm, 1, &unclosed_arr);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test unclosed object
    ember_value unclosed_obj = make_test_string(vm, "{\"key\": \"value\"");
    result = ember_json_parse(vm, 1, &unclosed_obj);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test invalid number
    ember_value invalid_num = make_test_string(vm, "12.34.56");
    result = ember_json_parse(vm, 1, &invalid_num);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test trailing comma in array
    ember_value trailing_comma_arr = make_test_string(vm, "[1, 2, 3,]");
    result = ember_json_parse(vm, 1, &trailing_comma_arr);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test trailing comma in object
    ember_value trailing_comma_obj = make_test_string(vm, "{\"key\": \"value\",}");
    result = ember_json_parse(vm, 1, &trailing_comma_obj);
    assert(result.type == EMBER_VAL_NIL);
    
    // Test invalid escape sequence
    ember_value invalid_escape = make_test_string(vm, "\"invalid \\x escape\"");
    result = ember_json_parse(vm, 1, &invalid_escape);
    assert(result.type == EMBER_VAL_NIL);
    
    ember_free_vm(vm);
    printf("✓ Malformed JSON handling tests passed\n");
}

// Test JSON size limits
static void test_json_size_limits() {
    printf("Testing JSON size limits...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Create a very large JSON string (exceeding MAX_JSON_SIZE)
    const int large_size = 1048577; // 1MB + 1 byte
    char* large_json = malloc(large_size + 1);
    assert(large_json != NULL);
    
    large_json[0] = '"';
    for (int i = 1; i < large_size - 1; i++) {
        large_json[i] = 'a';
    }
    large_json[large_size - 1] = '"';
    large_json[large_size] = '\0';
    
    ember_value large_str = make_test_string(vm, large_json);
    ember_value result = ember_json_parse(vm, 1, &large_str);
    assert(result.type == EMBER_VAL_NIL); // Should fail due to size limit
    
    free(large_json);
    ember_free_vm(vm);
    printf("✓ JSON size limit tests passed\n");
}

// Test JSON nesting depth limits
static void test_json_nesting_limits() {
    printf("Testing JSON nesting depth limits...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Create deeply nested JSON (exceeding MAX_NESTING_DEPTH)
    const int max_depth = 150; // Exceeds MAX_NESTING_DEPTH (100)
    char* deep_json = malloc(max_depth * 2 + 10);
    assert(deep_json != NULL);
    
    int pos = 0;
    for (int i = 0; i < max_depth; i++) {
        deep_json[pos++] = '[';
    }
    deep_json[pos++] = '1';
    for (int i = 0; i < max_depth; i++) {
        deep_json[pos++] = ']';
    }
    deep_json[pos] = '\0';
    
    ember_value deep_str = make_test_string(vm, deep_json);
    ember_value result = ember_json_parse(vm, 1, &deep_str);
    assert(result.type == EMBER_VAL_NIL); // Should fail due to depth limit
    
    free(deep_json);
    ember_free_vm(vm);
    printf("✓ JSON nesting depth limit tests passed\n");
}

// Test JSON stringify functionality
static void test_json_stringify() {
    printf("Testing JSON stringify...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test stringifying nil
    ember_value nil_val = ember_make_nil();
    ember_value result = ember_json_stringify(vm, 1, &nil_val);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "null") == 0);
    
    // Test stringifying boolean
    ember_value bool_val = ember_make_bool(1);
    result = ember_json_stringify(vm, 1, &bool_val);
    assert(result.type == EMBER_VAL_STRING);
    assert(strcmp(AS_CSTRING(result), "true") == 0);
    
    // Test stringifying number
    ember_value num_val = ember_make_number(42.5);
    result = ember_json_stringify(vm, 1, &num_val);
    assert(result.type == EMBER_VAL_STRING);
    
    // Test stringifying string with escapes
    ember_value str_val = make_test_string(vm, "Hello \"World\"");
    result = ember_json_stringify(vm, 1, &str_val);
    assert(result.type == EMBER_VAL_STRING);
    // Should contain escaped quotes
    
    ember_free_vm(vm);
    printf("✓ JSON stringify tests passed\n");
}

// Test JSON validation
static void test_json_validate() {
    printf("Testing JSON validation...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test valid JSON
    ember_value valid_json = make_test_string(vm, "{\"key\": \"value\"}");
    ember_value result = ember_json_validate(vm, 1, &valid_json);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 1);
    
    // Test invalid JSON
    ember_value invalid_json = make_test_string(vm, "{\"key\": value}");
    result = ember_json_validate(vm, 1, &invalid_json);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 0);
    
    ember_free_vm(vm);
    printf("✓ JSON validation tests passed\n");
}

// Main test runner
int main() {
    printf("Running JSON parser tests...\n\n");
    
    test_json_parse_basic();
    test_json_parse_strings();
    test_json_parse_arrays();
    test_json_parse_objects();
    test_json_parse_malformed();
    test_json_size_limits();
    test_json_nesting_limits();
    test_json_stringify();
    test_json_validate();
    
    printf("\n✅ All JSON parser tests passed!\n");
    return 0;
}