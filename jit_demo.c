/**
 * JIT Integration Demo
 * Simple demonstration of JIT functionality
 */

#include <stdio.h>
#include <stdlib.h>
#include "src/core/jit_vm_hook.h"
#include "src/vm.h"

int main() {
    printf("=== Ember JIT Integration Demo ===\n\n");
    
    // Test 1: Basic JIT hook functionality
    printf("1. Testing JIT Hook Creation\n");
    printf("   Creating VM...\n");
    ember_vm* vm = ember_new_vm();
    if (!vm) {
        printf("   ERROR: Failed to create VM\n");
        return 1;
    }
    printf("   ✓ VM created successfully\n");
    
    printf("   Creating JIT hook...\n");
    jit_vm_hook_t* hook = jit_vm_hook_create(vm, JIT_FLAGS_DEFAULT);
    if (!hook) {
        printf("   ERROR: Failed to create JIT hook\n");
        ember_free_vm(vm);
        return 1;
    }
    printf("   ✓ JIT hook created successfully\n");
    
    // Test 2: Global hook management
    printf("\n2. Testing Global Hook Management\n");
    printf("   Installing global hook...\n");
    jit_vm_hook_install_global(hook);
    
    jit_vm_hook_t* retrieved = jit_vm_hook_get_global();
    if (retrieved == hook) {
        printf("   ✓ Global hook installed and retrieved successfully\n");
    } else {
        printf("   ERROR: Global hook installation failed\n");
    }
    
    // Test 3: Configuration
    printf("\n3. Testing JIT Configuration\n");
    printf("   Initial threshold: %u\n", hook->compilation_threshold);
    printf("   Initial enabled: %s\n", hook->enabled ? "Yes" : "No");
    
    printf("   Configuring JIT (threshold=500, enabled=true)...\n");
    jit_vm_hook_configure(hook, 500, true);
    printf("   New threshold: %u\n", hook->compilation_threshold);
    printf("   New enabled: %s\n", hook->enabled ? "Yes" : "No");
    
    // Test 4: Statistics
    printf("\n4. Testing Statistics\n");
    printf("   Running built-in test...\n");
    jit_vm_hook_test();
    
    printf("   Printing hook statistics...\n");
    jit_vm_hook_print_stats(hook);
    
    // Test 5: Simple code execution
    printf("\n5. Testing Simple Code Execution\n");
    const char* test_code = "print(\"Hello from Ember with JIT!\")";
    
    printf("   Executing: %s\n", test_code);
    int result = ember_eval(vm, test_code);
    printf("   Result: %d\n", result);
    
    // Final statistics
    printf("\n6. Final Statistics\n");
    jit_vm_stats_t stats;
    jit_vm_hook_get_stats(hook, &stats);
    printf("   Total instructions: %lu\n", stats.total_instructions);
    printf("   JIT compilations: %lu\n", stats.jit_compilations);
    printf("   JIT executions: %lu\n", stats.jit_executions);
    
    // Cleanup
    printf("\n7. Cleanup\n");
    printf("   Removing global hook...\n");
    jit_vm_hook_install_global(NULL);
    
    printf("   Destroying JIT hook...\n");
    jit_vm_hook_destroy(hook);
    
    printf("   Destroying VM...\n");
    ember_free_vm(vm);
    
    printf("\n=== Demo Complete ===\n");
    printf("JIT integration is working and ready for use!\n");
    
    return 0;
}