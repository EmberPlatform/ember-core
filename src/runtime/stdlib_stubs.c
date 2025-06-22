#include "ember.h"
#include <math.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <ctype.h>

// String utility functions (ember_get_string_value defined in template_stubs.c)
const char* ember_get_string_data(ember_value value) {
    if (value.type == EMBER_VAL_STRING) {
        ember_string* str = AS_STRING(value);
        return str->chars;
    }
    return NULL;
}

int ember_strcasecmp(const char* a, const char* b) {
    if (!a || !b) return (a == b) ? 0 : (a ? 1 : -1);
    
    while (*a && *b) {
        int diff = tolower(*a) - tolower(*b);
        if (diff != 0) return diff;
        a++;
        b++;
    }
    return tolower(*a) - tolower(*b);
}

// Math functions
ember_value ember_native_abs(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(fabs(argv[0].as.number_val));
}

ember_value ember_native_sqrt(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(sqrt(argv[0].as.number_val));
}

ember_value ember_native_floor(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(floor(argv[0].as.number_val));
}

ember_value ember_native_ceil(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(ceil(argv[0].as.number_val));
}

ember_value ember_native_round(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(round(argv[0].as.number_val));
}

ember_value ember_native_pow(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(pow(argv[0].as.number_val, argv[1].as.number_val));
}

ember_value ember_native_max(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    double a = argv[0].as.number_val;
    double b = argv[1].as.number_val;
    return ember_make_number(a > b ? a : b);
}

ember_value ember_native_min(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    double a = argv[0].as.number_val;
    double b = argv[1].as.number_val;
    return ember_make_number(a < b ? a : b);
}

// String functions
ember_value ember_native_len(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) return ember_make_nil();
    
    if (argv[0].type == EMBER_VAL_STRING) {
        ember_string* str = AS_STRING(argv[0]);
        return ember_make_number(str->length);
    } else if (argv[0].type == EMBER_VAL_ARRAY) {
        ember_array* arr = AS_ARRAY(argv[0]);
        return ember_make_number(arr->length);
    }
    return ember_make_nil();
}

ember_value ember_native_substr(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc < 2 || argc > 3 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    
    ember_string* str = AS_STRING(argv[0]);
    int start = (int)argv[1].as.number_val;
    int length = (argc == 3 && argv[2].type == EMBER_VAL_NUMBER) ? 
                 (int)argv[2].as.number_val : str->length - start;
    
    if (start < 0 || start >= str->length || length <= 0) {
        return ember_make_string("");
    }
    
    if (start + length > str->length) {
        length = str->length - start;
    }
    
    // Create substring
    char* substr = malloc(length + 1);
    strncpy(substr, str->chars + start, length);
    substr[length] = '\0';
    ember_value result = ember_make_string(substr);
    free(substr);
    return result;
}

ember_value ember_native_split(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* str = AS_STRING(argv[0]);
    ember_string* delimiter = AS_STRING(argv[1]);
    
    // Create result array - simplified implementation
    ember_value result = ember_make_array(vm, 10); // Initial capacity
    ember_array* arr = AS_ARRAY(result);
    
    if (delimiter->length == 0) {
        // Add whole string if delimiter is empty
        if (arr->length < arr->capacity) {
            arr->elements[arr->length++] = argv[0];
        }
        return result;
    }
    
    const char* current = str->chars;
    const char* end = str->chars + str->length;
    
    while (current < end) {
        const char* found = strstr(current, delimiter->chars);
        if (!found) {
            // Add remaining string
            int remaining_len = end - current;
            if (remaining_len > 0) {
                char* part_str = malloc(remaining_len + 1);
                strncpy(part_str, current, remaining_len);
                part_str[remaining_len] = '\0';
                ember_value part = ember_make_string(part_str);
                free(part_str);
                
                if (arr->length < arr->capacity) {
                    arr->elements[arr->length++] = part;
                }
            }
            break;
        }
        
        // Add part before delimiter
        int part_len = found - current;
        if (part_len > 0) {
            char* part_str = malloc(part_len + 1);
            strncpy(part_str, current, part_len);
            part_str[part_len] = '\0';
            ember_value part = ember_make_string(part_str);
            free(part_str);
            
            if (arr->length < arr->capacity) {
                arr->elements[arr->length++] = part;
            }
        }
        
        current = found + delimiter->length;
    }
    
    return result;
}

ember_value ember_native_join(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_ARRAY || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_array* arr = AS_ARRAY(argv[0]);
    ember_string* separator = AS_STRING(argv[1]);
    
    if (arr->length == 0) {
        return ember_make_string("");
    }
    
    // Calculate total length needed
    int total_length = 0;
    for (int i = 0; i < arr->length; i++) {
        if (arr->elements[i].type == EMBER_VAL_STRING) {
            ember_string* str = AS_STRING(arr->elements[i]);
            total_length += str->length;
        }
        if (i > 0) total_length += separator->length;
    }
    
    char* result = malloc(total_length + 1);
    result[0] = '\0';
    
    for (int i = 0; i < arr->length; i++) {
        if (i > 0) {
            strcat(result, separator->chars);
        }
        
        if (arr->elements[i].type == EMBER_VAL_STRING) {
            ember_string* str = AS_STRING(arr->elements[i]);
            strncat(result, str->chars, str->length);
        }
    }
    
    ember_value ret = ember_make_string(result);
    free(result);
    return ret;
}

ember_value ember_native_starts_with(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* str = AS_STRING(argv[0]);
    ember_string* prefix = AS_STRING(argv[1]);
    
    if (prefix->length > str->length) {
        return ember_make_bool(0);
    }
    
    int result = strncmp(str->chars, prefix->chars, prefix->length) == 0;
    return ember_make_bool(result);
}

ember_value ember_native_ends_with(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_STRING || argv[1].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* str = AS_STRING(argv[0]);
    ember_string* suffix = AS_STRING(argv[1]);
    
    if (suffix->length > str->length) {
        return ember_make_bool(0);
    }
    
    const char* start = str->chars + str->length - suffix->length;
    int result = strncmp(start, suffix->chars, suffix->length) == 0;
    return ember_make_bool(result);
}

// File I/O functions (stubs)
ember_value ember_native_read_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] read_file function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_write_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] write_file function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_append_file(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] append_file function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_file_exists(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] file_exists function not implemented\n");
    return ember_make_bool(0);
}

// JSON functions (stubs)
ember_value ember_json_parse(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] json_parse function not implemented\n");
    return ember_make_nil();
}

ember_value ember_json_stringify(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] json_stringify function not implemented\n");
    return ember_make_nil();
}

ember_value ember_json_validate(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] json_validate function not implemented\n");
    return ember_make_bool(0);
}

// Crypto functions (stubs)
ember_value ember_native_sha256(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] sha256 function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_sha512(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] sha512 function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_hmac_sha256(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] hmac_sha256 function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_hmac_sha512(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] hmac_sha512 function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_secure_random(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] secure_random function not implemented\n");
    return ember_make_nil();
}

ember_value ember_native_secure_compare(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    printf("[STUB] secure_compare function not implemented\n");
    return ember_make_bool(0);
}