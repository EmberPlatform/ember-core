#ifndef EMBER_RUNTIME_H
#define EMBER_RUNTIME_H

#include "value/value.h"
#include "../vm.h"

// Value system
ember_value ember_make_nil(void);
ember_value ember_make_bool(int value);
ember_value ember_make_number(double value);
ember_value ember_make_string(const char* str);
ember_value ember_make_string_gc(ember_vm* vm, const char* str);
ember_value ember_make_array(ember_vm* vm, int capacity);

// Value utilities
int values_equal(ember_value a, ember_value b);
void print_value(ember_value value);
void free_ember_value(ember_value value);

// String operations
ember_value concatenate_strings(ember_vm* vm, ember_value a, ember_value b);

// Built-in functions registration
void register_builtin_functions(ember_vm* vm);

// Module system
int ember_import_module(ember_vm* vm, const char* module_name);

#endif