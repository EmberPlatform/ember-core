#ifndef EMBER_VALUE_H
#define EMBER_VALUE_H

#include "ember.h"
#include <stddef.h>

// Value creation functions
ember_value ember_make_number(double num);
ember_value ember_make_bool(int b);
ember_value ember_make_string(const char* str);
ember_value ember_make_string_gc(ember_vm* vm, const char* str);
ember_value ember_make_array(ember_vm* vm, int capacity);
ember_value ember_make_hash_map(ember_vm* vm, int capacity);
ember_value ember_make_class(ember_vm* vm, const char* name);
ember_value ember_make_instance(ember_vm* vm, ember_class* klass);
ember_value ember_make_bound_method(ember_vm* vm, ember_value receiver, ember_value method);
ember_value ember_make_nil(void);

// Value manipulation
void free_ember_value(ember_value value);
ember_value copy_ember_value(ember_value value);
int values_equal(ember_value a, ember_value b);
void print_value(ember_value value);

// String operations
ember_string* allocate_string(ember_vm* vm, char* chars, int length);
ember_string* copy_string(ember_vm* vm, const char* chars, int length);
ember_value concatenate_strings(ember_vm* vm, ember_value a, ember_value b);

// String interning
void init_string_intern_table(ember_vm* vm);
void free_string_intern_table(ember_vm* vm);
ember_string* intern_string(ember_vm* vm, const char* chars, int length);
ember_string* find_interned_string(ember_vm* vm, const char* chars, int length);

// Array operations
ember_array* allocate_array(ember_vm* vm, int capacity);
void array_push(ember_array* array, ember_value value);

// Hash map operations
ember_hash_map* allocate_hash_map(ember_vm* vm, int capacity);
void hash_map_set(ember_hash_map* map, ember_value key, ember_value value);
void hash_map_set_with_vm(ember_vm* vm, ember_hash_map* map, ember_value key, ember_value value);
ember_value hash_map_get(ember_hash_map* map, ember_value key);
int hash_map_has_key(ember_hash_map* map, ember_value key);
uint32_t hash_value(ember_value value);

// OOP operations
ember_class* allocate_class(ember_vm* vm, const char* name);
ember_instance* allocate_instance(ember_vm* vm, ember_class* klass);
ember_bound_method* allocate_bound_method(ember_vm* vm, ember_value receiver, ember_value method);

// Garbage collection helpers
ember_object* allocate_object(ember_vm* vm, size_t size, ember_object_type type);

#endif // EMBER_VALUE_H