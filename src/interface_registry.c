/*
 * Ember Platform Interface Registry
 * 
 * Manages interface registration and provides decoupled access to platform modules
 */

#include "ember_interfaces.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Global interface storage
static const ember_core_interface_t* g_core_interface = NULL;
static const ember_stdlib_interface_t* g_stdlib_interface = NULL;
static const emberweb_interface_t* g_emberweb_interface = NULL;

// Interface validation helpers
static int validate_function_pointers(const void* interface, size_t interface_size) {
    if (!interface) return 0;
    
    // Basic validation - check that interface structure is not null
    // More sophisticated validation could check individual function pointers
    return interface_size > 0;
}

// Core interface management
void ember_register_core_interface(const ember_core_interface_t* interface) {
    if (!interface) {
        fprintf(stderr, "Warning: Attempting to register NULL core interface\n");
        return;
    }
    
    if (!ember_validate_core_interface(interface)) {
        fprintf(stderr, "Error: Invalid core interface\n");
        return;
    }
    
    if (g_core_interface) {
        fprintf(stderr, "Warning: Core interface already registered, overwriting\n");
    }
    
    g_core_interface = interface;
    printf("Ember core interface v%s registered\n", interface->version);
}

const ember_core_interface_t* ember_get_core_interface(void) {
    return g_core_interface;
}

const ember_core_interface_t* ember_core(void) {
    if (!g_core_interface) {
        fprintf(stderr, "Error: Core interface not registered\n");
        return NULL;
    }
    return g_core_interface;
}

int ember_validate_core_interface(const ember_core_interface_t* interface) {
    if (!interface) return 0;
    
    // Check essential function pointers
    if (!interface->create_vm || !interface->destroy_vm || !interface->eval_code) {
        return 0;
    }
    
    if (!interface->make_nil || !interface->make_bool || !interface->make_number || !interface->make_string) {
        return 0;
    }
    
    if (!interface->register_native_function) {
        return 0;
    }
    
    return 1;
}

// Standard library interface management
void ember_register_stdlib_interface(const ember_stdlib_interface_t* interface) {
    if (!interface) {
        fprintf(stderr, "Warning: Attempting to register NULL stdlib interface\n");
        return;
    }
    
    if (!ember_validate_stdlib_interface(interface)) {
        fprintf(stderr, "Error: Invalid stdlib interface\n");
        return;
    }
    
    if (g_stdlib_interface) {
        fprintf(stderr, "Warning: Stdlib interface already registered, overwriting\n");
    }
    
    g_stdlib_interface = interface;
    printf("Ember stdlib interface v%s registered\n", interface->version);
}

const ember_stdlib_interface_t* ember_get_stdlib_interface(void) {
    return g_stdlib_interface;
}

const ember_stdlib_interface_t* ember_stdlib(void) {
    if (!g_stdlib_interface) {
        fprintf(stderr, "Error: Stdlib interface not registered\n");
        return NULL;
    }
    return g_stdlib_interface;
}

int ember_validate_stdlib_interface(const ember_stdlib_interface_t* interface) {
    if (!interface) return 0;
    
    // Check essential function pointers
    if (!interface->init_all || !interface->cleanup_all) {
        return 0;
    }
    
    return 1;
}

// EmberWeb interface management
void ember_register_emberweb_interface(const emberweb_interface_t* interface) {
    if (!interface) {
        fprintf(stderr, "Warning: Attempting to register NULL emberweb interface\n");
        return;
    }
    
    if (!ember_validate_emberweb_interface(interface)) {
        fprintf(stderr, "Error: Invalid emberweb interface\n");
        return;
    }
    
    if (g_emberweb_interface) {
        fprintf(stderr, "Warning: EmberWeb interface already registered, overwriting\n");
    }
    
    g_emberweb_interface = interface;
    printf("EmberWeb interface v%s registered\n", interface->version);
}

const emberweb_interface_t* ember_get_emberweb_interface(void) {
    return g_emberweb_interface;
}

const emberweb_interface_t* emberweb(void) {
    if (!g_emberweb_interface) {
        fprintf(stderr, "Error: EmberWeb interface not registered\n");
        return NULL;
    }
    return g_emberweb_interface;
}

int ember_validate_emberweb_interface(const emberweb_interface_t* interface) {
    if (!interface) return 0;
    
    // Check essential function pointers
    if (!interface->init_server || !interface->start_server || !interface->stop_server) {
        return 0;
    }
    
    if (!interface->init_vm_bridge || !interface->cleanup_vm_bridge) {
        return 0;
    }
    
    return 1;
}

// Platform initialization
int ember_init_platform(void) {
    printf("Initializing Ember platform...\n");
    
    // Validate that core interface is available
    if (!g_core_interface) {
        fprintf(stderr, "Error: Core interface must be registered before platform initialization\n");
        return -1;
    }
    
    // Initialize stdlib if available
    if (g_stdlib_interface && g_stdlib_interface->init_all) {
        printf("Initializing standard library...\n");
        // Note: VM will be created by the application, not here
        // The stdlib will be initialized when a VM is created
    }
    
    // EmberWeb will be initialized separately when needed
    
    printf("Ember platform initialization complete\n");
    return 0;
}

void ember_cleanup_platform(void) {
    printf("Cleaning up Ember platform...\n");
    
    // Cleanup is typically handled by individual modules
    // This provides a central cleanup point if needed
    
    g_core_interface = NULL;
    g_stdlib_interface = NULL;
    g_emberweb_interface = NULL;
    
    printf("Ember platform cleanup complete\n");
}