/**
 * Enhanced Module System for Ember
 * Header file for module loading and management
 */

#ifndef EMBER_MODULE_SYSTEM_H
#define EMBER_MODULE_SYSTEM_H

#include "../vm.h"
#include "value/value.h"

#ifdef __cplusplus
extern "C" {
#endif

// Module context for tracking exports and imports
typedef struct ember_module_context {
    char* module_path;                // Path to this module
    ember_value exports;              // Hash map of exported values
    ember_value default_export;       // Default export value
    bool has_default_export;          // Whether default export exists
    bool is_loaded;                   // Whether module is fully loaded
    bool is_loading;                  // Whether module is currently loading (for circular deps)
    struct ember_module_context* next; // Linked list for module registry
} ember_module_context;

// Export types
typedef enum {
    EXPORT_NAMED,     // export { name }
    EXPORT_DEFAULT,   // export default value
    EXPORT_ALL        // export * from "module"
} ember_export_type;

// Import types
typedef enum {
    IMPORT_NAMED,     // import { name } from "module"
    IMPORT_DEFAULT,   // import name from "module"
    IMPORT_NAMESPACE, // import * as name from "module"
    IMPORT_SIDE_EFFECT // import "module"
} ember_import_type;

// Module loading functions
ember_value ember_load_module(ember_vm* vm, const char* module_name, const char* current_file);
ember_module_context* ember_get_module_context(ember_vm* vm, const char* module_path);
ember_module_context* ember_create_module_context(ember_vm* vm, const char* module_path);

// Export functions
void ember_module_export(ember_vm* vm, const char* name, ember_value value);
void ember_module_export_default(ember_vm* vm, ember_value value);
void ember_module_export_all(ember_vm* vm, const char* from_module);

// Import functions
ember_value ember_module_import_named(ember_vm* vm, const char* module_name, const char* export_name);
ember_value ember_module_import_default(ember_vm* vm, const char* module_name);
ember_value ember_module_import_namespace(ember_vm* vm, const char* module_name);
void ember_module_import_side_effect(ember_vm* vm, const char* module_name);

// Native functions for import/require
ember_value ember_native_import(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_require(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_export(ember_vm* vm, int argc, ember_value* argv);

// Module resolution and path handling
char* ember_resolve_module_path(const char* module_name, const char* current_file);
bool ember_is_core_module(const char* module_name);
bool ember_module_exists(const char* module_path);

// Circular dependency detection
bool ember_detect_circular_dependency(ember_vm* vm, const char* module_path);
void ember_mark_module_loading(ember_vm* vm, const char* module_path);
void ember_mark_module_loaded(ember_vm* vm, const char* module_path);

// Module caching
void ember_cache_module(ember_vm* vm, const char* module_path, ember_module_context* context);
ember_module_context* ember_get_cached_module(ember_vm* vm, const char* module_path);
void ember_clear_module_cache(ember_vm* vm);

// Register module system native functions
void ember_module_system_register_natives(ember_vm* vm);

// Clean up module cache
void ember_module_system_cleanup(void);

// Module context management
void ember_push_module_context(ember_vm* vm, ember_module_context* context);
ember_module_context* ember_pop_module_context(ember_vm* vm);
ember_module_context* ember_get_current_module_context(ember_vm* vm);

// Standard library module initialization
ember_value ember_init_core_module(ember_vm* vm, const char* module_name);

#ifdef __cplusplus
}
#endif

#endif // EMBER_MODULE_SYSTEM_H