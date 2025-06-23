/**
 * String standard library functions for Ember Core
 * Consolidated from ember-native with simplified error handling
 */

#include "ember.h"
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

// Helper function to get string data
static const char* ember_get_string_value(ember_value value) {
    if (value.type == EMBER_VAL_STRING) {
        ember_string* str = AS_STRING(value);
        return str->chars;
    }
    return NULL;
}

// String length function - supports strings, arrays, and hash maps
ember_value ember_native_len(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) return ember_make_nil();
    
    if (argv[0].type == EMBER_VAL_STRING) {
        const char* str = ember_get_string_value(argv[0]);
        if (!str) return ember_make_number(0);
        return ember_make_number((double)strlen(str));
    } else if (argv[0].type == EMBER_VAL_ARRAY) {
        ember_array* array = AS_ARRAY(argv[0]);
        return ember_make_number((double)array->length);
    } else if (argv[0].type == EMBER_VAL_HASH_MAP) {
        ember_hash_map* map = AS_HASH_MAP(argv[0]);
        return ember_make_number((double)map->length);
    } else {
        return ember_make_nil();
    }
}

// String substring function
ember_value ember_native_substr(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2 || argc > 3) return ember_make_nil();
    if (argv[0].type != EMBER_VAL_STRING) return ember_make_nil();
    if (argv[1].type != EMBER_VAL_NUMBER) return ember_make_nil();
    
    const char* str = ember_get_string_value(argv[0]);
    if (!str) return ember_make_nil();
    
    int start = (int)argv[1].as.number_val;
    int len = strlen(str);
    int substr_len = (argc == 3 && argv[2].type == EMBER_VAL_NUMBER) ? 
                     (int)argv[2].as.number_val : len - start;
    
    if (start < 0 || start >= len || substr_len <= 0) {
        return ember_make_string_gc(vm, "");
    }
    
    if (start + substr_len > len) {
        substr_len = len - start;
    }
    
    char* result = malloc(substr_len + 1);
    if (!result) return ember_make_nil();
    
    strncpy(result, str + start, substr_len);
    result[substr_len] = '\0';
    
    ember_value ret_val = ember_make_string_gc(vm, result);
    free(result);
    return ret_val;
}

// String split function - simple implementation
ember_value ember_native_split(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2) return ember_make_nil();
    if (argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    const char* str = ember_get_string_value(argv[0]);
    const char* delimiter = ember_get_string_value(argv[1]);
    if (!str || !delimiter) return ember_make_nil();
    
    ember_value array = ember_make_array(vm, 8);
    ember_array* arr = AS_ARRAY(array);
    
    size_t str_len = strlen(str);
    char* str_copy = malloc(str_len + 1);
    if (!str_copy) return ember_make_nil();
    
    strcpy(str_copy, str);
    
    char* token = strtok(str_copy, delimiter);
    while (token != NULL && arr->length < arr->capacity) {
        arr->elements[arr->length++] = ember_make_string_gc(vm, token);
        token = strtok(NULL, delimiter);
    }
    
    free(str_copy);
    return array;
}

// String join function - join array elements with delimiter
ember_value ember_native_join(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2) return ember_make_nil();
    if (argv[0].type != EMBER_VAL_ARRAY || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_array* arr = AS_ARRAY(argv[0]);
    const char* delimiter = ember_get_string_value(argv[1]);
    if (!delimiter) return ember_make_nil();
    
    if (arr->length == 0) {
        return ember_make_string_gc(vm, "");
    }
    
    // Calculate total length needed
    size_t total_len = 0;
    size_t delimiter_len = strlen(delimiter);
    
    for (int i = 0; i < arr->length; i++) {
        if (arr->elements[i].type == EMBER_VAL_STRING) {
            const char* str = ember_get_string_value(arr->elements[i]);
            if (str) {
                total_len += strlen(str);
                if (i < arr->length - 1) {
                    total_len += delimiter_len;
                }
            }
        }
    }
    
    // Security limit
    if (total_len > 1048576) return ember_make_nil(); // 1MB max
    
    char* result = malloc(total_len + 1);
    if (!result) return ember_make_nil();
    
    result[0] = '\0';
    for (int i = 0; i < arr->length; i++) {
        if (arr->elements[i].type == EMBER_VAL_STRING) {
            const char* str = ember_get_string_value(arr->elements[i]);
            if (str) {
                strcat(result, str);
                if (i < arr->length - 1) {
                    strcat(result, delimiter);
                }
            }
        }
    }
    
    ember_value ret_val = ember_make_string_gc(vm, result);
    free(result);
    return ret_val;
}

// String starts_with function
ember_value ember_native_starts_with(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2) return ember_make_bool(0);
    if (argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    const char* str = ember_get_string_value(argv[0]);
    const char* prefix = ember_get_string_value(argv[1]);
    if (!str || !prefix) return ember_make_bool(0);
    
    size_t prefix_len = strlen(prefix);
    if (prefix_len > strlen(str)) return ember_make_bool(0);
    
    return ember_make_bool(strncmp(str, prefix, prefix_len) == 0);
}

// String ends_with function
ember_value ember_native_ends_with(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2) return ember_make_bool(0);
    if (argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    const char* str = ember_get_string_value(argv[0]);
    const char* suffix = ember_get_string_value(argv[1]);
    if (!str || !suffix) return ember_make_bool(0);
    
    size_t str_len = strlen(str);
    size_t suffix_len = strlen(suffix);
    if (suffix_len > str_len) return ember_make_bool(0);
    
    const char* start_pos = str + str_len - suffix_len;
    return ember_make_bool(strcmp(start_pos, suffix) == 0);
}