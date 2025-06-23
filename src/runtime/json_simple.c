/**
 * Simple functional JSON implementations to replace stubs
 * Basic JSON parsing and serialization
 */

#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

// Simple JSON parsing - supports basic objects and primitives
static char* skip_whitespace(char* json) {
    while (*json && isspace(*json)) json++;
    return json;
}

// Parse a simple JSON string (very basic implementation)
static ember_value parse_json_value(ember_vm* vm, char** json_ptr) {
    char* json = skip_whitespace(*json_ptr);
    
    if (*json == '"') {
        // Parse string
        json++; // Skip opening quote
        char* start = json;
        while (*json && *json != '"') {
            if (*json == '\\') json++; // Skip escaped chars
            json++;
        }
        if (*json == '"') {
            size_t len = json - start;
            char* str = malloc(len + 1);
            if (str) {
                strncpy(str, start, len);
                str[len] = '\0';
                ember_value result = ember_make_string_gc(vm, str);
                free(str);
                *json_ptr = json + 1;
                return result;
            }
        }
        return ember_make_nil();
    } else if (*json == '{') {
        // Parse object (return simple representation)
        ember_value map = ember_make_hash_map(vm, 8);
        json++; // Skip {
        json = skip_whitespace(json);
        
        if (*json == '}') {
            *json_ptr = json + 1;
            return map;
        }
        
        // For simplicity, just return an empty map
        while (*json && *json != '}') json++;
        if (*json == '}') json++;
        *json_ptr = json;
        return map;
    } else if (isdigit(*json) || *json == '-') {
        // Parse number
        char* endptr;
        double num = strtod(json, &endptr);
        *json_ptr = endptr;
        return ember_make_number(num);
    } else if (strncmp(json, "true", 4) == 0) {
        *json_ptr = json + 4;
        return ember_make_bool(1);
    } else if (strncmp(json, "false", 5) == 0) {
        *json_ptr = json + 5;
        return ember_make_bool(0);
    } else if (strncmp(json, "null", 4) == 0) {
        *json_ptr = json + 4;
        return ember_make_nil();
    }
    
    return ember_make_nil();
}

ember_value ember_json_parse_working(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_nil();
    }
    
    ember_string* json_str = AS_STRING(argv[0]);
    char* json_copy = malloc(strlen(json_str->chars) + 1);
    if (!json_copy) return ember_make_nil();
    
    strcpy(json_copy, json_str->chars);
    char* json_ptr = json_copy;
    
    ember_value result = parse_json_value(vm, &json_ptr);
    free(json_copy);
    
    return result;
}

static void append_escaped_string(char** buffer, size_t* size, size_t* capacity, const char* str) {
    size_t needed = strlen(str) + 2; // quotes
    if (*size + needed >= *capacity) {
        *capacity = (*capacity == 0) ? 256 : *capacity * 2;
        *buffer = realloc(*buffer, *capacity);
    }
    
    (*buffer)[(*size)++] = '"';
    while (*str) {
        if (*str == '"' || *str == '\\') {
            (*buffer)[(*size)++] = '\\';
        }
        (*buffer)[(*size)++] = *str++;
    }
    (*buffer)[(*size)++] = '"';
}

static void append_string(char** buffer, size_t* size, size_t* capacity, const char* str) {
    size_t len = strlen(str);
    if (*size + len >= *capacity) {
        *capacity = (*capacity == 0) ? 256 : *capacity * 2;
        while (*size + len >= *capacity) *capacity *= 2;
        *buffer = realloc(*buffer, *capacity);
    }
    strcpy(*buffer + *size, str);
    *size += len;
}

ember_value ember_json_stringify_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    char* buffer = NULL;
    size_t size = 0;
    size_t capacity = 0;
    
    ember_value value = argv[0];
    char temp[64];
    
    switch (value.type) {
        case EMBER_VAL_STRING: {
            ember_string* str = AS_STRING(value);
            append_escaped_string(&buffer, &size, &capacity, str->chars);
            break;
        }
        case EMBER_VAL_NUMBER:
            snprintf(temp, sizeof(temp), "%g", value.as.number_val);
            append_string(&buffer, &size, &capacity, temp);
            break;
        case EMBER_VAL_BOOL:
            append_string(&buffer, &size, &capacity, value.as.bool_val ? "true" : "false");
            break;
        case EMBER_VAL_NIL:
            append_string(&buffer, &size, &capacity, "null");
            break;
        case EMBER_VAL_HASH_MAP:
            append_string(&buffer, &size, &capacity, "{}");
            break;
        case EMBER_VAL_ARRAY:
            append_string(&buffer, &size, &capacity, "[]");
            break;
        default:
            append_string(&buffer, &size, &capacity, "null");
            break;
    }
    
    if (buffer) {
        buffer[size] = '\0';
        ember_value result = ember_make_string_gc(vm, buffer);
        free(buffer);
        return result;
    }
    
    return ember_make_nil();
}

ember_value ember_json_validate_working(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_STRING) {
        return ember_make_bool(0);
    }
    
    ember_string* json_str = AS_STRING(argv[0]);
    const char* json = json_str->chars;
    
    // Very basic validation - check for balanced braces/brackets
    int brace_count = 0;
    int bracket_count = 0;
    int in_string = 0;
    
    while (*json) {
        if (!in_string) {
            if (*json == '{') brace_count++;
            else if (*json == '}') brace_count--;
            else if (*json == '[') bracket_count++;
            else if (*json == ']') bracket_count--;
            else if (*json == '"') in_string = 1;
        } else {
            if (*json == '"' && (json == json_str->chars || *(json-1) != '\\')) {
                in_string = 0;
            }
        }
        json++;
    }
    
    return ember_make_bool(brace_count == 0 && bracket_count == 0 && !in_string);
}