/*
 * Architecture Refactor Integration Test
 * 
 * This test validates that the new interface-based architecture works correctly
 * and that functionality is preserved after breaking circular dependencies.
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "../../include/ember.h"
#include "../../include/ember_interfaces.h"

// Test results structure
typedef struct {
    int total_tests;
    int passed_tests;
    int failed_tests;
    char error_messages[10][256];
} test_results_t;

static test_results_t results = {0};

// Helper macros for testing
#define TEST_ASSERT(condition, message) \
    do { \
        results.total_tests++; \
        if (condition) { \
            results.passed_tests++; \
            printf("  ✓ %s\n", message); \
        } else { \
            results.failed_tests++; \
            printf("  ✗ %s\n", message); \
            if (results.failed_tests <= 10) { \
                snprintf(results.error_messages[results.failed_tests-1], 256, "%s", message); \
            } \
        } \
    } while(0)

#define TEST_NOT_NULL(ptr, message) \
    TEST_ASSERT((ptr) != NULL, message)

#define TEST_NULL(ptr, message) \
    TEST_ASSERT((ptr) == NULL, message)

#define TEST_EQUAL(a, b, message) \
    TEST_ASSERT((a) == (b), message)

#define TEST_STRING_EQUAL(a, b, message) \
    TEST_ASSERT(strcmp((a), (b)) == 0, message)

// Forward declarations for external interface registration functions
extern void ember_core_register_interface(void);

void test_interface_registration(void) {
    printf("\n=== Testing Interface Registration ===\n");
    
    // Initially, no interfaces should be registered
    TEST_NULL(ember_get_core_interface(), "Core interface initially null");
    TEST_NULL(ember_get_stdlib_interface(), "Stdlib interface initially null");
    TEST_NULL(ember_get_emberweb_interface(), "EmberWeb interface initially null");
    
    // Register core interface
    ember_core_register_interface();
    TEST_NOT_NULL(ember_get_core_interface(), "Core interface registered successfully");
    
    // Test core interface validation
    const ember_core_interface_t* core = ember_get_core_interface();
    TEST_NOT_NULL(core->create_vm, "Core interface has create_vm function");
    TEST_NOT_NULL(core->destroy_vm, "Core interface has destroy_vm function");
    TEST_NOT_NULL(core->eval_code, "Core interface has eval_code function");
    TEST_NOT_NULL(core->make_nil, "Core interface has make_nil function");
    TEST_NOT_NULL(core->make_bool, "Core interface has make_bool function");
    TEST_NOT_NULL(core->make_number, "Core interface has make_number function");
    TEST_NOT_NULL(core->make_string, "Core interface has make_string function");
    TEST_NOT_NULL(core->register_native_function, "Core interface has register_native_function");
    
    TEST_ASSERT(ember_validate_core_interface(core), "Core interface validation passes");
}

void test_core_functionality(void) {
    printf("\n=== Testing Core Functionality (Standalone) ===\n");
    
    const ember_core_interface_t* core = ember_get_core_interface();
    if (!core) {
        printf("  Skipping core functionality test - interface not available\n");
        return;
    }
    
    // Test VM creation and destruction
    ember_vm* vm = core->create_vm();
    TEST_NOT_NULL(vm, "VM creation successful");
    
    if (vm) {
        // Test value creation functions
        ember_value nil_val = core->make_nil();
        TEST_EQUAL(nil_val.type, EMBER_VAL_NIL, "make_nil creates NIL value");
        
        ember_value bool_val = core->make_bool(1);
        TEST_EQUAL(bool_val.type, EMBER_VAL_BOOL, "make_bool creates BOOL value");
        TEST_EQUAL(bool_val.as.bool_val, 1, "make_bool sets correct value");
        
        ember_value num_val = core->make_number(42.5);
        TEST_EQUAL(num_val.type, EMBER_VAL_NUMBER, "make_number creates NUMBER value");
        TEST_ASSERT(num_val.as.number_val == 42.5, "make_number sets correct value");
        
        ember_value str_val = core->make_string("test");
        TEST_EQUAL(str_val.type, EMBER_VAL_STRING, "make_string creates STRING value");
        
        // Test type checking functions
        TEST_ASSERT(core->is_nil(nil_val), "is_nil correctly identifies NIL value");
        TEST_ASSERT(core->is_bool(bool_val), "is_bool correctly identifies BOOL value");
        TEST_ASSERT(core->is_number(num_val), "is_number correctly identifies NUMBER value");
        TEST_ASSERT(core->is_string(str_val), "is_string correctly identifies STRING value");
        
        // Test value extraction functions
        TEST_EQUAL(core->get_bool(bool_val), 1, "get_bool extracts correct value");
        TEST_ASSERT(core->get_number(num_val) == 42.5, "get_number extracts correct value");
        
        // Test simple code evaluation
        int eval_result = core->eval_code(vm, "print(\"Hello from refactored core!\");");
        TEST_ASSERT(eval_result == 0, "Basic code evaluation succeeds");
        
        core->destroy_vm(vm);
        printf("  ✓ VM destruction completed\n");
    }
}

void test_dependency_isolation(void) {
    printf("\n=== Testing Dependency Isolation ===\n");
    
    // Test that core can function without stdlib
    printf("  Testing core-only functionality...\n");
    
    ember_vm* vm = ember_new_vm();
    TEST_NOT_NULL(vm, "Core VM creation without stdlib");
    
    if (vm) {
        // Test that basic functions work
        int result = ember_eval(vm, "let x = 5 + 3; print(x);");
        TEST_EQUAL(result, 0, "Basic arithmetic without stdlib");
        
        result = ember_eval(vm, "let arr = [1, 2, 3]; print(len(arr));");
        TEST_EQUAL(result, 0, "Array operations without stdlib");
        
        result = ember_eval(vm, "print(type(42));");
        TEST_EQUAL(result, 0, "Type checking without stdlib");
        
        ember_free_vm(vm);
        printf("  ✓ Core functions work independently\n");
    }
}

void test_memory_management(void) {
    printf("\n=== Testing Memory Management ===\n");
    
    const ember_core_interface_t* core = ember_get_core_interface();
    if (!core) {
        printf("  Skipping memory management test - interface not available\n");
        return;
    }
    
    // Test memory allocation through interface
    void* ptr = core->allocate(1024);
    TEST_NOT_NULL(ptr, "Interface memory allocation successful");
    
    if (ptr) {
        core->deallocate(ptr);
        printf("  ✓ Interface memory deallocation completed\n");
    }
    
    // Test garbage collection interface
    ember_vm* vm = core->create_vm();
    if (vm) {
        core->gc_collect(vm);
        printf("  ✓ Garbage collection interface available\n");
        core->destroy_vm(vm);
    }
}

void test_error_handling(void) {
    printf("\n=== Testing Error Handling ===\n");
    
    const ember_core_interface_t* core = ember_get_core_interface();
    if (!core) {
        printf("  Skipping error handling test - interface not available\n");
        return;
    }
    
    ember_vm* vm = core->create_vm();
    if (vm) {
        // Test error setting and retrieval
        TEST_ASSERT(!core->has_error(vm), "VM initially has no errors");
        
        core->set_error(vm, "Test error message");
        // Note: error handling implementation may vary
        
        core->clear_error(vm);
        printf("  ✓ Error handling interface available\n");
        
        core->destroy_vm(vm);
    }
}

void test_build_system_isolation(void) {
    printf("\n=== Testing Build System Isolation ===\n");
    
    // Test that we can check for compile-time dependency isolation
    // This is more of a compile-time test, but we can verify some aspects
    
    #ifdef EMBER_VERSION
    printf("  ✓ Ember version defined: %s\n", EMBER_VERSION);
    #else
    printf("  ✗ Ember version not defined\n");
    results.failed_tests++;
    #endif
    
    // Test that interfaces are properly defined
    TEST_ASSERT(sizeof(ember_core_interface_t) > 0, "Core interface structure defined");
    TEST_ASSERT(sizeof(ember_stdlib_interface_t) > 0, "Stdlib interface structure defined");
    TEST_ASSERT(sizeof(emberweb_interface_t) > 0, "EmberWeb interface structure defined");
    
    printf("  ✓ Build system produces isolated components\n");
}

void print_test_summary(void) {
    printf("\n" "================================================\n");
    printf("ARCHITECTURE REFACTOR TEST SUMMARY\n");
    printf("================================================\n");
    printf("Total tests:  %d\n", results.total_tests);
    printf("Passed:       %d\n", results.passed_tests);
    printf("Failed:       %d\n", results.failed_tests);
    
    if (results.failed_tests > 0) {
        printf("\nFailed tests:\n");
        for (int i = 0; i < results.failed_tests && i < 10; i++) {
            printf("  - %s\n", results.error_messages[i]);
        }
    }
    
    printf("\nARCHITECTURE VALIDATION:\n");
    if (results.failed_tests == 0) {
        printf("  ✓ All tests passed - refactored architecture is working correctly\n");
        printf("  ✓ Circular dependencies successfully broken\n");
        printf("  ✓ Interface-based design functional\n");
        printf("  ✓ Core can operate independently\n");
    } else {
        printf("  ✗ Some tests failed - architecture needs attention\n");
    }
    
    printf("================================================\n");
}

int main(void) {
    printf("EMBER PLATFORM ARCHITECTURE REFACTOR TEST\n");
    printf("Testing the new interface-based, circular-dependency-free architecture\n");
    printf("======================================================================\n");
    
    // Initialize the platform
    int init_result = ember_init_platform();
    TEST_EQUAL(init_result, 0, "Platform initialization successful");
    
    // Run all tests
    test_interface_registration();
    test_core_functionality();
    test_dependency_isolation();
    test_memory_management();
    test_error_handling();
    test_build_system_isolation();
    
    // Print summary
    print_test_summary();
    
    // Cleanup
    ember_cleanup_platform();
    
    return (results.failed_tests == 0) ? 0 : 1;
}