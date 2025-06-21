#include "fuzz_common.h"

// Test modes
typedef enum {
    FUZZ_PARSER_ONLY,
    FUZZ_VM_ONLY,
    FUZZ_BOTH,
    FUZZ_INTEGRATION
} fuzz_mode;

// Test the parser with various input types
fuzz_result test_parser_comprehensive(const char* input) {
    ember_vm* vm = NULL;
    
    if (setup_timeout(FUZZ_TIMEOUT_SECONDS)) {
        return FUZZ_TIMEOUT;
    }
    
    vm = ember_new_vm();
    if (!vm) {
        clear_timeout();
        return FUZZ_ERROR;
    }
    
    // Test parser through ember_eval
    int result = ember_eval(vm, input);
    
    clear_timeout();
    ember_free_vm(vm);
    
    // ember_eval returns 0 on success, non-zero on error
    return (result == 0) ? FUZZ_SUCCESS : FUZZ_ERROR;
}

// Test VM with pre-built valid Ember code
fuzz_result test_vm_with_code(const char* code) {
    ember_vm* vm = NULL;
    
    if (setup_timeout(FUZZ_TIMEOUT_SECONDS)) {
        return FUZZ_TIMEOUT;
    }
    
    vm = ember_new_vm();
    if (!vm) {
        clear_timeout();
        return FUZZ_ERROR;
    }
    
    // Execute the code
    int result = ember_eval(vm, code);
    
    clear_timeout();
    ember_free_vm(vm);
    
    return (result == 0) ? FUZZ_SUCCESS : FUZZ_ERROR;
}

// Test integration: parser + VM execution
fuzz_result test_integration(const char* input) {
    // This is essentially the same as test_parser_comprehensive
    // but we could add more complex integration testing here
    return test_parser_comprehensive(input);
}

// Generate valid Ember code for VM testing
void generate_valid_ember_code(char* buffer, size_t max_size) {
    const char* valid_programs[] = {
        "1 + 2",
        "print(\"hello\")",
        "fn test() { return 42; }",
        "if (true) { print(\"yes\"); }",
        "while (false) { break; }",
        "for (i = 0; i < 5; i++) { print(i); }",
        "let x = 10; print(x);",
        "let arr = [1, 2, 3]; print(arr[0]);",
        "let map = {\"key\": \"value\"}; print(map[\"key\"]);",
        "fn factorial(n) { if (n <= 1) return 1; return n * factorial(n-1); } print(factorial(5));",
        "let a = 5; let b = 10; print(a + b);",
        "print(abs(-5));",
        "print(sqrt(16));",
        "print(max(1, 2, 3));",
        "print(type(\"string\"));",
        "let str = \"Hello World\"; print(len(str));",
        "true && false || true",
        "not false",
        "1 == 1 && 2 != 3",
        "\"string\" + \" concatenation\"",
    };
    
    int prog_idx = rand() % (sizeof(valid_programs) / sizeof(valid_programs[0]));
    strncpy(buffer, valid_programs[prog_idx], max_size - 1);
    buffer[max_size - 1] = '\0';
}

// Run parser fuzzing
int fuzz_parser_tests(int iterations, fuzz_stats* stats) {
    char input_buffer[FUZZ_MAX_INPUT_SIZE];
    
    printf("Running parser fuzzing tests...\n");
    
    for (int i = 0; i < iterations; i++) {
        if (i % 100 == 0) {
            printf("Parser progress: %d/%d\n", i, iterations);
        }
        
        // Generate different types of input
        int input_type = rand() % 6;
        
        switch (input_type) {
            case 0:
                generate_random_string(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 1:
                generate_ember_keywords(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 2:
                generate_malformed_syntax(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 3:
                generate_edge_case_numbers(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 4:
                generate_unicode_strings(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 5:
            default:
                generate_valid_ember_code(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
        }
        
        fuzz_result result = test_parser_comprehensive(input_buffer);
        update_stats(stats, result);
        
        if (result == FUZZ_CRASH) {
            printf("PARSER CRASH at iteration %d with input: '%.50s%s'\n",
                   i, input_buffer, strlen(input_buffer) > 50 ? "..." : "");
        }
        
        // Stop if too many crashes
        if (stats->crash_count > iterations / 20) {
            printf("Too many parser crashes, stopping early.\n");
            return i;
        }
    }
    
    return iterations;
}

// Run VM fuzzing
int fuzz_vm_tests(int iterations, fuzz_stats* stats) {
    char code_buffer[FUZZ_MAX_INPUT_SIZE];
    
    printf("Running VM fuzzing tests...\n");
    
    for (int i = 0; i < iterations; i++) {
        if (i % 100 == 0) {
            printf("VM progress: %d/%d\n", i, iterations);
        }
        
        // Generate different types of code
        int code_type = rand() % 4;
        
        switch (code_type) {
            case 0:
                generate_valid_ember_code(code_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 1:
                generate_ember_keywords(code_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 2:
                generate_malformed_syntax(code_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 3:
            default:
                generate_edge_case_numbers(code_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
        }
        
        fuzz_result result = test_vm_with_code(code_buffer);
        update_stats(stats, result);
        
        if (result == FUZZ_CRASH) {
            printf("VM CRASH at iteration %d with code: '%.50s%s'\n",
                   i, code_buffer, strlen(code_buffer) > 50 ? "..." : "");
        }
        
        // Stop if too many crashes
        if (stats->crash_count > iterations / 20) {
            printf("Too many VM crashes, stopping early.\n");
            return i;
        }
    }
    
    return iterations;
}

// Run integration fuzzing
int fuzz_integration_tests(int iterations, fuzz_stats* stats) {
    char input_buffer[FUZZ_MAX_INPUT_SIZE];
    
    printf("Running integration fuzzing tests...\n");
    
    for (int i = 0; i < iterations; i++) {
        if (i % 100 == 0) {
            printf("Integration progress: %d/%d\n", i, iterations);
        }
        
        // Focus on valid-looking code for integration testing
        int input_type = rand() % 3;
        
        switch (input_type) {
            case 0:
                generate_valid_ember_code(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 1:
                generate_ember_keywords(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
            case 2:
            default:
                generate_malformed_syntax(input_buffer, FUZZ_MAX_INPUT_SIZE);
                break;
        }
        
        fuzz_result result = test_integration(input_buffer);
        update_stats(stats, result);
        
        if (result == FUZZ_CRASH) {
            printf("INTEGRATION CRASH at iteration %d with input: '%.50s%s'\n",
                   i, input_buffer, strlen(input_buffer) > 50 ? "..." : "");
        }
        
        // Stop if too many crashes
        if (stats->crash_count > iterations / 20) {
            printf("Too many integration crashes, stopping early.\n");
            return i;
        }
    }
    
    return iterations;
}

// Main fuzzing function
int run_comprehensive_fuzz(fuzz_mode mode, int iterations) {
    fuzz_stats parser_stats = {0};
    fuzz_stats vm_stats = {0};
    fuzz_stats integration_stats = {0};
    int total_crashes = 0;
    
    printf("Starting comprehensive fuzzing...\n");
    printf("Mode: %s\n", 
           mode == FUZZ_PARSER_ONLY ? "Parser Only" :
           mode == FUZZ_VM_ONLY ? "VM Only" :
           mode == FUZZ_BOTH ? "Both Separately" : "Integration");
    printf("Iterations per component: %d\n", iterations);
    
    if (mode == FUZZ_PARSER_ONLY || mode == FUZZ_BOTH) {
        int completed = fuzz_parser_tests(iterations, &parser_stats);
        print_fuzz_stats("Parser", &parser_stats, completed);
        total_crashes += parser_stats.crash_count;
    }
    
    if (mode == FUZZ_VM_ONLY || mode == FUZZ_BOTH) {
        int completed = fuzz_vm_tests(iterations, &vm_stats);
        print_fuzz_stats("VM", &vm_stats, completed);
        total_crashes += vm_stats.crash_count;
    }
    
    if (mode == FUZZ_INTEGRATION) {
        int completed = fuzz_integration_tests(iterations, &integration_stats);
        print_fuzz_stats("Integration", &integration_stats, completed);
        total_crashes += integration_stats.crash_count;
    }
    
    printf("\n=== Overall Summary ===\n");
    printf("Total crashes detected: %d\n", total_crashes);
    
    return total_crashes;
}

int main(int argc, char* argv[]) {
    int iterations = FUZZ_ITERATIONS_DEFAULT;
    fuzz_mode mode = FUZZ_BOTH;
    
    // Parse command line arguments
    for (int i = 1; i < argc; i++) {
        if (strcmp(argv[i], "-n") == 0 && i + 1 < argc) {
            iterations = atoi(argv[i + 1]);
            if (iterations <= 0) {
                printf("Invalid iteration count: %s\n", argv[i + 1]);
                return 1;
            }
            i++; // Skip next argument
        } else if (strcmp(argv[i], "--parser") == 0) {
            mode = FUZZ_PARSER_ONLY;
        } else if (strcmp(argv[i], "--vm") == 0) {
            mode = FUZZ_VM_ONLY;
        } else if (strcmp(argv[i], "--integration") == 0) {
            mode = FUZZ_INTEGRATION;
        } else if (strcmp(argv[i], "--both") == 0) {
            mode = FUZZ_BOTH;
        } else if (strcmp(argv[i], "--help") == 0 || strcmp(argv[i], "-h") == 0) {
            printf("Usage: %s [options]\n", argv[0]);
            printf("Options:\n");
            printf("  -n <num>        Number of iterations (default: %d)\n", FUZZ_ITERATIONS_DEFAULT);
            printf("  --parser        Test parser only\n");
            printf("  --vm            Test VM only\n");
            printf("  --integration   Test integration only\n");
            printf("  --both          Test both parser and VM separately (default)\n");
            printf("  --help, -h      Show this help\n");
            return 0;
        }
    }
    
    init_fuzzing();
    
    printf("Ember Comprehensive Fuzzer\n");
    printf("==========================\n");
    
    int crashes = run_comprehensive_fuzz(mode, iterations);
    
    cleanup_fuzzing();
    
    printf("\nFuzzing completed.\n");
    
    if (crashes > 0) {
        printf("WARNING: %d total crashes detected during fuzzing!\n", crashes);
        return 1;
    }
    
    printf("No crashes detected. Ember appears stable.\n");
    return 0;
}