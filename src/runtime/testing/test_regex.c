#include "testing_framework.h"

// Test Regex creation
TEST_DEFINE(regex_creation) {
    ember_value regex = ember_make_regex(vm, "test.*pattern", REGEX_NONE);
    TEST_ASSERT_NOT_NULL(regex, "Regex should be created successfully");
    TEST_ASSERT(regex.type == EMBER_VAL_REGEX, "Value should be of type REGEX");
    
    ember_regex* regex_obj = AS_REGEX(regex);
    TEST_ASSERT_STR_EQ("test.*pattern", regex_obj->pattern, "Pattern should be stored correctly");
    TEST_ASSERT(regex_obj->flags == REGEX_NONE, "Flags should be set correctly");
}

TEST_DEFINE(regex_flags) {
    ember_value regex_case_insensitive = ember_make_regex(vm, "Hello", REGEX_CASE_INSENSITIVE);
    ember_value regex_multiline = ember_make_regex(vm, "^line", REGEX_MULTILINE);
    ember_value regex_global = ember_make_regex(vm, "all", REGEX_GLOBAL);
    ember_value regex_combined = ember_make_regex(vm, "test", REGEX_CASE_INSENSITIVE | REGEX_GLOBAL);
    
    ember_regex* regex_ci = AS_REGEX(regex_case_insensitive);
    ember_regex* regex_ml = AS_REGEX(regex_multiline);
    ember_regex* regex_g = AS_REGEX(regex_global);
    ember_regex* regex_comb = AS_REGEX(regex_combined);
    
    TEST_ASSERT(regex_ci->flags == REGEX_CASE_INSENSITIVE, "Case insensitive flag should be set");
    TEST_ASSERT(regex_ml->flags == REGEX_MULTILINE, "Multiline flag should be set");
    TEST_ASSERT(regex_g->flags == REGEX_GLOBAL, "Global flag should be set");
    TEST_ASSERT(regex_comb->flags == (REGEX_CASE_INSENSITIVE | REGEX_GLOBAL), "Combined flags should be set");
}

// Test regex operations
TEST_DEFINE(regex_test_operation) {
    ember_value regex = ember_make_regex(vm, "hello", REGEX_NONE);
    ember_regex* regex_obj = AS_REGEX(regex);
    
    // Test positive matches
    TEST_ASSERT(regex_test(regex_obj, "hello world"), "Should match 'hello world'");
    TEST_ASSERT(regex_test(regex_obj, "say hello"), "Should match 'say hello'");
    TEST_ASSERT(regex_test(regex_obj, "hello"), "Should match exact 'hello'");
    
    // Test negative matches
    TEST_ASSERT(!regex_test(regex_obj, "hi there"), "Should not match 'hi there'");
    TEST_ASSERT(!regex_test(regex_obj, "goodbye"), "Should not match 'goodbye'");
    TEST_ASSERT(!regex_test(regex_obj, ""), "Should not match empty string");
    
    // Test case sensitivity
    ember_value regex_case = ember_make_regex(vm, "Hello", REGEX_NONE);
    ember_regex* regex_case_obj = AS_REGEX(regex_case);
    
    TEST_ASSERT(regex_test(regex_case_obj, "Hello world"), "Should match exact case 'Hello world'");
    TEST_ASSERT(!regex_test(regex_case_obj, "hello world"), "Should not match different case without flag");
    
    // Test case insensitive
    ember_value regex_ci = ember_make_regex(vm, "Hello", REGEX_CASE_INSENSITIVE);
    ember_regex* regex_ci_obj = AS_REGEX(regex_ci);
    
    TEST_ASSERT(regex_test(regex_ci_obj, "hello world"), "Should match different case with case insensitive flag");
    TEST_ASSERT(regex_test(regex_ci_obj, "HELLO WORLD"), "Should match uppercase with case insensitive flag");
}

TEST_DEFINE(regex_match_operation) {
    ember_value regex = ember_make_regex(vm, "test", REGEX_NONE);
    ember_regex* regex_obj = AS_REGEX(regex);
    
    // Test successful match
    ember_array* matches = regex_match(vm, regex_obj, "this is a test string");
    TEST_ASSERT(matches != NULL, "Match should return results");
    TEST_ASSERT(matches->length > 0, "Should find at least one match");
    
    // Test no match
    ember_array* no_matches = regex_match(vm, regex_obj, "no pattern here");
    TEST_ASSERT(no_matches != NULL, "Match should return empty array for no matches");
    TEST_ASSERT(no_matches->length == 0, "Should find no matches");
}

TEST_DEFINE(regex_replace_operation) {
    ember_value regex = ember_make_regex(vm, "old", REGEX_NONE);
    ember_regex* regex_obj = AS_REGEX(regex);
    
    // Test successful replacement
    ember_value result = regex_replace(vm, regex_obj, "the old text", "new");
    TEST_ASSERT(result.type == EMBER_VAL_STRING, "Replace should return string");
    
    const char* result_str = AS_CSTRING(result);
    TEST_ASSERT_STR_EQ("the new text", result_str, "Replacement should work correctly");
    
    // Test no replacement needed
    ember_value no_replace = regex_replace(vm, regex_obj, "nothing to change", "new");
    TEST_ASSERT(no_replace.type == EMBER_VAL_STRING, "Replace should return original string when no match");
    
    const char* no_replace_str = AS_CSTRING(no_replace);
    TEST_ASSERT_STR_EQ("nothing to change", no_replace_str, "Should return original string when no match");
}

TEST_DEFINE(regex_split_operation) {
    ember_value regex = ember_make_regex(vm, ",", REGEX_NONE);
    ember_regex* regex_obj = AS_REGEX(regex);
    
    // Test successful split
    ember_array* parts = regex_split(vm, regex_obj, "apple,banana,cherry");
    TEST_ASSERT(parts != NULL, "Split should return array");
    TEST_ASSERT(parts->length == 3, "Should split into 3 parts");
    
    // Check individual parts
    if (parts->length >= 3) {
        ember_value part1 = parts->elements[0];
        ember_value part2 = parts->elements[1];
        ember_value part3 = parts->elements[2];
        
        TEST_ASSERT(part1.type == EMBER_VAL_STRING, "First part should be string");
        TEST_ASSERT(part2.type == EMBER_VAL_STRING, "Second part should be string");
        TEST_ASSERT(part3.type == EMBER_VAL_STRING, "Third part should be string");
        
        TEST_ASSERT_STR_EQ("apple", AS_CSTRING(part1), "First part should be 'apple'");
        TEST_ASSERT_STR_EQ("banana", AS_CSTRING(part2), "Second part should be 'banana'");
        TEST_ASSERT_STR_EQ("cherry", AS_CSTRING(part3), "Third part should be 'cherry'");
    }
    
    // Test no split needed
    ember_array* no_split = regex_split(vm, regex_obj, "noseparator");
    TEST_ASSERT(no_split != NULL, "Split should return array even with no separator");
    TEST_ASSERT(no_split->length == 1, "Should return single element when no separator found");
}

// Test regex equality
TEST_DEFINE(regex_equality) {
    ember_value regex1 = ember_make_regex(vm, "pattern", REGEX_NONE);
    ember_value regex2 = ember_make_regex(vm, "pattern", REGEX_NONE);
    ember_value regex3 = ember_make_regex(vm, "different", REGEX_NONE);
    ember_value regex4 = ember_make_regex(vm, "pattern", REGEX_CASE_INSENSITIVE);
    
    // Test equal regexes
    TEST_ASSERT_EQ(regex1, regex2, "Regexes with same pattern and flags should be equal");
    
    // Test different patterns
    TEST_ASSERT_NEQ(regex1, regex3, "Regexes with different patterns should not be equal");
    
    // Test different flags
    TEST_ASSERT_NEQ(regex1, regex4, "Regexes with different flags should not be equal");
}

// Test error cases
TEST_DEFINE(regex_error_cases) {
    // Test null pattern
    ember_value null_regex = ember_make_regex(vm, NULL, REGEX_NONE);
    TEST_ASSERT(null_regex.type == EMBER_VAL_NIL, "Regex with null pattern should return nil");
    
    // Test operations on null regex
    TEST_ASSERT(!regex_test(NULL, "test"), "regex_test with null regex should return false");
    
    ember_array* null_match = regex_match(vm, NULL, "test");
    TEST_ASSERT(null_match == NULL, "regex_match with null regex should return NULL");
    
    ember_value null_replace = regex_replace(vm, NULL, "test", "replacement");
    TEST_ASSERT(null_replace.type == EMBER_VAL_NIL, "regex_replace with null regex should return nil");
    
    ember_array* null_split = regex_split(vm, NULL, "test");
    TEST_ASSERT(null_split == NULL, "regex_split with null regex should return NULL");
}

// Performance test
void benchmark_regex_test(ember_vm* vm) {
    ember_value regex = ember_make_regex(vm, "pattern", REGEX_NONE);
    ember_regex* regex_obj = AS_REGEX(regex);
    
    for (int i = 0; i < 1000; i++) {
        regex_test(regex_obj, "this is a test pattern for benchmarking");
    }
}

TEST_DEFINE(regex_performance) {
    test_benchmark("Regex Test Performance", benchmark_regex_test, vm, 100);
}

// Test suite runner
TEST_SUITE(regex) {
    ember_vm* vm = ember_new_vm();
    test_suite* suite = test_runner_add_suite(runner, "Regular Expressions");
    
    test_suite_add_test(suite, "Regex Creation", test_regex_creation, vm);
    test_suite_add_test(suite, "Regex Flags", test_regex_flags, vm);
    test_suite_add_test(suite, "Regex Test Operation", test_regex_test_operation, vm);
    test_suite_add_test(suite, "Regex Match Operation", test_regex_match_operation, vm);
    test_suite_add_test(suite, "Regex Replace Operation", test_regex_replace_operation, vm);
    test_suite_add_test(suite, "Regex Split Operation", test_regex_split_operation, vm);
    test_suite_add_test(suite, "Regex Equality", test_regex_equality, vm);
    test_suite_add_test(suite, "Regex Error Cases", test_regex_error_cases, vm);
    test_suite_add_test(suite, "Regex Performance", test_regex_performance, vm);
    
    ember_free_vm(vm);
}