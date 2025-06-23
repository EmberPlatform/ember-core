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

// Module loading function
ember_value ember_load_module(ember_vm* vm, const char* module_name, const char* current_file);

// Native functions for import/require
ember_value ember_native_import(ember_vm* vm, int argc, ember_value* argv);
ember_value ember_native_require(ember_vm* vm, int argc, ember_value* argv);

// Register module system native functions
void ember_module_system_register_natives(ember_vm* vm);

// Clean up module cache
void ember_module_system_cleanup(void);

#ifdef __cplusplus
}
#endif

#endif // EMBER_MODULE_SYSTEM_H