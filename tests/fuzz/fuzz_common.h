#ifndef FUZZ_COMMON_H
#define FUZZ_COMMON_H

#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <signal.h>
#include <setjmp.h>
#include <unistd.h>

// Common fuzzing configuration
#define FUZZ_MAX_INPUT_SIZE 1024
#define FUZZ_MAX_BYTECODE_SIZE 512
#define FUZZ_MAX_CONSTANTS 64
#define FUZZ_ITERATIONS_DEFAULT 1000
#define FUZZ_TIMEOUT_SECONDS 5

// Fuzzing statistics structure
typedef struct {
    int test_count;
    int crash_count;
    int timeout_count;
    int success_count;
    int error_count;
} fuzz_stats;

// Test result codes
typedef enum {
    FUZZ_SUCCESS = 0,
    FUZZ_ERROR = -1,
    FUZZ_TIMEOUT = -2,
    FUZZ_CRASH = -3
} fuzz_result;

// Function prototypes for common fuzzing utilities
void init_fuzzing(void);
void cleanup_fuzzing(void);
void print_fuzz_stats(const char* component, fuzz_stats* stats, int iterations);
void update_stats(fuzz_stats* stats, fuzz_result result);

// Random data generators
void generate_random_string(char* buffer, size_t max_size);
void generate_ember_keywords(char* buffer, size_t max_size);
void generate_malformed_syntax(char* buffer, size_t max_size);
void generate_edge_case_numbers(char* buffer, size_t max_size);
void generate_unicode_strings(char* buffer, size_t max_size);

// Bytecode generators
void generate_random_bytecode_sequence(uint8_t* buffer, size_t size);
void generate_malformed_jumps(uint8_t* buffer, size_t size);
void generate_stack_manipulation_bytecode(uint8_t* buffer, size_t size);

// Signal handling
extern jmp_buf fuzz_timeout_jmp;
void fuzz_timeout_handler(int sig);
int setup_timeout(int seconds);
void clear_timeout(void);

#endif // FUZZ_COMMON_H