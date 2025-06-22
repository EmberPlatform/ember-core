#include "include/ember.h"
#include <stdio.h>
#include <assert.h>

// Simple test that doesn't rely on the full library
int main() {
    printf("Testing core regex functionality...\n");
    
    // Create VM  
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        printf("Failed to create VM\n");
        return 1;
    }
    
    // Test regex creation
    ember_value regex = ember_make_regex(vm, "hello", REGEX_NONE);
    if (regex.type != EMBER_VAL_REGEX) {
        printf("Failed to create regex\n");
        return 1;
    }
    printf("✓ Regex creation successful\n");
    
    // Test basic regex functionality
    ember_regex* regex_obj = AS_REGEX(regex);
    if (!regex_obj) {
        printf("Failed to extract regex object\n");
        return 1;
    }
    
    // Test pattern matching
    int match = regex_test(regex_obj, "hello world");
    if (!match) {
        printf("Failed basic pattern matching\n");
        return 1;
    }
    printf("✓ Pattern matching works\n");
    
    int no_match = regex_test(regex_obj, "goodbye world");
    if (no_match) {
        printf("False positive in pattern matching\n");
        return 1;
    }
    printf("✓ Negative pattern matching works\n");
    
    printf("✓ All core regex tests passed!\n");
    return 0;
}