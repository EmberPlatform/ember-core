/*
 * Standard Library Exception Integration Examples
 * 
 * This file shows how to integrate the enhanced exception handling system
 * with existing standard library functions. These are examples that can be
 * used to update actual standard library implementations.
 */

#include "ember.h"
#include "runtime/value/value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>

// Example: Enhanced file operations with proper exception handling
ember_value enhanced_file_read(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1) {
        ember_value err = ember_make_range_error(vm, "file_read requires at least 1 argument");
        return err;
    }
    
    const char* filename = ember_get_string_value(argv[0]);
    if (!filename) {
        return ember_make_type_error(vm, "file_read: filename must be a string");
    }
    
    FILE* file = fopen(filename, "r");
    if (!file) {
        char error_msg[512];
        snprintf(error_msg, sizeof(error_msg), 
                "Failed to open file '%s': %s", filename, strerror(errno));
        return ember_make_io_error(vm, error_msg);
    }
    
    // Get file size
    fseek(file, 0, SEEK_END);
    long file_size = ftell(file);
    fseek(file, 0, SEEK_SET);
    
    if (file_size < 0) {
        fclose(file);
        return ember_make_io_error(vm, "Failed to determine file size");
    }
    
    if (file_size > 10 * 1024 * 1024) { // 10MB limit
        fclose(file);
        char error_msg[256];
        snprintf(error_msg, sizeof(error_msg), 
                "File '%s' is too large (%ld bytes, max 10MB)", filename, file_size);
        return ember_make_range_error(vm, error_msg);
    }
    
    char* buffer = malloc(file_size + 1);
    if (!buffer) {
        fclose(file);
        return ember_make_memory_error(vm, "Failed to allocate memory for file contents");
    }
    
    size_t bytes_read = fread(buffer, 1, file_size, file);
    if (bytes_read != (size_t)file_size) {
        free(buffer);
        fclose(file);
        return ember_make_io_error(vm, "Failed to read complete file contents");
    }
    
    buffer[file_size] = '\0';
    fclose(file);
    
    ember_value result = ember_make_string_gc(vm, buffer);
    free(buffer);
    return result;
}

// Example: Enhanced array access with bounds checking
ember_value enhanced_array_get(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2) {
        return ember_make_range_error(vm, "array_get requires 2 arguments: array and index");
    }
    
    if (argv[0].type != EMBER_VAL_ARRAY) {
        return ember_make_type_error(vm, "array_get: first argument must be an array");
    }
    
    if (argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_type_error(vm, "array_get: second argument must be a number");
    }
    
    ember_array* array = AS_ARRAY(argv[0]);
    double index_double = argv[1].as.number_val;
    
    // Check if index is an integer
    if (index_double != floor(index_double)) {
        return ember_make_type_error(vm, "array_get: index must be an integer");
    }
    
    int index = (int)index_double;
    
    // Check bounds
    if (index < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "Array index %d is negative", index);
        return ember_make_range_error(vm, error_msg);
    }
    
    if (index >= array->length) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "Array index %d out of bounds for array of length %d", 
                index, array->length);
        return ember_make_range_error(vm, error_msg);
    }
    
    return array->elements[index];
}

// Example: Enhanced string operations with validation
ember_value enhanced_string_substring(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 2) {
        return ember_make_range_error(vm, "substring requires at least 2 arguments");
    }
    
    const char* str = ember_get_string_value(argv[0]);
    if (!str) {
        return ember_make_type_error(vm, "substring: first argument must be a string");
    }
    
    if (argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_type_error(vm, "substring: start index must be a number");
    }
    
    int str_len = strlen(str);
    int start = (int)argv[1].as.number_val;
    int end = str_len;
    
    if (argc >= 3) {
        if (argv[2].type != EMBER_VAL_NUMBER) {
            return ember_make_type_error(vm, "substring: end index must be a number");
        }
        end = (int)argv[2].as.number_val;
    }
    
    // Validate indices
    if (start < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "substring: start index %d is negative", start);
        return ember_make_range_error(vm, error_msg);
    }
    
    if (start > str_len) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "substring: start index %d exceeds string length %d", start, str_len);
        return ember_make_range_error(vm, error_msg);
    }
    
    if (end < start) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "substring: end index %d is less than start index %d", end, start);
        return ember_make_range_error(vm, error_msg);
    }
    
    if (end > str_len) {
        end = str_len; // Clamp to string length
    }
    
    int substring_len = end - start;
    char* substring = malloc(substring_len + 1);
    if (!substring) {
        return ember_make_memory_error(vm, "Failed to allocate memory for substring");
    }
    
    memcpy(substring, str + start, substring_len);
    substring[substring_len] = '\0';
    
    ember_value result = ember_make_string_gc(vm, substring);
    free(substring);
    return result;
}

// Example: Enhanced math operations with domain checking
ember_value enhanced_math_sqrt(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1) {
        return ember_make_range_error(vm, "sqrt requires 1 argument");
    }
    
    if (argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_type_error(vm, "sqrt: argument must be a number");
    }
    
    double value = argv[0].as.number_val;
    
    if (value < 0) {
        char error_msg[128];
        snprintf(error_msg, sizeof(error_msg), 
                "sqrt: cannot take square root of negative number %g", value);
        return ember_make_range_error(vm, error_msg);
    }
    
    if (isinf(value)) {
        return ember_make_range_error(vm, "sqrt: argument is infinite");
    }
    
    if (isnan(value)) {
        return ember_make_range_error(vm, "sqrt: argument is NaN");
    }
    
    return ember_make_number(sqrt(value));
}

// Example: Enhanced network operations with proper error handling
ember_value enhanced_http_get(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1) {
        return ember_make_range_error(vm, "http_get requires at least 1 argument");
    }
    
    const char* url = ember_get_string_value(argv[0]);
    if (!url) {
        return ember_make_type_error(vm, "http_get: URL must be a string");
    }
    
    // Validate URL format (simplified)
    if (strncmp(url, "http://", 7) != 0 && strncmp(url, "https://", 8) != 0) {
        return ember_make_type_error(vm, "http_get: URL must start with http:// or https://");
    }
    
    if (strlen(url) > 2048) {
        return ember_make_range_error(vm, "http_get: URL is too long (max 2048 characters)");
    }
    
    // Simulate network operation (in real implementation, this would make actual HTTP request)
    // For now, just return a mock response or appropriate error
    
    // Simulate different types of network errors
    if (strstr(url, "timeout")) {
        return ember_make_timeout_error(vm, "HTTP request timed out");
    }
    
    if (strstr(url, "unreachable")) {
        return ember_make_network_error(vm, "Host unreachable");
    }
    
    if (strstr(url, "forbidden")) {
        return ember_make_security_error(vm, "HTTP 403 Forbidden");
    }
    
    // Mock successful response
    return ember_make_string_gc(vm, "HTTP response data");
}

// Example: Enhanced JSON parsing with detailed error messages
ember_value enhanced_json_parse(ember_vm* vm, int argc, ember_value* argv) {
    if (argc < 1) {
        return ember_make_range_error(vm, "json_parse requires 1 argument");
    }
    
    const char* json_str = ember_get_string_value(argv[0]);
    if (!json_str) {
        return ember_make_type_error(vm, "json_parse: argument must be a string");
    }
    
    // Basic JSON validation (simplified)
    int len = strlen(json_str);
    if (len == 0) {
        return ember_make_syntax_error(vm, "json_parse: empty JSON string");
    }
    
    // Count braces and brackets for basic validation
    int brace_count = 0;
    int bracket_count = 0;
    int in_string = 0;
    int line = 1;
    int column = 1;
    
    for (int i = 0; i < len; i++) {
        char c = json_str[i];
        
        if (c == '\n') {
            line++;
            column = 1;
            continue;
        }
        column++;
        
        if (c == '"' && (i == 0 || json_str[i-1] != '\\')) {
            in_string = !in_string;
        }
        
        if (!in_string) {
            if (c == '{') brace_count++;
            else if (c == '}') brace_count--;
            else if (c == '[') bracket_count++;
            else if (c == ']') bracket_count--;
            
            if (brace_count < 0) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "json_parse: unexpected '}' at line %d, column %d", line, column);
                return ember_make_syntax_error(vm, error_msg);
            }
            
            if (bracket_count < 0) {
                char error_msg[256];
                snprintf(error_msg, sizeof(error_msg), 
                        "json_parse: unexpected ']' at line %d, column %d", line, column);
                return ember_make_syntax_error(vm, error_msg);
            }
        }
    }
    
    if (brace_count != 0) {
        return ember_make_syntax_error(vm, "json_parse: mismatched braces");
    }
    
    if (bracket_count != 0) {
        return ember_make_syntax_error(vm, "json_parse: mismatched brackets");
    }
    
    if (in_string) {
        return ember_make_syntax_error(vm, "json_parse: unterminated string");
    }
    
    // In a real implementation, this would parse the JSON and return the appropriate value
    // For now, return a simple success indication
    return ember_make_string_gc(vm, "Parsed JSON object");
}

/*
 * Example usage in standard library registration:
 * 
 * void register_enhanced_stdlib_functions(ember_vm* vm) {
 *     // File operations
 *     ember_register_func(vm, "file_read", enhanced_file_read);
 *     
 *     // Array operations
 *     ember_register_func(vm, "array_get", enhanced_array_get);
 *     
 *     // String operations
 *     ember_register_func(vm, "substring", enhanced_string_substring);
 *     
 *     // Math operations
 *     ember_register_func(vm, "sqrt", enhanced_math_sqrt);
 *     
 *     // Network operations
 *     ember_register_func(vm, "http_get", enhanced_http_get);
 *     
 *     // JSON operations
 *     ember_register_func(vm, "json_parse", enhanced_json_parse);
 * }
 */

/*
 * Guidelines for integrating exception handling in standard library functions:
 * 
 * 1. Parameter Validation:
 *    - Check argument count and types
 *    - Return TypeError for wrong types
 *    - Return RangeError for invalid argument counts
 * 
 * 2. Bounds Checking:
 *    - Validate array indices, string ranges
 *    - Return RangeError for out-of-bounds access
 * 
 * 3. Resource Management:
 *    - Use try/finally patterns for cleanup
 *    - Return IOError for file/network operations
 *    - Return MemoryError for allocation failures
 * 
 * 4. Domain Validation:
 *    - Check mathematical domains (sqrt of negative, log of zero)
 *    - Return RangeError for domain errors
 * 
 * 5. Security Constraints:
 *    - Validate file paths, URLs
 *    - Return SecurityError for permission violations
 * 
 * 6. Parsing and Syntax:
 *    - Provide detailed error messages with line/column info
 *    - Return SyntaxError for malformed input
 * 
 * 7. Network and I/O:
 *    - Handle timeouts, connection failures
 *    - Return NetworkError, TimeoutError, IOError appropriately
 * 
 * 8. Detailed Error Messages:
 *    - Include context (filenames, indices, values)
 *    - Help users understand what went wrong
 *    - Include suggestions for fixing the problem when possible
 */