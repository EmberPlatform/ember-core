#include "ember.h"
#include "core/vm_pool/vm_pool_secure.h"
#include "runtime/value/value.h"
#include <stdlib.h>
#include <stdio.h>

// API wrapper for the secure VM pool implementation

int ember_pool_init(const vm_pool_config_t* config) {
    // Convert from public API config to secure config
    vm_pool_secure_config_t secure_config;
    
    if (config) {
        secure_config.initial_size = config->initial_size;
        secure_config.max_size = config->initial_size * 4; // Conservative max size
        secure_config.chunk_size = config->chunk_size;
        secure_config.thread_cache_size = config->thread_cache_size;
        secure_config.max_vms_per_thread = config->max_vms_per_thread;
        secure_config.allocation_rate_limit = config->rate_limit_max_allocs;
        secure_config.security_flags = VM_POOL_SECURE_CLEAR_MEMORY | 
                                      VM_POOL_SECURE_RATE_LIMIT |
                                      VM_POOL_SECURE_AUDIT_LOG;
    } else {
        secure_config = ember_pool_default_config();
    }
    
    int result = ember_pool_init_secure(&secure_config);
    return (result == VM_POOL_SUCCESS) ? 0 : -1;
}

void ember_pool_cleanup(void) {
    ember_pool_cleanup_secure();
}

ember_vm* ember_pool_get_vm(void) {
    return ember_pool_get_vm_secure();
}

void ember_pool_release_vm(ember_vm* vm) {
    ember_pool_release_vm_secure(vm);
}

// Additional utility functions for stress/fuzz testing
int ember_pool_get_pool_stats(void) {
    size_t active = 0, allocated = 0;
    uint64_t violations = 0;
    
    if (ember_pool_get_stats(&active, &allocated, &violations) == VM_POOL_SUCCESS) {
        printf("Pool Stats: Active=%zu, Allocated=%zu, Violations=%llu\n", 
               active, allocated, (unsigned long long)violations);
        return (violations == 0) ? 0 : -1;
    }
    return -1;
}