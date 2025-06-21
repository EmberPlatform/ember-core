#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ember.h"

// Test basic try/catch functionality
void test_basic_try_catch() {
    printf("Testing basic try/catch...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test 1: Basic exception throwing and catching
    const char* source1 = 
        "try {\n"
        "    throw \"Test exception\"\n"
        "    print(\"This should not execute\")\n"
        "} catch (e) {\n"
        "    print(\"Caught exception: \" + e)\n"
        "}\n"
        "print(\"After try/catch\")\n";
        
    int result1 = ember_eval(vm, source1);
    assert(result1 == 0); // Should succeed
    
    // Test 2: Try without exception
    const char* source2 = 
        "try {\n"
        "    print(\"No exception here\")\n"
        "} catch (e) {\n"
        "    print(\"This should not execute\")\n"
        "}\n"
        "print(\"After safe try/catch\")\n";
        
    int result2 = ember_eval(vm, source2);
    assert(result2 == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Basic try/catch tests passed!\n");
}

// Test finally blocks
void test_finally_blocks() {
    printf("Testing finally blocks...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test 1: Finally with exception
    const char* source1 = 
        "try {\n"
        "    throw \"Test exception\"\n"
        "} catch (e) {\n"
        "    print(\"In catch: \" + e)\n"
        "} finally {\n"
        "    print(\"In finally block\")\n"
        "}\n"
        "print(\"After try/catch/finally\")\n";
        
    int result1 = ember_eval(vm, source1);
    assert(result1 == 0); // Should succeed
    
    // Test 2: Finally without exception
    const char* source2 = 
        "try {\n"
        "    print(\"Safe code\")\n"
        "} finally {\n"
        "    print(\"Finally always runs\")\n"
        "}\n"
        "print(\"After try/finally\")\n";
        
    int result2 = ember_eval(vm, source2);
    assert(result2 == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Finally block tests passed!\n");
}

// Test nested try/catch blocks
void test_nested_exceptions() {
    printf("Testing nested try/catch blocks...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    const char* source = 
        "try {\n"
        "    print(\"Outer try\")\n"
        "    try {\n"
        "        print(\"Inner try\")\n"
        "        throw \"Inner exception\"\n"
        "    } catch (inner_e) {\n"
        "        print(\"Inner catch: \" + inner_e)\n"
        "        throw \"Outer exception\"\n"
        "    } finally {\n"
        "        print(\"Inner finally\")\n"
        "    }\n"
        "} catch (outer_e) {\n"
        "    print(\"Outer catch: \" + outer_e)\n"
        "} finally {\n"
        "    print(\"Outer finally\")\n"
        "}\n"
        "print(\"After nested try/catch\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Nested exception tests passed!\n");
}

// Test exception types and objects
void test_exception_objects() {
    printf("Testing exception objects...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test different exception types
    const char* source1 = 
        "try {\n"
        "    throw 42\n"
        "} catch (e) {\n"
        "    print(\"Caught number: \" + str(e))\n"
        "}\n";
        
    int result1 = ember_eval(vm, source1);
    assert(result1 == 0); // Should succeed
    
    const char* source2 = 
        "try {\n"
        "    throw true\n"
        "} catch (e) {\n"
        "    print(\"Caught boolean: \" + str(e))\n"
        "}\n";
        
    int result2 = ember_eval(vm, source2);
    assert(result2 == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Exception object tests passed!\n");
}

// Test error conditions
void test_error_conditions() {
    printf("Testing error conditions...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test 1: Try without catch or finally should fail to compile
    const char* source1 = "try { print(\"test\") }";
    int result1 = ember_eval(vm, source1);
    assert(result1 != 0); // Should fail
    
    // Test 2: Uncaught exception should terminate
    const char* source2 = "throw \"Uncaught exception\"";
    int result2 = ember_eval(vm, source2);
    assert(result2 != 0); // Should fail
    
    ember_free_vm(vm);
    printf("Error condition tests passed!\n");
}

int main() {
    printf("Starting exception handling tests...\n\n");
    
    test_basic_try_catch();
    test_finally_blocks();
    test_nested_exceptions();
    test_exception_objects();
    test_error_conditions();
    
    printf("\nAll exception handling tests passed!\n");
    return 0;
}