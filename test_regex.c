#include "include/ember.h"
#include <stdio.h>
#include <assert.h>

int main() {
    printf("Testing Ember regex implementation...\n");
    
    // Create VM
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Test regex creation
    ember_value regex = ember_make_regex(vm, "test", REGEX_NONE);
    assert(regex.type == EMBER_VAL_REGEX);
    printf("✓ Regex creation works\n");
    
    // Test regex_test function
    ember_regex* regex_obj = AS_REGEX(regex);
    int match_result = regex_test(regex_obj, "this is a test string");
    assert(match_result == 1);
    printf("✓ Regex test (positive match) works\n");
    
    int no_match_result = regex_test(regex_obj, "no match here");
    assert(no_match_result == 0);
    printf("✓ Regex test (negative match) works\n");
    
    // Test regex_replace function
    ember_value replaced = regex_replace(vm, regex_obj, "this is a test string", "example");
    assert(replaced.type == EMBER_VAL_STRING);
    printf("✓ Regex replace works\n");
    
    // Test regex printing
    printf("Regex representation: ");
    print_value(regex);
    printf("\n");
    
    // Test values_equal for regex
    ember_value regex2 = ember_make_regex(vm, "test", REGEX_NONE);
    assert(values_equal(regex, regex2));
    printf("✓ Regex equality comparison works\n");
    
    ember_value regex3 = ember_make_regex(vm, "different", REGEX_NONE);
    assert(!values_equal(regex, regex3));
    printf("✓ Regex inequality comparison works\n");
    
    // Clean up
    ember_free_vm(vm);
    
    printf("\nAll regex tests passed! ✓\n");
    return 0;
}