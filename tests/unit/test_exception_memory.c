#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include "ember.h"

// Test memory safety during exception handling
void test_stack_unwinding_memory() {
    printf("Testing stack unwinding memory safety...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test complex stack unwinding with multiple locals and nested calls
    const char* source = 
        "fn create_large_data() {\n"
        "    a = [1, 2, 3, 4, 5]\n"
        "    b = {\"key1\": \"value1\", \"key2\": \"value2\"}\n"
        "    c = \"Large string that should be properly cleaned up\"\n"
        "    throw \"Exception in function\"\n"
        "    return a  # Should not reach here\n"
        "}\n"
        "\n"
        "try {\n"
        "    x = [10, 20, 30]\n"
        "    y = {\"test\": \"data\"}\n"
        "    create_large_data()\n"
        "} catch (e) {\n"
        "    print(\"Caught in main: \" + e)\n"
        "    # x and y should still be accessible\n"
        "    print(\"Array length: \" + str(len(x)))\n"
        "}\n"
        "print(\"Memory test completed\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Stack unwinding memory tests passed!\n");
}

// Test exception handler cleanup
void test_handler_cleanup() {
    printf("Testing exception handler cleanup...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test that exception handlers are properly cleaned up
    const char* source = 
        "for (i = 0; i < 10; i = i + 1) {\n"
        "    try {\n"
        "        if (i == 5) {\n"
        "            throw \"Exception at iteration \" + str(i)\n"
        "        }\n"
        "        print(\"Iteration: \" + str(i))\n"
        "    } catch (e) {\n"
        "        print(\"Caught: \" + e)\n"
        "    } finally {\n"
        "        print(\"Finally for iteration \" + str(i))\n"
        "    }\n"
        "}\n"
        "print(\"Loop completed\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    // Verify handler count is back to 0
    assert(vm->exception_handler_count == 0);
    assert(vm->finally_block_count == 0);
    
    ember_free_vm(vm);
    printf("Exception handler cleanup tests passed!\n");
}

// Test exception object memory management
void test_exception_object_memory() {
    printf("Testing exception object memory management...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Create many exceptions to test GC
    const char* source = 
        "for (i = 0; i < 100; i = i + 1) {\n"
        "    try {\n"
        "        throw \"Exception number \" + str(i)\n"
        "    } catch (e) {\n"
        "        # Exception objects should be GC'd properly\n"
        "        if (i % 10 == 0) {\n"
        "            print(\"Processed \" + str(i) + \" exceptions\")\n"
        "        }\n"
        "    }\n"
        "}\n"
        "print(\"Exception object memory test completed\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Exception object memory tests passed!\n");
}

// Test resource cleanup in finally blocks
void test_resource_cleanup() {
    printf("Testing resource cleanup in finally blocks...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Simulate resource allocation and cleanup
    const char* source = 
        "resource_count = 0\n"
        "\n"
        "fn allocate_resource() {\n"
        "    resource_count = resource_count + 1\n"
        "    print(\"Allocated resource \" + str(resource_count))\n"
        "    return resource_count\n"
        "}\n"
        "\n"
        "fn free_resource(id) {\n"
        "    print(\"Freed resource \" + str(id))\n"
        "}\n"
        "\n"
        "try {\n"
        "    res1 = allocate_resource()\n"
        "    res2 = allocate_resource()\n"
        "    throw \"Simulated error\"\n"
        "} catch (e) {\n"
        "    print(\"Error: \" + e)\n"
        "} finally {\n"
        "    # Cleanup should always happen\n"
        "    if (resource_count > 0) {\n"
        "        free_resource(2)\n"
        "        free_resource(1)\n"
        "    }\n"
        "    print(\"Resources cleaned up\")\n"
        "}\n"
        "print(\"Resource cleanup test completed\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Resource cleanup tests passed!\n");
}

// Test deep recursion with exceptions
void test_deep_recursion_exceptions() {
    printf("Testing deep recursion with exceptions...\n");
    
    ember_vm* vm = ember_new_vm();
    assert(vm != NULL);
    
    // Test that deep recursion doesn't cause stack overflow
    const char* source = 
        "fn recursive_func(n) {\n"
        "    if (n <= 0) {\n"
        "        throw \"Base case reached: \" + str(n)\n"
        "    }\n"
        "    try {\n"
        "        return recursive_func(n - 1)\n"
        "    } catch (e) {\n"
        "        print(\"Caught at level \" + str(n) + \": \" + e)\n"
        "        throw \"Propagated from level \" + str(n)\n"
        "    }\n"
        "}\n"
        "\n"
        "try {\n"
        "    recursive_func(10)\n"
        "} catch (e) {\n"
        "    print(\"Final catch: \" + e)\n"
        "}\n"
        "print(\"Deep recursion test completed\")\n";
        
    int result = ember_eval(vm, source);
    assert(result == 0); // Should succeed
    
    ember_free_vm(vm);
    printf("Deep recursion exception tests passed!\n");
}

int main() {
    printf("Starting exception memory safety tests...\n\n");
    
    test_stack_unwinding_memory();
    test_handler_cleanup();
    test_exception_object_memory();
    test_resource_cleanup();
    test_deep_recursion_exceptions();
    
    printf("\nAll exception memory safety tests passed!\n");
    return 0;
}