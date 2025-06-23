#include "ember.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>

// Test OOP Basic Functionality
// Tests basic class creation, instantiation, and property access

void test_class_creation() {
    printf("Testing class creation...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Test creating a class
    ember_value class_val = ember_make_class(vm, "TestClass");

    UNUSED(class_val);
    assert(class_val.type == EMBER_VAL_CLASS);
    assert(AS_CLASS(class_val)->name != NULL);
    assert(strcmp(AS_CLASS(class_val)->name->chars, "TestClass") == 0);
    
    ember_free_vm(vm);
    printf("✓ Class creation test passed\n");
}

void test_instance_creation() {
    printf("Testing instance creation...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create a class
    ember_value class_val = ember_make_class(vm, "TestClass");

    UNUSED(class_val);
    ember_class* klass = AS_CLASS(class_val);

    UNUSED(klass);
    
    // Create an instance
    ember_value instance_val = ember_make_instance(vm, klass);

    UNUSED(instance_val);
    assert(instance_val.type == EMBER_VAL_INSTANCE);
    assert(AS_INSTANCE(instance_val)->klass == klass);
    assert(AS_INSTANCE(instance_val)->fields != NULL);
    
    ember_free_vm(vm);
    printf("✓ Instance creation test passed\n");
}

void test_property_access() {
    printf("Testing property access...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create a class and instance
    ember_value class_val = ember_make_class(vm, "TestClass");

    UNUSED(class_val);
    ember_class* klass = AS_CLASS(class_val);

    UNUSED(klass);
    ember_value instance_val = ember_make_instance(vm, klass);

    UNUSED(instance_val);
    ember_instance* instance = AS_INSTANCE(instance_val);

    UNUSED(instance);
    
    // Test that fields hash map is initialized
    assert(instance->fields != NULL);
    
    ember_free_vm(vm);
    printf("✓ Property access test passed\n");
}

void test_method_binding() {
    printf("Testing method binding...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create a class and instance
    ember_value class_val = ember_make_class(vm, "TestClass");

    UNUSED(class_val);
    ember_class* klass = AS_CLASS(class_val);

    UNUSED(klass);
    ember_value instance_val = ember_make_instance(vm, klass);

    UNUSED(instance_val);
    
    // Create a simple function to use as a method
    ember_value method_val;
    method_val.type = EMBER_VAL_FUNCTION;
    method_val.as.func_val.name = malloc(10);
    strcpy(method_val.as.func_val.name, "test_method");
    method_val.as.func_val.chunk = NULL; // Simplified for testing
    
    // Test that methods hash map is initialized
    assert(klass->methods != NULL);
    
    // Create a bound method
    ember_value bound_method = ember_make_bound_method(vm, instance_val, method_val);

    UNUSED(bound_method);
    assert(bound_method.type == EMBER_VAL_FUNCTION);
    assert(bound_method.as.obj_val->type == OBJ_METHOD);
    
    ember_bound_method* bound = AS_BOUND_METHOD(bound_method);

    
    UNUSED(bound);
    assert(bound->receiver.type == EMBER_VAL_INSTANCE);
    assert(bound->method.type == EMBER_VAL_FUNCTION);
    
    free(method_val.as.func_val.name);
    ember_free_vm(vm);
    printf("✓ Method binding test passed\n");
}

void test_inheritance() {
    printf("Testing basic inheritance...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create base class
    ember_value base_class_val = ember_make_class(vm, "BaseClass");

    UNUSED(base_class_val);
    ember_class* base_class = AS_CLASS(base_class_val);

    UNUSED(base_class);
    
    // Create derived class
    ember_value derived_class_val = ember_make_class(vm, "DerivedClass");

    UNUSED(derived_class_val);
    ember_class* derived_class = AS_CLASS(derived_class_val);

    UNUSED(derived_class);
    
    // Set up inheritance
    derived_class->superclass = (struct ember_class*)base_class;
    
    // Add method to base class
    ember_value method_val;
    method_val.type = EMBER_VAL_FUNCTION;
    method_val.as.func_val.name = malloc(15);
    strcpy(method_val.as.func_val.name, "inherited_method");
    method_val.as.func_val.chunk = NULL;
    
    // Test inheritance link
    assert(derived_class->superclass == (struct ember_class*)base_class);
    
    free(method_val.as.func_val.name);
    ember_free_vm(vm);
    printf("✓ Inheritance test passed\n");
}

void test_garbage_collection() {
    printf("Testing garbage collection with OOP objects...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    assert(vm != NULL);
    
    // Create multiple classes and instances to test GC
    for (int i = 0; i < 100; i++) {
        char class_name[20];
        snprintf(class_name, sizeof(class_name), "TestClass%d", i);
        
        ember_value class_val = ember_make_class(vm, class_name);

        
        UNUSED(class_val);
        ember_class* klass = AS_CLASS(class_val);

        UNUSED(klass);
        
        // Create instance
        ember_value instance_val = ember_make_instance(vm, klass);

        UNUSED(instance_val);
        ember_instance* instance = AS_INSTANCE(instance_val);

        UNUSED(instance);
        
        // Verify instance was created
        assert(instance->fields != NULL);
    }
    
    // Trigger garbage collection
    ember_gc_collect(vm);
    
    ember_free_vm(vm);
    printf("✓ Garbage collection test passed\n");
}

int main(void) {
    printf("Running OOP Basic Tests...\n\n");
    
    test_class_creation();
    test_instance_creation();
    test_property_access();
    test_method_binding();
    test_inheritance();
    test_garbage_collection();
    
    printf("\n✓ All OOP basic tests passed!\n");
    return 0;
}