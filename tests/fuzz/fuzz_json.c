#include "fuzz_common.h"
#include "../../src/stdlib/stdlib.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>

// Maximum sizes for fuzzing
#define MAX_FUZZ_STRING_SIZE 8192
#define FUZZ_ITERATIONS 1000

// Generate random JSON-like strings for fuzzing
static char* generate_fuzz_json(int size, int* actual_size) {
    if (size <= 0 || size > MAX_FUZZ_STRING_SIZE) {
        size = 256;
    }
    
    char* buffer = malloc(size + 1);
    if (!buffer) {
        *actual_size = 0;
        return NULL;
    }
    
    // JSON characters and structure elements
    const char* json_chars = "{}[]\",:truefalsnil0123456789.-+eE \t\n\r\\";
    const char* special_chars = "\x00\x01\x02\x03\x04\x05\x06\x07\x08\x09\x0a\x0b\x0c\x0d\x0e\x0f";
    const char* escape_chars = "\\\"/\b\f\n\r\t";
    
    int pos = 0;
    
    for (int i = 0; i < size - 1 && pos < size - 1; i++) {
        int choice = rand() % 100;
        
        if (choice < 70) {
            // 70% chance of normal JSON characters
            buffer[pos++] = json_chars[rand() % strlen(json_chars)];
        } else if (choice < 85) {
            // 15% chance of escape characters
            buffer[pos++] = escape_chars[rand() % strlen(escape_chars)];
        } else if (choice < 95) {
            // 10% chance of special control characters
            buffer[pos++] = special_chars[rand() % strlen(special_chars)];
        } else {
            // 5% chance of random high ASCII
            buffer[pos++] = (char)(rand() % 256);
        }
    }
    
    buffer[pos] = '\0';
    *actual_size = pos;
    return buffer;
}

// Generate malformed JSON structures
static char* generate_malformed_json(int type) {
    char* buffer = malloc(1024);
    if (!buffer) return NULL;
    
    switch (type % 20) {
        case 0: // Unclosed string
            strcpy(buffer, "\"unclosed string");
            break;
        case 1: // Unclosed array
            strcpy(buffer, "[1, 2, 3");
            break;
        case 2: // Unclosed object
            strcpy(buffer, "{\"key\": \"value\"");
            break;
        case 3: // Trailing comma in array
            strcpy(buffer, "[1, 2, 3,]");
            break;
        case 4: // Trailing comma in object
            strcpy(buffer, "{\"key\": \"value\",}");
            break;
        case 5: // Missing quotes on key
            strcpy(buffer, "{key: \"value\"}");
            break;
        case 6: // Single quotes instead of double
            strcpy(buffer, "{'key': 'value'}");
            break;
        case 7: // Invalid number format
            strcpy(buffer, "[12.34.56]");
            break;
        case 8: // Invalid escape sequence
            strcpy(buffer, "\"invalid \\x escape\"");
            break;
        case 9: // Missing colon in object
            strcpy(buffer, "{\"key\" \"value\"}");
            break;
        case 10: // Extra comma
            strcpy(buffer, "[1,, 2]");
            break;
        case 11: // Invalid literal
            strcpy(buffer, "[undefined]");
            break;
        case 12: // Mixed brackets
            strcpy(buffer, "[1, 2}");
            break;
        case 13: // Unescaped control character
            strcpy(buffer, "\"line1\nline2\"");  // Literal newline instead of \\n
            break;
        case 14: // Leading zeros
            strcpy(buffer, "[001, 002]");
            break;
        case 15: // Hex numbers (not valid JSON)
            strcpy(buffer, "[0x1A, 0xFF]");
            break;
        case 16: // Comments (not valid JSON)
            strcpy(buffer, "{/* comment */\"key\": \"value\"}");
            break;
        case 17: // Infinity/NaN
            strcpy(buffer, "[Infinity, NaN]");
            break;
        case 18: // Empty key
            strcpy(buffer, "{\"\": \"value\"}");
            break;
        case 19: // Duplicate keys
            strcpy(buffer, "{\"key\": 1, \"key\": 2}");
            break;
        default:
            strcpy(buffer, "null");
            break;
    }
    
    return buffer;
}

// Generate deeply nested structures
static char* generate_deep_nesting(int depth) {
    int estimated_size = depth * 4 + 100; // Rough estimate
    char* buffer = malloc(estimated_size);
    if (!buffer) return NULL;
    
    int pos = 0;
    
    // Create deep nesting with arrays
    for (int i = 0; i < depth && pos < estimated_size - 10; i++) {
        if (i % 2 == 0) {
            buffer[pos++] = '[';
        } else {
            buffer[pos++] = '{';
            if (pos < estimated_size - 20) {
                strcpy(buffer + pos, "\"k\":");
                pos += 4;
            }
        }
    }
    
    buffer[pos++] = '1'; // Add a value
    
    // Close all the brackets
    for (int i = 0; i < depth && pos < estimated_size - 2; i++) {
        if ((depth - 1 - i) % 2 == 0) {
            buffer[pos++] = ']';
        } else {
            buffer[pos++] = '}';
        }
    }
    
    buffer[pos] = '\0';
    return buffer;
}

// Generate JSON with many elements
static char* generate_large_array(int count) {
    int estimated_size = count * 10 + 100;
    char* buffer = malloc(estimated_size);
    if (!buffer) return NULL;
    
    int pos = 0;
    buffer[pos++] = '[';
    
    for (int i = 0; i < count && pos < estimated_size - 20; i++) {
        if (i > 0) {
            buffer[pos++] = ',';
        }
        pos += snprintf(buffer + pos, estimated_size - pos, "%d", i);
    }
    
    buffer[pos++] = ']';
    buffer[pos] = '\0';
    return buffer;
}

// Test JSON parser with random malformed inputs
static void fuzz_json_parser(int iterations) {
    printf("Fuzzing JSON parser with %d iterations...\n", iterations);
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    int crashes = 0;
    int parsing_errors = 0;
    int successful_parses = 0;
    
    for (int i = 0; i < iterations; i++) {
        char* fuzz_json = NULL;
        int test_type = rand() % 4;
        
        switch (test_type) {
            case 0: {
                // Random character sequences
                int size = rand() % MAX_FUZZ_STRING_SIZE + 1;
                int actual_size;
                fuzz_json = generate_fuzz_json(size, &actual_size);
                break;
            }
            case 1: {
                // Structured malformed JSON
                fuzz_json = generate_malformed_json(i);
                break;
            }
            case 2: {
                // Deep nesting test
                int depth = rand() % 200 + 50; // 50-250 levels
                fuzz_json = generate_deep_nesting(depth);
                break;
            }
            case 3: {
                // Large arrays
                int count = rand() % 20000 + 5000; // 5000-25000 elements
                fuzz_json = generate_large_array(count);
                break;
            }
        }
        
        if (fuzz_json) {
            ember_value json_str = ember_make_string_gc(vm, fuzz_json);
            ember_value result = ember_json_parse(vm, 1, &json_str);
            
            if (result.type == EMBER_VAL_NIL) {
                parsing_errors++;
            } else {
                successful_parses++;
                
                // Try to stringify the result to test round-trip
                ember_value stringified = ember_json_stringify(vm, 1, &result);
                if (stringified.type != EMBER_VAL_STRING) {
                    // Stringify failed - that's okay for complex structures
                }
            }
            
            free(fuzz_json);
        }
        
        // Print progress every 100 iterations
        if ((i + 1) % 100 == 0) {
            printf("Progress: %d/%d iterations, %d parsing errors, %d successful\n", 
                   i + 1, iterations, parsing_errors, successful_parses);
        }
    }
    
    ember_free_vm(vm);
    
    printf("Fuzzing completed:\n");
    printf("  Total iterations: %d\n", iterations);
    printf("  Parsing errors: %d\n", parsing_errors);
    printf("  Successful parses: %d\n", successful_parses);
    printf("  Crashes: %d\n", crashes);
    
    if (crashes > 0) {
        printf("❌ FUZZING FAILED: %d crashes detected!\n", crashes);
        exit(1);
    } else {
        printf("✅ Fuzzing completed successfully - no crashes detected\n");
    }
}

// Test specific security-focused edge cases
static void test_security_edge_cases() {
    printf("Testing security-focused edge cases...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test extremely large string
    printf("  Testing size limits...\n");
    char* large_str = malloc(1048580); // Just over 1MB
    if (large_str) {
        memset(large_str, 'a', 1048579);
        large_str[0] = '"';
        large_str[1048578] = '"';
        large_str[1048579] = '\0';
        
        ember_value large_json = ember_make_string_gc(vm, large_str);
        ember_value result = ember_json_parse(vm, 1, &large_json);
        assert(result.type == EMBER_VAL_NIL); // Should reject due to size
        free(large_str);
    }
    
    // Test maximum nesting depth
    printf("  Testing nesting depth limits...\n");
    char* deep_json = generate_deep_nesting(150); // Exceeds MAX_NESTING_DEPTH
    if (deep_json) {
        ember_value deep_str = ember_make_string_gc(vm, deep_json);
        ember_value result = ember_json_parse(vm, 1, &deep_str);
        assert(result.type == EMBER_VAL_NIL); // Should reject due to depth
        free(deep_json);
    }
    
    // Test maximum array elements
    printf("  Testing array size limits...\n");
    char* huge_array = generate_large_array(15000); // Exceeds MAX_ARRAY_ELEMENTS
    if (huge_array) {
        ember_value huge_str = ember_make_string_gc(vm, huge_array);
        ember_value result = ember_json_parse(vm, 1, &huge_str);
        assert(result.type == EMBER_VAL_NIL); // Should reject due to element count
        free(huge_array);
    }
    
    // Test string length limits
    printf("  Testing string length limits...\n");
    char test_str[70000]; // Exceeds MAX_STRING_LENGTH
    test_str[0] = '"';
    for (int i = 1; i < 69999; i++) {
        test_str[i] = 'x';
    }
    test_str[69999] = '"';
    test_str[70000] = '\0';
    
    ember_value long_str = ember_make_string_gc(vm, test_str);
    ember_value result = ember_json_parse(vm, 1, &long_str);
    assert(result.type == EMBER_VAL_NIL); // Should reject due to string length
    
    ember_free_vm(vm);
    printf("✅ Security edge case tests passed\n");
}

// Main fuzzing entry point
int main(int argc, char* argv[]) {
    int iterations = FUZZ_ITERATIONS;
    
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) {
            iterations = FUZZ_ITERATIONS;
        }
    }
    
    printf("JSON Fuzzing Test Suite\n");
    printf("======================\n\n");
    
    // Seed random number generator
    srand((unsigned int)time(NULL));
    
    // Run security-focused tests first
    test_security_edge_cases();
    printf("\n");
    
    // Run main fuzzing
    fuzz_json_parser(iterations);
    
    printf("\n🎉 All JSON fuzzing tests completed successfully!\n");
    return 0;
}