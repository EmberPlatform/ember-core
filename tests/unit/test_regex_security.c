#include "test_ember_internal.h"

// Macro to mark variables as intentionally unused
#define UNUSED(x) ((void)(x))
#include "../../src/stdlib/regex_native.h"
#include <assert.h>
#include <string.h>
#include <time.h>
#include <stdio.h>

// Test basic regex functionality
void test_regex_basic_match() {
    ember_regex_options opts = ember_regex_default_options();
    
    // Test simple character matching
    ember_regex_pattern* regex = ember_regex_compile("hello", &opts);

    UNUSED(regex);
    assert(regex != NULL);
    
    ember_regex_match* match = ember_regex_match_string(regex, "hello world");

    
    UNUSED(match);
    assert(match != NULL);
    assert(match->matched == 1);
    assert(match->start == 0);
    assert(match->end == 5);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Basic regex matching works\n");
}

// Test regex with quantifiers
void test_regex_quantifiers() {
    ember_regex_options opts = ember_regex_default_options();
    
    // Test + quantifier
    ember_regex_pattern* regex = ember_regex_compile("a+", &opts);

    UNUSED(regex);
    assert(regex != NULL);
    
    ember_regex_match* match = ember_regex_match_string(regex, "aaab");

    
    UNUSED(match);
    assert(match != NULL);
    assert(match->matched == 1);
    assert(match->start == 0);
    assert(match->end == 3);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    // Test * quantifier
    regex = ember_regex_compile("a*", &opts);
    assert(regex != NULL);
    
    match = ember_regex_match_string(regex, "aaab");
    assert(match != NULL);
    assert(match->matched == 1);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Regex quantifiers work correctly\n");
}

// Test character classes
void test_regex_character_classes() {
    ember_regex_options opts = ember_regex_default_options();
    
    // Test digit class
    ember_regex_pattern* regex = ember_regex_compile("\\d+", &opts);

    UNUSED(regex);
    assert(regex != NULL);
    
    ember_regex_match* match = ember_regex_match_string(regex, "abc123def");

    
    UNUSED(match);
    assert(match != NULL);
    assert(match->matched == 1);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    // Test word class
    regex = ember_regex_compile("\\w+", &opts);
    assert(regex != NULL);
    
    match = ember_regex_match_string(regex, "hello world");
    assert(match != NULL);
    assert(match->matched == 1);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Character classes work correctly\n");
}

// Test catastrophic backtracking patterns
void test_regex_catastrophic_backtracking() {
    ember_regex_options opts = ember_regex_secure_options();
    opts.timeout_ms = 100;  // Very short timeout for testing
    opts.max_backtrack = 1000;  // Low backtrack limit
    
    // Test nested quantifiers - should be rejected during compilation
    ember_regex_pattern* regex = ember_regex_compile("(a+)+", &opts);

    UNUSED(regex);
    assert(regex == NULL);  // Should fail compilation due to dangerous pattern
    
    // Test alternation that could cause backtracking
    regex = ember_regex_compile("(a|a)*", &opts);
    if (regex != NULL) {
        // If compilation succeeds, execution should timeout or hit backtrack limit
        ember_regex_match* match = ember_regex_match_string(regex, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        UNUSED(match);
        // Match should fail due to timeout or backtrack limit
        assert(match == NULL || !match->matched);
        
        ember_regex_free_match(match);
        ember_regex_free_pattern(regex);
    }
    
    printf("✓ Catastrophic backtracking protection works\n");
}

// Test timeout protection
void test_regex_timeout_protection() {
    ember_regex_options opts = ember_regex_default_options();
    opts.timeout_ms = 10;  // Very short timeout
    
    // Create a regex that would normally match quickly
    ember_regex_pattern* regex = ember_regex_compile("a*", &opts);

    UNUSED(regex);
    assert(regex != NULL);
    
    // Test with a very long string that could cause timeout
    char long_string[10000];
    memset(long_string, 'a', sizeof(long_string) - 1);
    long_string[sizeof(long_string) - 1] = '\0';
    
    clock_t start = clock();
    ember_regex_match* match = ember_regex_match_string(regex, long_string);

    UNUSED(match);
    clock_t end = clock();
    
    double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
    
    // Should complete within reasonable time due to timeout protection
    assert(elapsed < 100);  // Should not take more than 100ms
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Timeout protection works (elapsed: %.2fms)\n", elapsed);
}

// Test input size validation
void test_regex_input_size_validation() {
    ember_regex_options opts = ember_regex_secure_options();
    opts.max_input_size = 100;  // Small limit for testing
    
    ember_regex_pattern* regex = ember_regex_compile("test", &opts);

    
    UNUSED(regex);
    assert(regex != NULL);
    
    // Create string larger than limit
    char large_string[200];
    memset(large_string, 'a', sizeof(large_string) - 1);
    large_string[sizeof(large_string) - 1] = '\0';
    
    // Should reject large input
    ember_regex_match* match = ember_regex_match_string(regex, large_string);

    UNUSED(match);
    assert(match == NULL);
    
    // Should accept small input
    match = ember_regex_match_string(regex, "test");
    assert(match != NULL);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Input size validation works\n");
}

// Test pattern validation
void test_regex_pattern_validation() {
    // Test pattern too long
    char long_pattern[2000];
    memset(long_pattern, 'a', sizeof(long_pattern) - 1);
    long_pattern[sizeof(long_pattern) - 1] = '\0';
    
    ember_regex_result result = ember_regex_validate_pattern(long_pattern);
    assert(result == EMBER_REGEX_ERROR_PATTERN_TOO_LONG);
    
    // Test dangerous pattern
    result = ember_regex_validate_pattern("(a+)+");
    assert(result == EMBER_REGEX_ERROR_INVALID_PATTERN);
    
    // Test valid pattern
    result = ember_regex_validate_pattern("hello");
    assert(result == EMBER_REGEX_OK);
    
    printf("✓ Pattern validation works\n");
}

// Test dangerous pattern detection
void test_regex_dangerous_pattern_detection() {
    // Test various dangerous patterns
    assert(ember_regex_is_dangerous_pattern("(a+)+") == 1);
    assert(ember_regex_is_dangerous_pattern("(a*)*") == 1);
    assert(ember_regex_is_dangerous_pattern("(a?)+") == 1);
    assert(ember_regex_is_dangerous_pattern("a{1000,2000}") == 1);
    
    // Test safe patterns
    assert(ember_regex_is_dangerous_pattern("hello") == 0);
    assert(ember_regex_is_dangerous_pattern("a+") == 0);
    assert(ember_regex_is_dangerous_pattern("(abc)+") == 0);
    assert(ember_regex_is_dangerous_pattern("a{1,10}") == 0);
    
    printf("✓ Dangerous pattern detection works\n");
}

// Test escape sequences
void test_regex_escape_sequences() {
    ember_regex_options opts = ember_regex_default_options();
    
    // Test literal dot
    ember_regex_pattern* regex = ember_regex_compile("\\.", &opts);

    UNUSED(regex);
    assert(regex != NULL);
    
    ember_regex_match* match = ember_regex_match_string(regex, "a.b");

    
    UNUSED(match);
    assert(match != NULL);
    assert(match->matched == 1);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    // Test literal parentheses
    regex = ember_regex_compile("\\(\\)", &opts);
    assert(regex != NULL);
    
    match = ember_regex_match_string(regex, "()");
    assert(match != NULL);
    assert(match->matched == 1);
    
    ember_regex_free_match(match);
    ember_regex_free_pattern(regex);
    
    printf("✓ Escape sequences work correctly\n");
}

// Test backtrack limit protection
void test_regex_backtrack_limit() {
    ember_regex_options opts = ember_regex_default_options();
    opts.max_backtrack = 100;  // Very low limit
    
    // Pattern that could cause excessive backtracking
    ember_regex_pattern* regex = ember_regex_compile("a*a*a*a*a*a*a*a*a*a*b", &opts);

    UNUSED(regex);
    if (regex != NULL) {
        ember_regex_match* match = ember_regex_match_string(regex, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        UNUSED(match);
        // Should fail due to backtrack limit
        assert(match == NULL || !match->matched);
        
        ember_regex_free_match(match);
        ember_regex_free_pattern(regex);
    }
    
    printf("✓ Backtrack limit protection works\n");
}

// Test standard library functions
void test_regex_stdlib_functions() {
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    assert(vm != NULL);
    
    // Test regex_test function
    ember_value args[2];
    args[0] = ember_make_string_gc(vm, "test");
    args[1] = ember_make_string_gc(vm, "this is a test");
    
    ember_value result = ember_regex_test(vm, 2, args);

    
    UNUSED(result);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 1);
    
    // Test regex_match function
    result = ember_regex_match(vm, 2, args);
    assert(result.type == EMBER_VAL_HASH_MAP);
    
    // Test with invalid arguments
    args[0] = ember_make_number(123);
    result = ember_regex_test(vm, 2, args);
    assert(result.type == EMBER_VAL_BOOL);
    assert(result.as.bool_val == 0);
    
    ember_free_vm(vm);
    
    printf("✓ Standard library functions work correctly\n");
}

// Comprehensive security test with known attack patterns
void test_regex_security_comprehensive() {
    printf("Running comprehensive security tests...\n");
    
    // Test ReDoS attack patterns
    const char* attack_patterns[] = {
        "(a+)+",
        "(a*)*",
        "(a?)+",
        "(a|a)*",
        "a{1000,}",
        "(a+)+$",
        "^(a+)+",
        "^(a*)*$",
        "^(a|a)*$",
        NULL
    };
    
    ember_regex_options opts = ember_regex_secure_options();
    opts.timeout_ms = 50;  // Very short timeout
    
    for (int i = 0; attack_patterns[i] != NULL; i++) {
        printf("  Testing pattern: %s\n", attack_patterns[i]);
        
        ember_regex_pattern* regex = ember_regex_compile(attack_patterns[i], &opts);

        
        UNUSED(regex);
        if (regex == NULL) {
            printf("    ✓ Pattern rejected during compilation\n");
            continue;
        }
        
        // If compilation succeeded, test with potentially problematic input
        clock_t start = clock();
        ember_regex_match* match = ember_regex_match_string(regex, "aaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaaa");

        UNUSED(match);
        clock_t end = clock();
        
        double elapsed = ((double)(end - start)) / CLOCKS_PER_SEC * 1000;
        
        // Should complete quickly due to protections
        assert(elapsed < 200);  // Should not take more than 200ms
        
        if (match == NULL || !match->matched) {
            printf("    ✓ Pattern execution protected by timeout/backtrack limits\n");
        } else {
            printf("    ✓ Pattern completed safely in %.2fms\n", elapsed);
        }
        
        ember_regex_free_match(match);
        ember_regex_free_pattern(regex);
    }
    
    printf("✓ Comprehensive security tests passed\n");
}

// Run all regex tests
void run_regex_tests() {
    printf("Running regex security and functionality tests...\n");
    
    test_regex_basic_match();
    test_regex_quantifiers();
    test_regex_character_classes();
    test_regex_catastrophic_backtracking();
    test_regex_timeout_protection();
    test_regex_input_size_validation();
    test_regex_pattern_validation();
    test_regex_dangerous_pattern_detection();
    test_regex_escape_sequences();
    test_regex_backtrack_limit();
    test_regex_stdlib_functions();
    test_regex_security_comprehensive();
    
    printf("All regex tests passed! ✓\n");
}

#ifndef REGEX_TESTS_ONLY
int main() {
    run_regex_tests();
    return 0;
}
#endif