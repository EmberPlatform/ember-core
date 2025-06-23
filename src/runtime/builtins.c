#include "runtime.h"
#include "ember.h"
#include "../vm.h"
#include "value/value.h"
#include "stdlib_working.h"
#include "ember_native_functions.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Note: get_string_value is now provided by common.h as ember_get_string_value

// Note: str_case_cmp is now provided by common.h as ember_strcasecmp

// Native print function
ember_value ember_native_print(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" "); // Space separator between arguments
        
        if (argv[i].type == EMBER_VAL_NUMBER) {
            printf("%g", argv[i].as.number_val);
        } else if (argv[i].type == EMBER_VAL_STRING) {
            const char* str = ember_get_string_value(argv[i]);
            if (str) printf("%s", str);
        } else if (argv[i].type == EMBER_VAL_BOOL) {
            printf("%s", argv[i].as.bool_val ? "true" : "false");
        } else if (argv[i].type == EMBER_VAL_ARRAY) {
            ember_array* array = AS_ARRAY(argv[i]);
            printf("[");
            for (int j = 0; j < array->length; j++) {
                if (j > 0) printf(", ");
                print_value(array->elements[j]);
            }
            printf("]");
        } else if (argv[i].type == EMBER_VAL_HASH_MAP) {
            ember_hash_map* map = AS_HASH_MAP(argv[i]);
            printf("{");
            int first = 1;
            for (int j = 0; j < map->capacity; j++) {
                if (map->entries[j].is_occupied) {
                    if (!first) printf(", ");
                    print_value(map->entries[j].key);
                    printf(": ");
                    print_value(map->entries[j].value);
                    first = 0;
                }
            }
            printf("}");
        } else {
            printf("nil");
        }
    }
    printf("\n"); // Single newline at the end
    return ember_make_nil();
}

// Type checking functions
ember_value ember_native_type(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "type", 1, argc);
    }
    
    switch (argv[0].type) {
        case EMBER_VAL_NUMBER: return ember_make_string("number");
        case EMBER_VAL_BOOL: return ember_make_string("bool");
        case EMBER_VAL_STRING: return ember_make_string("string");
        case EMBER_VAL_FUNCTION: return ember_make_string("function");
        case EMBER_VAL_NATIVE: return ember_make_string("native");
        case EMBER_VAL_ARRAY: return ember_make_string("array");
        case EMBER_VAL_HASH_MAP: return ember_make_string("hash_map");
        default: return ember_make_string("nil");
    }
}

// Boolean utility functions
ember_value ember_native_not(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "not", 1, argc);
    }
    
    // Convert value to boolean and negate
    int is_truthy = 0;
    switch (argv[0].type) {
        case EMBER_VAL_BOOL:
            is_truthy = argv[0].as.bool_val;
            break;
        case EMBER_VAL_NUMBER:
            is_truthy = argv[0].as.number_val != 0.0;
            break;
        case EMBER_VAL_NIL:
            is_truthy = 0;
            break;
        default:
            is_truthy = 1; // Non-nil, non-zero values are truthy
            break;
    }
    
    return ember_make_bool(!is_truthy);
}

// Type conversion functions for security (explicit, no automatic coercion)
ember_value ember_native_str(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) return ember_make_nil();
    
    switch (argv[0].type) {
        case EMBER_VAL_NUMBER: {
            char buffer[64];
            snprintf(buffer, sizeof(buffer), "%g", argv[0].as.number_val);
            return ember_make_string_gc(vm, buffer);
        }
        case EMBER_VAL_BOOL:
            return ember_make_string_gc(vm, argv[0].as.bool_val ? "true" : "false");
        case EMBER_VAL_STRING: {
            // Return copy of existing string
            const char* str = ember_get_string_value(argv[0]);
            return str ? ember_make_string_gc(vm, str) : ember_make_string_gc(vm, "");
        }
        case EMBER_VAL_NIL:
            return ember_make_string_gc(vm, "nil");
        case EMBER_VAL_FUNCTION:
            return ember_make_string_gc(vm, "function");
        case EMBER_VAL_NATIVE:
            return ember_make_string_gc(vm, "native");
        case EMBER_VAL_ARRAY: {
            char buffer[32];
            ember_array* arr = AS_ARRAY(argv[0]);
            snprintf(buffer, sizeof(buffer), "array[%d]", arr->length);
            return ember_make_string_gc(vm, buffer);
        }
        default:
            return ember_make_string_gc(vm, "unknown");
    }
}

ember_value ember_native_num(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "num", 1, argc);
    }
    
    switch (argv[0].type) {
        case EMBER_VAL_NUMBER:
            return argv[0]; // Already a number
        case EMBER_VAL_STRING: {
            const char* str = ember_get_string_value(argv[0]);
            if (!str) return ember_make_nil();
            
            // Skip leading whitespace
            while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
            if (*str == '\0') return ember_make_nil(); // Empty or whitespace-only
            
            char* endptr;
            double value = strtod(str, &endptr);
            
            // Check if entire string was consumed (valid number)
            while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' || *endptr == '\r') endptr++;
            if (*endptr != '\0') return ember_make_nil(); // Invalid number format
            
            return ember_make_number(value);
        }
        case EMBER_VAL_BOOL:
            return ember_make_number(argv[0].as.bool_val ? 1.0 : 0.0);
        default:
            return ember_make_nil(); // Cannot convert to number
    }
}

ember_value ember_native_int(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "int", 1, argc);
    }
    
    switch (argv[0].type) {
        case EMBER_VAL_NUMBER: {
            double num = argv[0].as.number_val;
            return ember_make_number(floor(num)); // Truncate to integer
        }
        case EMBER_VAL_STRING: {
            const char* str = ember_get_string_value(argv[0]);
            if (!str) return ember_make_nil();
            
            // Skip leading whitespace
            while (*str == ' ' || *str == '\t' || *str == '\n' || *str == '\r') str++;
            if (*str == '\0') return ember_make_nil();
            
            char* endptr;
            long value = strtol(str, &endptr, 10);
            
            // Check if entire string was consumed
            while (*endptr == ' ' || *endptr == '\t' || *endptr == '\n' || *endptr == '\r') endptr++;
            if (*endptr != '\0') return ember_make_nil();
            
            return ember_make_number((double)value);
        }
        case EMBER_VAL_BOOL:
            return ember_make_number(argv[0].as.bool_val ? 1.0 : 0.0);
        default:
            return ember_make_nil();
    }
}

ember_value ember_native_bool(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "bool", 1, argc);
    }
    
    switch (argv[0].type) {
        case EMBER_VAL_BOOL:
            return argv[0]; // Already a boolean
        case EMBER_VAL_NUMBER:
            return ember_make_bool(argv[0].as.number_val != 0.0);
        case EMBER_VAL_STRING: {
            const char* str = ember_get_string_value(argv[0]);
            if (!str) return ember_make_bool(0);
            
            // Case-insensitive comparison
            if (ember_strcasecmp(str, "true") == 0 || strcmp(str, "1") == 0) {
                return ember_make_bool(1);
            } else if (ember_strcasecmp(str, "false") == 0 || strcmp(str, "0") == 0 || *str == '\0') {
                return ember_make_bool(0);
            } else {
                return ember_make_bool(1); // Non-empty strings are truthy
            }
        }
        case EMBER_VAL_NIL:
            return ember_make_bool(0);
        default:
            return ember_make_bool(1); // Non-nil values are truthy
    }
}

// Function to register all built-in functions with the VM
void register_builtin_functions(ember_vm* vm) {
    // Built-in functions from runtime/builtins.c
    ember_register_func(vm, "print", ember_native_print);
    ember_register_func(vm, "type", ember_native_type);
    ember_register_func(vm, "not", ember_native_not);
    ember_register_func(vm, "str", ember_native_str);
    ember_register_func(vm, "num", ember_native_num);
    ember_register_func(vm, "int", ember_native_int);
    ember_register_func(vm, "bool", ember_native_bool);
    
    // Math functions from runtime/math_stdlib.c
    ember_register_func(vm, "abs", ember_native_abs);
    ember_register_func(vm, "sqrt", ember_native_sqrt);
    ember_register_func(vm, "max", ember_native_max);
    ember_register_func(vm, "min", ember_native_min);
    ember_register_func(vm, "floor", ember_native_floor);
    ember_register_func(vm, "ceil", ember_native_ceil);
    ember_register_func(vm, "round", ember_native_round);
    ember_register_func(vm, "pow", ember_native_pow);
    
    // String functions from runtime/string_stdlib.c
    ember_register_func(vm, "len", ember_native_len);
    ember_register_func(vm, "substr", ember_native_substr);
    ember_register_func(vm, "split", ember_native_split);
    ember_register_func(vm, "join", ember_native_join);
    ember_register_func(vm, "starts_with", ember_native_starts_with);
    ember_register_func(vm, "ends_with", ember_native_ends_with);
    
    // File I/O functions (working implementations)
    ember_register_func(vm, "read_file", ember_native_read_file_working);
    ember_register_func(vm, "write_file", ember_native_write_file_working);
    ember_register_func(vm, "append_file", ember_native_append_file_working);
    ember_register_func(vm, "file_exists", ember_native_file_exists_working);
    
    // JSON functions from runtime/json_simple.c (working implementations)
    ember_register_func(vm, "json_parse", ember_json_parse_working);
    ember_register_func(vm, "json_stringify", ember_json_stringify_working);
    ember_register_func(vm, "json_validate", ember_json_validate_working);
    
    // Cryptographic functions from runtime/crypto_simple.c (working implementations)
    ember_register_func(vm, "sha256", ember_native_sha256_working);
    ember_register_func(vm, "sha512", ember_native_sha512_working);
    ember_register_func(vm, "hmac_sha256", ember_native_hmac_sha256_working);
    ember_register_func(vm, "secure_random", ember_native_secure_random_working);
    // Additional crypto functions (some may need testing)
    // ember_register_func(vm, "hmac_sha512", ember_native_hmac_sha512);
    // ember_register_func(vm, "secure_compare", ember_native_secure_compare);
    // ember_register_func(vm, "bcrypt_hash", ember_native_bcrypt_hash);
    // ember_register_func(vm, "bcrypt_verify", ember_native_bcrypt_verify);
    
    // Regular expression functions from stdlib/regex_native.c (temporarily disabled)
    // ember_register_func(vm, "regex_match", ember_regex_match_func);
    // ember_register_func(vm, "regex_test", ember_regex_test);
    // ember_register_func(vm, "regex_replace", ember_regex_replace);
    // ember_register_func(vm, "regex_split", ember_regex_split);
    // ember_register_func(vm, "regex_find_all", ember_regex_find_all);
    
    // Date/time functions from stdlib/datetime_native.c (may need testing)
    // ember_register_func(vm, "datetime_now", ember_native_datetime_now);
    
    // Note: HTTP, WebSocket, upload, streaming, router, session, and template functions 
    // temporarily disabled due to integration issues - focus on core stdlib first
}