#include "fuzz_common.h"

// Global variables for signal handling
jmp_buf fuzz_timeout_jmp;
static fuzz_stats global_stats = {0};

// Initialize fuzzing environment
void init_fuzzing(void) {
    srand(time(NULL));
    memset(&global_stats, 0, sizeof(global_stats));
}

// Cleanup fuzzing environment
void cleanup_fuzzing(void) {
    // Nothing to cleanup for now
}

// Print fuzzing statistics
void print_fuzz_stats(const char* component, fuzz_stats* stats, int iterations) {
    printf("\n=== %s Fuzzing Statistics ===\n", component);
    printf("Total tests: %d\n", stats->test_count);
    printf("Iterations completed: %d\n", iterations);
    printf("Successes: %d\n", stats->success_count);
    printf("Errors: %d\n", stats->error_count);
    printf("Crashes: %d\n", stats->crash_count);
    printf("Timeouts: %d\n", stats->timeout_count);
    if (stats->test_count > 0) {
        printf("Success rate: %.2f%%\n", (100.0 * stats->success_count) / stats->test_count);
        printf("Crash rate: %.2f%%\n", (100.0 * stats->crash_count) / stats->test_count);
    }
}

// Update statistics based on test result
void update_stats(fuzz_stats* stats, fuzz_result result) {
    stats->test_count++;
    
    switch (result) {
        case FUZZ_SUCCESS:
            stats->success_count++;
            break;
        case FUZZ_ERROR:
            stats->error_count++;
            break;
        case FUZZ_TIMEOUT:
            stats->timeout_count++;
            break;
        case FUZZ_CRASH:
            stats->crash_count++;
            break;
    }
}

// Timeout handler
void fuzz_timeout_handler(int sig) {
    (void)sig;
    longjmp(fuzz_timeout_jmp, 1);
}

// Setup timeout for test
int setup_timeout(int seconds) {
    if (setjmp(fuzz_timeout_jmp)) {
        return 1; // Timeout occurred
    }
    
    signal(SIGALRM, fuzz_timeout_handler);
    alarm(seconds);
    return 0;
}

// Clear timeout
void clear_timeout(void) {
    alarm(0);
}

// Generate random string with various character types
void generate_random_string(char* buffer, size_t max_size) {
    size_t len = 1 + (rand() % (max_size - 1));
    
    for (size_t i = 0; i < len - 1; i++) {
        int char_type = rand() % 8;
        
        switch (char_type) {
            case 0: // Printable ASCII
                buffer[i] = 32 + (rand() % 95);
                break;
            case 1: // Digits
                buffer[i] = '0' + (rand() % 10);
                break;
            case 2: // Letters
                buffer[i] = 'a' + (rand() % 26);
                break;
            case 3: // Whitespace
                buffer[i] = " \t\n\r"[rand() % 4];
                break;
            case 4: // Special characters
                buffer[i] = "!@#$%^&*()_+-=[]{}|;':\",./<>?"[rand() % 32];
                break;
            case 5: // Control characters
                buffer[i] = rand() % 32;
                break;
            case 6: // High ASCII
                buffer[i] = 128 + (rand() % 128);
                break;
            case 7: // Random byte
            default:
                buffer[i] = rand() % 256;
                break;
        }
    }
    buffer[len - 1] = '\0';
}

// Generate Ember language keywords and syntax
void generate_ember_keywords(char* buffer, size_t max_size) {
    const char* keywords[] = {
        "fn", "if", "else", "while", "for", "return", "break", "continue",
        "import", "true", "false", "nil", "and", "or", "not"
    };
    
    const char* operators[] = {
        "+", "-", "*", "/", "%", "=", "==", "!=", "<", ">", "<=", ">=",
        "&&", "||", "!", "&", "|", "^", "<<", ">>", "+=", "-=", "*=", "/="
    };
    
    const char* punctuation[] = {
        "(", ")", "{", "}", "[", "]", ";", ",", ".", ":", "@", "\"", "'"
    };
    
    // Combine random keywords, operators, and punctuation
    buffer[0] = '\0';
    size_t pos = 0;
    int num_elements = 3 + (rand() % 8);
    
    for (int i = 0; i < num_elements && pos < max_size - 20; i++) {
        const char* element = NULL;
        int element_type = rand() % 6;
        
        if (element_type < 2) {
            element = keywords[rand() % (sizeof(keywords) / sizeof(keywords[0]))];
        } else if (element_type < 4) {
            element = operators[rand() % (sizeof(operators) / sizeof(operators[0]))];
        } else {
            element = punctuation[rand() % (sizeof(punctuation) / sizeof(punctuation[0]))];
        }
        
        if (pos + strlen(element) + 1 < max_size) {
            strcat(buffer + pos, element);
            pos += strlen(element);
            if (rand() % 3 == 0 && pos < max_size - 1) {
                buffer[pos++] = ' ';
                buffer[pos] = '\0';
            }
        }
    }
}

// Generate malformed syntax patterns
void generate_malformed_syntax(char* buffer, size_t max_size) {
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
        "123.456.789.123",
        "true false nil nil nil",
        "break continue return return",
        "import import import",
        "@@@@@@@@@",
        "\"string\" + + + \"string\"",
        "fn test() { return return return; }",
        "if if if (true) { }",
        "while while (true) { }",
        "{} [] () \"\" '' `` ~~",
        "a.b.c.d.e.f.g.h.i.j.k.l.m.n.o.p.q.r.s.t.u.v.w.x.y.z",
        "1 + 2 * 3 / 4 - 5 % 6 == 7 != 8 < 9 > 10 <= 11 >= 12",
        "fn(x) { if(x) { while(x) { for(x) { return x; } } } }",
        "\"string with \\n\\t\\r\\0\\x00\\u0000 escapes\"",
        "123.456e+789",
        "0x1234567890ABCDEF",
        "true && false || nil and not or",
    };
    
    int pattern_idx = rand() % (sizeof(patterns) / sizeof(patterns[0]));
    strncpy(buffer, patterns[pattern_idx], max_size - 1);
    buffer[max_size - 1] = '\0';
    
    // Add some random noise
    size_t len = strlen(buffer);
    if (len < max_size - 10) {
        for (int i = 0; i < 5 && len + i < max_size - 1; i++) {
            buffer[len + i] = 32 + (rand() % 95);
        }
        buffer[len + 5] = '\0';
    }
}

// Generate edge case numbers
void generate_edge_case_numbers(char* buffer, size_t max_size) {
    const char* numbers[] = {
        "0", "1", "-1", "3.14159", "-3.14159",
        "1e10", "1e-10", "-1e10", "-1e-10",
        "1.7976931348623157e+308", // Near double max
        "2.2250738585072014e-308", // Near double min
        "inf", "-inf", "nan", "NaN",
        "0.0", "-0.0", "1.0", "-1.0",
        "999999999999999999999999999999999999999999999999999999999999999",
        "0.000000000000000000000000000000000000000000000000000000000001",
        "123.456.789", // Invalid number format
        "1.2.3.4.5",   // Invalid number format
        "1e", "1e+", "1e-", "1e+1e", // Invalid exponents
        "0x", "0X", "0x123", "0X123", // Hex numbers (may not be supported)
        "0b", "0B", "0b101", "0B101", // Binary numbers (may not be supported)
        "0o", "0O", "0o123", "0O123", // Octal numbers (may not be supported)
    };
    
    int num_idx = rand() % (sizeof(numbers) / sizeof(numbers[0]));
    strncpy(buffer, numbers[num_idx], max_size - 1);
    buffer[max_size - 1] = '\0';
}

// Generate Unicode and special character strings
void generate_unicode_strings(char* buffer, size_t max_size) {
    const char* unicode_strings[] = {
        "Hello, 世界!", // Chinese
        "Привет, мир!", // Russian
        "こんにちは世界", // Japanese
        "🚀🌟💻🔥", // Emojis
        "café naïve résumé", // Accented characters
        "αβγδεζηθικλμνξοπρστυφχψω", // Greek
        "אבגדהוזחטיכלמנסעפצקרשת", // Hebrew
        "العربية", // Arabic
        "ñáéíóúü", // Spanish accents
        "čšžđć", // Croatian
        "øæå", // Norwegian
        "ß", // German
        "€£¥₹₽", // Currency symbols
        "©®™", // Symbols
        "←↑→↓↔↕", // Arrows
        "♠♣♥♦", // Card suits
        "♪♫♬", // Musical notes
        "∞∅∈∉∑∏", // Mathematical symbols
        "⚠️⚡⚽⚾", // Warning and sports
        "text with \x00 null bytes", // Null bytes
        "text with \x7F DEL char", // DEL character
        "\x01\x02\x03\x04\x05", // Control characters
    };
    
    int str_idx = rand() % (sizeof(unicode_strings) / sizeof(unicode_strings[0]));
    strncpy(buffer, unicode_strings[str_idx], max_size - 1);
    buffer[max_size - 1] = '\0';
}

// Generate random bytecode sequence
void generate_random_bytecode_sequence(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        int opcode_type = rand() % 10;
        
        if (opcode_type < 3) {
            // Common arithmetic operations
            ember_opcode ops[] = {OP_ADD, OP_SUB, OP_MUL, OP_DIV, OP_MOD};
            buffer[i] = ops[rand() % 5];
        } else if (opcode_type < 5) {
            // Stack operations
            ember_opcode ops[] = {OP_PUSH_CONST, OP_POP};
            buffer[i] = ops[rand() % 2];
        } else if (opcode_type < 7) {
            // Comparison operations
            ember_opcode ops[] = {OP_EQUAL, OP_NOT_EQUAL, OP_LESS, OP_GREATER};
            buffer[i] = ops[rand() % 4];
        } else if (opcode_type < 8) {
            // Control flow
            ember_opcode ops[] = {OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP, OP_CALL, OP_RETURN};
            buffer[i] = ops[rand() % 5];
        } else {
            // Random opcode (including potentially invalid ones)
            buffer[i] = rand() % 256;
        }
    }
}

// Generate malformed jump instructions
void generate_malformed_jumps(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        if (i % 3 == 0) {
            // Jump instruction
            ember_opcode jumps[] = {OP_JUMP, OP_JUMP_IF_FALSE, OP_LOOP};
            buffer[i] = jumps[rand() % 3];
        } else {
            // Random jump target (likely invalid)
            buffer[i] = rand() % 256;
        }
    }
}

// Generate stack manipulation bytecode
void generate_stack_manipulation_bytecode(uint8_t* buffer, size_t size) {
    for (size_t i = 0; i < size; i++) {
        int op_type = rand() % 5;
        
        switch (op_type) {
            case 0:
                buffer[i] = OP_PUSH_CONST;
                break;
            case 1:
                buffer[i] = OP_POP;
                break;
            case 2:
                buffer[i] = OP_ADD; // Requires 2 stack items
                break;
            case 3:
                buffer[i] = OP_NOT; // Requires 1 stack item
                break;
            case 4:
            default:
                // Random stack operation
                ember_opcode stack_ops[] = {
                    OP_GET_LOCAL, OP_SET_LOCAL, OP_GET_GLOBAL, OP_SET_GLOBAL
                };
                buffer[i] = stack_ops[rand() % 4];
                break;
        }
    }
}