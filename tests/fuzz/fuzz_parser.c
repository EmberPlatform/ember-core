#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

// Fuzzing configuration
#define FUZZ_MAX_INPUT_SIZE 1024
#define FUZZ_ITERATIONS_DEFAULT 1000
#define FUZZ_TIMEOUT_SECONDS 5

// Global variables for signal handling
static jmp_buf timeout_jmp;
static int test_count = 0;
static int crash_count = 0;
static int timeout_count = 0;

// Timeout handler
void timeout_handler(int sig) {
    (void)sig;
    timeout_count++;
    longjmp(timeout_jmp, 1);
}

// Generate random input data for fuzzing
void generate_random_input(char* buffer, size_t max_size) {
    // Generate random length between 1 and max_size-1
    size_t len = 1 + (rand() % (max_size - 1));
    
    for (size_t i = 0; i < len - 1; i++) {
        // Mix of different character types for better coverage
        int char_type = rand() % 10;
        
        if (char_type < 3) {
            // Printable ASCII
            buffer[i] = 32 + (rand() % 95);
        } else if (char_type < 6) {
            // Common Ember syntax characters
            const char syntax_chars[] = "(){}[];,.:=+-*/><! \t\n\"'";
            buffer[i] = syntax_chars[rand() % (sizeof(syntax_chars) - 1)];
        } else if (char_type < 8) {
            // Numbers and letters
            if (rand() % 2) {
                buffer[i] = '0' + (rand() % 10);
            } else {
                buffer[i] = 'a' + (rand() % 26);
            }
        } else {
            // Random bytes (including control characters)
            buffer[i] = rand() % 256;
        }
    }
    buffer[len - 1] = '\0';
}

// Generate malformed Ember-like syntax
void generate_malformed_ember(char* buffer, size_t max_size) {
    const char* patterns[] = {
        "fn unclosed_function() {",
        "if (true { missing_paren",
        "while true) { extra_paren",
        "for (i = 0; i < 10 i++) {",
        "\"unclosed string",
        "/* unclosed comment",
        "// incomplete line",
        "{{{{{{{{{{{",
        "}}}}}}}}}}",
        "(((((((((",
        ")))))))))",
        "[[[[[[[[[",
        "]]]]]]]]]",
        "= = = = =",
        "fn fn fn fn",
        "123.456.789",
        "true false nil",
        "break continue return",
        "import import import",
        "@@@@@@@@@",
        "\"string\" + + + \"string\"",
        "fn test() { return return return; }",
        "if if if (true) { }",
        "while while (true) { }",
        "{} [] () \"\" '' `` ~~",
        "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p",
    };
    
    int pattern_idx = rand() % (sizeof(patterns) / sizeof(patterns[0]));
    strncpy(buffer, patterns[pattern_idx], max_size - 1);
    buffer[max_size - 1] = '\0';
    
    // Add some random noise to the end
    size_t len = strlen(buffer);
    if (len < max_size - 10) {
        for (int i = 0; i < 5 && len + i < max_size - 1; i++) {
            buffer[len + i] = 32 + (rand() % 95);
        }
        buffer[len + 5] = '\0';
    }
}

// Test parser with given input
int test_parser_input(const char* input) {
    ember_vm* vm = NULL;
    ember_chunk chunk;
    int result = 0;
    
    // Set up timeout
    if (setjmp(timeout_jmp)) {
        // Timeout occurred
        if (vm) ember_free_vm(vm);
        return -2; // Timeout error
    }
    
    signal(SIGALRM, timeout_handler);
    alarm(FUZZ_TIMEOUT_SECONDS);
    
    // Initialize VM and chunk
    vm = ember_new_vm();
    if (!vm) {
        alarm(0);
        return -1; // Setup error
    }
    
    // Initialize chunk
    memset(&chunk, 0, sizeof(chunk));
    
    // Try to compile the input
    // Note: This uses the internal compile function which may not be exposed
    // In a real scenario, you'd use ember_eval or a public parsing API
    result = ember_eval(vm, input);
    
    // Clean up
    alarm(0);
    ember_free_vm(vm);
    
    return result;
}

// Print fuzzing statistics
void print_stats(int iterations) {
    printf("\n=== Fuzzing Statistics ===\n");
    printf("Total tests: %d\n", test_count);
    printf("Iterations completed: %d\n", iterations);
    printf("Crashes detected: %d\n", crash_count);
    printf("Timeouts: %d\n", timeout_count);
    printf("Success rate: %.2f%%\n", 
           test_count > 0 ? (100.0 * (test_count - crash_count - timeout_count) / test_count) : 0.0);
}

// Main fuzzing function
int fuzz_parser(int iterations) {
    char input_buffer[FUZZ_MAX_INPUT_SIZE];
    int i;
    
    printf("Starting parser fuzzing with %d iterations...\n", iterations);
    printf("Max input size: %d bytes\n", FUZZ_MAX_INPUT_SIZE);
    printf("Timeout per test: %d seconds\n", FUZZ_TIMEOUT_SECONDS);
    
    for (i = 0; i < iterations; i++) {
        test_count++;
        
        // Show progress every 100 iterations
        if (i % 100 == 0) {
            printf("Progress: %d/%d (crashes: %d, timeouts: %d)\n", 
                   i, iterations, crash_count, timeout_count);
        }
        
        // Generate different types of input
        if (i % 3 == 0) {
            generate_random_input(input_buffer, FUZZ_MAX_INPUT_SIZE);
        } else {
            generate_malformed_ember(input_buffer, FUZZ_MAX_INPUT_SIZE);
        }
        
        // Test the parser
        int result = test_parser_input(input_buffer);
        
        if (result == -1) {
            crash_count++;
            printf("CRASH detected at iteration %d with input: '%.50s%s'\n", 
                   i, input_buffer, strlen(input_buffer) > 50 ? "..." : "");
        } else if (result == -2) {
            // Timeout already counted in handler
            printf("TIMEOUT at iteration %d with input: '%.50s%s'\n", 
                   i, input_buffer, strlen(input_buffer) > 50 ? "..." : "");
        }
        
        // Stop if too many crashes
        if (crash_count > iterations / 10) {
            printf("Too many crashes detected, stopping early.\n");
            break;
        }
    }
    
    print_stats(i < iterations ? i : iterations);
    return crash_count;
}

int main(int argc, char* argv[]) {
    int iterations = FUZZ_ITERATIONS_DEFAULT;
    
    // Parse command line arguments
    if (argc > 1) {
        iterations = atoi(argv[1]);
        if (iterations <= 0) {
            printf("Invalid iteration count: %s\n", argv[1]);
            return 1;
        }
    }
    
    // Initialize random seed
    srand(time(NULL));
    
    printf("Ember Parser Fuzzer\n");
    printf("==================\n");
    
    int crashes = fuzz_parser(iterations);
    
    printf("\nFuzzing completed.\n");
    
    if (crashes > 0) {
        printf("WARNING: %d crashes detected during fuzzing!\n", crashes);
        return 1;
    }
    
    printf("No crashes detected. Parser appears stable.\n");
    return 0;
}