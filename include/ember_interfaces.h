#ifndef EMBER_INTERFACES_H
#define EMBER_INTERFACES_H

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/*
 * Ember Platform Interface Definitions
 * 
 * This file defines the interface contracts between ember-core, ember-stdlib, 
 * and emberweb modules to break circular dependencies and enable proper layering.
 */

// Forward declarations
typedef struct ember_vm ember_vm;
typedef struct ember_value ember_value;

// Core function pointer types
typedef ember_value (*ember_native_function_t)(ember_vm* vm, int argc, ember_value* argv);
typedef int (*ember_module_init_t)(ember_vm* vm);
typedef void (*ember_module_cleanup_t)(ember_vm* vm);

// Core interface - provided by ember-core
typedef struct ember_core_interface {
    // Version information
    const char* version;
    
    // VM management
    ember_vm* (*create_vm)(void);
    void (*destroy_vm)(ember_vm* vm);
    int (*eval_code)(ember_vm* vm, const char* source);
    
    // Value creation and manipulation
    ember_value (*make_nil)(void);
    ember_value (*make_bool)(int value);
    ember_value (*make_number)(double value);
    ember_value (*make_string)(const char* str);
    ember_value (*make_array)(ember_vm* vm, int capacity);
    ember_value (*make_hash_map)(ember_vm* vm, int capacity);
    
    // Value type checking
    int (*is_nil)(ember_value value);
    int (*is_bool)(ember_value value);
    int (*is_number)(ember_value value);
    int (*is_string)(ember_value value);
    int (*is_array)(ember_value value);
    int (*is_hash_map)(ember_value value);
    
    // Value extraction
    int (*get_bool)(ember_value value);
    double (*get_number)(ember_value value);
    const char* (*get_string)(ember_value value);
    size_t (*get_string_length)(ember_value value);
    
    // Function registration
    void (*register_native_function)(ember_vm* vm, const char* name, ember_native_function_t func);
    
    // Error handling
    void (*set_error)(ember_vm* vm, const char* message);
    const char* (*get_error)(ember_vm* vm);
    int (*has_error)(ember_vm* vm);
    void (*clear_error)(ember_vm* vm);
    
    // Memory management
    void* (*allocate)(size_t size);
    void (*deallocate)(void* ptr);
    void (*gc_collect)(ember_vm* vm);
    
} ember_core_interface_t;

// Standard library interface - provided by ember-stdlib
typedef struct ember_stdlib_interface {
    // Version information
    const char* version;
    
    // Module initialization
    int (*init_crypto)(ember_vm* vm);
    int (*init_datetime)(ember_vm* vm);
    int (*init_http)(ember_vm* vm);
    int (*init_io)(ember_vm* vm);
    int (*init_json)(ember_vm* vm);
    int (*init_math)(ember_vm* vm);
    int (*init_regex)(ember_vm* vm);
    int (*init_string)(ember_vm* vm);
    int (*init_template)(ember_vm* vm);
    int (*init_database)(ember_vm* vm);
    int (*init_session)(ember_vm* vm);
    int (*init_websocket)(ember_vm* vm);
    
    // Full stdlib initialization
    int (*init_all)(ember_vm* vm, const ember_core_interface_t* core_interface);
    void (*cleanup_all)(ember_vm* vm);
    
    // Configuration
    int (*configure)(const void* config);
    
} ember_stdlib_interface_t;

// Web server interface - provided by emberweb
typedef struct emberweb_interface {
    // Version information
    const char* version;
    
    // Server lifecycle
    int (*init_server)(const char* config_file);
    int (*start_server)(void);
    void (*stop_server)(void);
    void (*cleanup_server)(void);
    
    // Request/response handling
    int (*register_route)(const char* method, const char* path, const char* handler);
    int (*register_static_directory)(const char* path, const char* directory);
    
    // VM integration
    int (*init_vm_bridge)(ember_vm* vm, const ember_core_interface_t* core_interface);
    void (*cleanup_vm_bridge)(ember_vm* vm);
    
} emberweb_interface_t;

// Interface registration functions
extern const ember_core_interface_t* ember_get_core_interface(void);
extern const ember_stdlib_interface_t* ember_get_stdlib_interface(void);
extern const emberweb_interface_t* ember_get_emberweb_interface(void);

// Interface registration (for module providers)
void ember_register_core_interface(const ember_core_interface_t* interface);
void ember_register_stdlib_interface(const ember_stdlib_interface_t* interface);
void ember_register_emberweb_interface(const emberweb_interface_t* interface);

// Interface validation
int ember_validate_core_interface(const ember_core_interface_t* interface);
int ember_validate_stdlib_interface(const ember_stdlib_interface_t* interface);
int ember_validate_emberweb_interface(const emberweb_interface_t* interface);

// Global interface accessors (for runtime use)
const ember_core_interface_t* ember_core(void);
const ember_stdlib_interface_t* ember_stdlib(void);
const emberweb_interface_t* emberweb(void);

// Initialization order control
int ember_init_platform(void);
void ember_cleanup_platform(void);

#ifdef __cplusplus
}
#endif

#endif // EMBER_INTERFACES_H