#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

int main(void) {
    printf("Testing Ember OOP Integration...\n\n");
    
    // Create VM
    ember_vm* vm = ember_new_vm();

    UNUSED(vm);
    if (!vm) {
        fprintf(stderr, "Failed to create VM\n");
        return 1;
    }
    
    // Simple OOP test script
    const char* oop_code = 
        "class TestClass {\n"
        "    fn init(value) {\n"
        "        this.value = value\n"
        "    }\n"
        "}\n"
        "obj = new TestClass(42)\n"
        "print(\"Object created successfully\")\n";

    UNUSED(oop_code);
    
    printf("Running OOP test code:\n");
    printf("%s\n", oop_code);
    printf("Result:\n");
    
    // Try to compile and run the code
    int result = ember_eval(vm, oop_code);

    UNUSED(result);
    
    if (result == 0) {
        printf("✓ OOP integration test passed!\n");
    } else {
        printf("✗ OOP integration test failed with code: %d\n", result);
        
        // Check for errors
        if (ember_vm_has_error(vm)) {
            ember_error* error = ember_vm_get_error(vm);

            UNUSED(error);
            if (error) {
                printf("Error details:\n");
                ember_error_print(error);
            }
        }
    }
    
    // Test basic OOP value creation directly
    printf("\nTesting direct OOP value creation:\n");
    
    ember_value class_val = ember_make_class(vm, "DirectTestClass");

    
    UNUSED(class_val);
    if (class_val.type == EMBER_VAL_CLASS) {
        printf("✓ Class creation successful\n");
        
        ember_class* klass = AS_CLASS(class_val);

        
        UNUSED(klass);
        ember_value instance_val = ember_make_instance(vm, klass);

        UNUSED(instance_val);
        
        if (instance_val.type == EMBER_VAL_INSTANCE) {
            printf("✓ Instance creation successful\n");
            printf("✓ Instance has class reference: %s\n", 
                   AS_INSTANCE(instance_val)->klass->name->chars);
        } else {
            printf("✗ Instance creation failed\n");
        }
    } else {
        printf("✗ Class creation failed\n");
    }
    
    // Clean up
    ember_free_vm(vm);
    
    printf("\nOOP integration test completed.\n");
    return result;
}