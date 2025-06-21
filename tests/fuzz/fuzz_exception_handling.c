#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <assert.h>
#include "../../include/ember.h"

// Fuzzing test for exception handling security
// Tests edge cases and potential security vulnerabilities

// Generate random exception handling code
char* generate_random_exception_code(int complexity) {
    static char buffer[4096];
    buffer[0] = '\0';
    
    int nest_level = rand() % (complexity + 1);
    
    for (int i = 0; i < nest_level; i++) {
        strcat(buffer, "try {\n");
    }
    
    // Add some random operations
    int operations = rand() % 10 + 1;
    for (int i = 0; i < operations; i++) {
        int op_type = rand() % 4;
        switch (op_type) {
            case 0:
                strcat(buffer, "  x = x + 1\n");
                break;
            case 1:
                strcat(buffer, "  throw \"random exception\"\n");
                break;
            case 2:
                strcat(buffer, "  y = [1, 2, 3]\n");
                break;
            case 3:
                strcat(buffer, "  z = {\"key\": \"value\"}\n");
                break;
        }
    }
    
    // Close with catch/finally blocks
    for (int i = 0; i < nest_level; i++) {
        int block_type = rand() % 3;
        switch (block_type) {
            case 0:  // catch only
                strcat(buffer, "} catch (e) {\n  print(\"caught: \" + str(e))\n");
                break;
            case 1:  // finally only
                strcat(buffer, "} finally {\n  print(\"finally\")\n");
                break;
            case 2:  // both
                strcat(buffer, "} catch (e) {\n  print(\"caught: \" + str(e))\n} finally {\n  print(\"finally\")\n");
                break;
        }
        strcat(buffer, "}\n");
    }
    
    return buffer;
}

// Test stack overflow protection
void test_stack_overflow_protection() {
    printf("Testing stack overflow protection...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Generate deeply nested try blocks
    char deep_nesting[8192];
    strcpy(deep_nesting, "x = 0\n");
    
    // Create 100 nested try blocks (should be caught by limits)
    for (int i = 0; i < 100; i++) {
        strcat(deep_nesting, "try {\n  x = x + 1\n");
    }
    
    strcat(deep_nesting, "  throw \"deep exception\"\n");
    
    for (int i = 0; i < 100; i++) {
        strcat(deep_nesting, "} catch (e) {\n  print(\"level \" + str(x))\n}\n");
    }
    
    // This should either succeed with proper limits or fail gracefully
    int result = ember_eval(vm, deep_nesting);
    // We don't assert the result as it may legitimately fail due to limits
    
    ember_free_vm(vm);
    printf("Stack overflow protection test completed\n");
}

// Test exception handler limit enforcement
void test_handler_limits() {
    printf("Testing exception handler limits...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Try to create more handlers than the limit
    char many_handlers[4096];
    strcpy(many_handlers, "x = 0\n");
    
    // Create many nested exception handlers
    for (int i = 0; i < 50; i++) {
        strcat(many_handlers, "try {\n");
    }
    
    strcat(many_handlers, "  throw \"test\"\n");
    
    for (int i = 0; i < 50; i++) {
        strcat(many_handlers, "} catch (e) {\n  x = x + 1\n}\n");
    }
    
    int result = ember_eval(vm, many_handlers);
    // Should either succeed within limits or fail gracefully
    
    ember_free_vm(vm);
    printf("Handler limits test completed\n");
}

// Test malformed exception handling code
void test_malformed_code() {
    printf("Testing malformed exception handling code...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Array of malformed code snippets
    const char* malformed_codes[] = {
        "try { } catch { }",  // Missing exception variable
        "try { }",            // Missing catch/finally
        "catch (e) { }",      // Catch without try
        "finally { }",        // Finally without try
        "try { } catch (e) { } catch (f) { }",  // Multiple catch blocks
        "try { throw } catch (e) { }",  // Throw without value
        "try { } catch () { }",  // Empty catch variable
        NULL
    };
    
    for (int i = 0; malformed_codes[i] != NULL; i++) {
        printf("  Testing malformed code %d...\n", i + 1);
        int result = ember_eval(vm, malformed_codes[i]);
        // All of these should fail to compile
        assert(result != 0);
    }
    
    ember_free_vm(vm);
    printf("Malformed code tests passed\n");
}

// Test memory exhaustion scenarios
void test_memory_exhaustion() {
    printf("Testing memory exhaustion scenarios...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Create many exception objects to test memory limits
    const char* memory_test = 
        "for (i = 0; i < 1000; i = i + 1) {\n"
        "  try {\n"
        "    # Create large string for exception\n"
        "    big_string = \"\"\n"
        "    for (j = 0; j < 100; j = j + 1) {\n"
        "      big_string = big_string + \"ABCDEFGHIJ\"\n"
        "    }\n"
        "    throw big_string\n"
        "  } catch (e) {\n"
        "    # Exception should be collected by GC\n"
        "    if (i % 100 == 0) {\n"
        "      print(\"Iteration: \" + str(i))\n"
        "    }\n"
        "  }\n"
        "}\n"
        "print(\"Memory exhaustion test completed\")\n";
    
    int result = ember_eval(vm, memory_test);
    assert(result == 0); // Should succeed with proper memory management
    
    ember_free_vm(vm);
    printf("Memory exhaustion test passed\n");
}

// Test exception propagation edge cases
void test_exception_propagation() {
    printf("Testing exception propagation edge cases...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test exception in finally block
    const char* finally_exception = 
        "try {\n"
        "  try {\n"
        "    throw \"original exception\"\n"
        "  } finally {\n"
        "    throw \"finally exception\"\n"  // This should override the original
        "  }\n"
        "} catch (e) {\n"
        "  print(\"Caught: \" + e)\n"
        "}\n";
    
    int result1 = ember_eval(vm, finally_exception);
    assert(result1 == 0);
    
    // Test rethrow without exception
    const char* invalid_rethrow = 
        "try {\n"
        "  print(\"no exception here\")\n"
        "} catch (e) {\n"
        "  # This catch block shouldn't execute\n"
        "  throw e  # This would be a rethrow if we were in exception state\n"
        "}\n";
    
    int result2 = ember_eval(vm, invalid_rethrow);
    assert(result2 == 0); // Should succeed as catch block doesn't execute
    
    ember_free_vm(vm);
    printf("Exception propagation tests passed\n");
}

// Random fuzzing test
void test_random_fuzzing() {
    printf("Testing random exception handling code...\n");
    
    srand(time(NULL));
    
    for (int test = 0; test < 50; test++) {
        ember_vm* vm = ember_new_vm();
        assert(vm != NULL);
        
        char* code = generate_random_exception_code(test % 5 + 1);
        printf("  Fuzz test %d (complexity %d)...\n", test + 1, test % 5 + 1);
        
        // Run the code - it may succeed or fail, but shouldn't crash
        int result = ember_eval(vm, code);
        
        // Verify VM state is still valid
        assert(vm->stack_top >= 0);
        assert(vm->stack_top <= EMBER_STACK_MAX);
        assert(vm->exception_handler_count >= 0);
        assert(vm->finally_block_count >= 0);
        
        ember_free_vm(vm);
    }
    
    printf("Random fuzzing tests completed\n");
}

int main() {
    printf("Starting exception handling fuzzing tests...\n\n");
    
    test_stack_overflow_protection();
    test_handler_limits();
    test_malformed_code();
    test_memory_exhaustion();
    test_exception_propagation();
    test_random_fuzzing();
    
    printf("\nAll exception handling fuzzing tests completed!\n");
    return 0;
}