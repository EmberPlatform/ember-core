#include <stdio.h>
#include <stdlib.h>
#include <assert.h>
#include "ember.h"

int main() {
    printf("Testing basic exception handling implementation...\n");
    
    ember_vm* vm = ember_new_vm();

    
    UNUSED(vm);
    if (!vm) {
        printf("ERROR: Failed to create VM\n");
        return 1;
    }
    
    printf("✓ VM created successfully\n");
    
    // Test 1: Simple try/catch
    printf("\nTest 1: Basic try/catch\n");
    const char* test1 = 
        "try {\n"
        "    print(\"In try block\")\n"
        "    throw \"Test exception\"\n"
        "    print(\"This should not print\")\n"
        "} catch (e) {\n"
        "    print(\"Caught: \" + e)\n"
        "}\n"
        "print(\"After try/catch\")\n";

    UNUSED(test1);
    
    int result1 = ember_eval(vm, test1);

    
    UNUSED(result1);
    printf("Result: %s\n", result1 == 0 ? "SUCCESS" : "FAILED");
    
    // Test 2: Try/finally
    printf("\nTest 2: Try/finally\n");
    const char* test2 = 
        "try {\n"
        "    print(\"In try block\")\n"
        "} finally {\n"
        "    print(\"In finally block\")\n"
        "}\n"
        "print(\"After try/finally\")\n";

    UNUSED(test2);
    
    int result2 = ember_eval(vm, test2);

    
    UNUSED(result2);
    printf("Result: %s\n", result2 == 0 ? "SUCCESS" : "FAILED");
    
    // Test 3: Try/catch/finally
    printf("\nTest 3: Try/catch/finally\n");
    const char* test3 = 
        "try {\n"
        "    print(\"In try block\")\n"
        "    throw \"Another exception\"\n"
        "} catch (err) {\n"
        "    print(\"In catch: \" + err)\n"
        "} finally {\n"
        "    print(\"In finally block\")\n"
        "}\n"
        "print(\"All done\")\n";

    UNUSED(test3);
    
    int result3 = ember_eval(vm, test3);

    
    UNUSED(result3);
    printf("Result: %s\n", result3 == 0 ? "SUCCESS" : "FAILED");
    
    ember_free_vm(vm);
    
    printf("\nException handling tests completed!\n");
    printf("Summary: Test1=%s, Test2=%s, Test3=%s\n", 
           result1 == 0 ? "PASS" : "FAIL",
           result2 == 0 ? "PASS" : "FAIL", 
           result3 == 0 ? "PASS" : "FAIL");
    
    return (result1 == 0 && result2 == 0 && result3 == 0) ? 0 : 1;
}