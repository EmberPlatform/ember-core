/*
 * Ember Core Interface Implementation
 * 
 * Provides the core interface implementation for ember-core
 */

#include "ember_interfaces.h"
#include "ember.h"
#include <string.h>
#include <stdlib.h>

// Forward declarations of internal functions
extern ember_vm* ember_new_vm(void);
extern void ember_free_vm(ember_vm* vm);
extern int ember_eval(ember_vm* vm, const char* source);
extern void ember_register_func(ember_vm* vm, const char* name, ember_native_func func);

// Implementation functions
static ember_vm* core_create_vm(void) {
    return ember_new_vm();
}

static void core_destroy_vm(ember_vm* vm) {
    ember_free_vm(vm);
}

static int core_eval_code(ember_vm* vm, const char* source) {
    return ember_eval(vm, source);
}

static ember_value core_make_nil(void) {
    return ember_make_nil();
}

static ember_value core_make_bool(int value) {
    return ember_make_bool(value);
}

static ember_value core_make_number(double value) {
    return ember_make_number(value);
}

static ember_value core_make_string(const char* str) {
    return ember_make_string(str);
}

static ember_value core_make_array(ember_vm* vm, int capacity) {
    return ember_make_array(vm, capacity);
}

static ember_value core_make_hash_map(ember_vm* vm, int capacity) {
    return ember_make_hash_map(vm, capacity);
}

static int core_is_nil(ember_value value) {
    return value.type == EMBER_VAL_NIL;
}

static int core_is_bool(ember_value value) {
    return value.type == EMBER_VAL_BOOL;
}

static int core_is_number(ember_value value) {
    return value.type == EMBER_VAL_NUMBER;
}

static int core_is_string(ember_value value) {
    return value.type == EMBER_VAL_STRING;
}

static int core_is_array(ember_value value) {
    return value.type == EMBER_VAL_ARRAY;
}

static int core_is_hash_map(ember_value value) {
    return value.type == EMBER_VAL_HASH_MAP;
}

static int core_get_bool(ember_value value) {
    if (value.type == EMBER_VAL_BOOL) {
        return value.as.bool_val;
    }
    return 0;
}

static double core_get_number(ember_value value) {
    if (value.type == EMBER_VAL_NUMBER) {
        return value.as.number_val;
    }
    return 0.0;
}

static const char* core_get_string(ember_value value) {
    if (value.type == EMBER_VAL_STRING) {
        if (IS_STRING(value)) {
            return AS_CSTRING(value);
        } else {
            return value.as.string_val;
        }
    }
    return NULL;
}

static size_t core_get_string_length(ember_value value) {
    if (value.type == EMBER_VAL_STRING) {
        if (IS_STRING(value)) {
            return AS_STRING(value)->length;
        } else if (value.as.string_val) {
            return strlen(value.as.string_val);
        }
    }
    return 0;
}

static void core_register_native_function(ember_vm* vm, const char* name, ember_native_function_t func) {
    ember_register_func(vm, name, func);
}

static void core_set_error(ember_vm* vm, const char* message) {
    if (vm && message) {
        // Implementation depends on internal error handling
        // This is a placeholder - real implementation would set VM error state
        (void)vm;
        (void)message;
    }
}

static const char* core_get_error(ember_vm* vm) {
    if (vm) {
        // Implementation depends on internal error handling
        // This is a placeholder - real implementation would get VM error state
        (void)vm;
    }
    return NULL;
}

static int core_has_error(ember_vm* vm) {
    if (vm) {
        // Implementation depends on internal error handling
        // This is a placeholder - real implementation would check VM error state
        (void)vm;
    }
    return 0;
}

static void core_clear_error(ember_vm* vm) {
    if (vm) {
        // Implementation depends on internal error handling
        // This is a placeholder - real implementation would clear VM error state
        (void)vm;
    }
}

static void* core_allocate(size_t size) {
    return malloc(size);
}

static void core_deallocate(void* ptr) {
    free(ptr);
}

static void core_gc_collect(ember_vm* vm) {
    if (vm) {
        ember_gc_collect(vm);
    }
}

// Core interface instance
static const ember_core_interface_t core_interface = {
    .version = EMBER_VERSION,
    
    // VM management
    .create_vm = core_create_vm,
    .destroy_vm = core_destroy_vm,
    .eval_code = core_eval_code,
    
    // Value creation
    .make_nil = core_make_nil,
    .make_bool = core_make_bool,
    .make_number = core_make_number,
    .make_string = core_make_string,
    .make_array = core_make_array,
    .make_hash_map = core_make_hash_map,
    
    // Value type checking
    .is_nil = core_is_nil,
    .is_bool = core_is_bool,
    .is_number = core_is_number,
    .is_string = core_is_string,
    .is_array = core_is_array,
    .is_hash_map = core_is_hash_map,
    
    // Value extraction
    .get_bool = core_get_bool,
    .get_number = core_get_number,
    .get_string = core_get_string,
    .get_string_length = core_get_string_length,
    
    // Function registration
    .register_native_function = core_register_native_function,
    
    // Error handling
    .set_error = core_set_error,
    .get_error = core_get_error,
    .has_error = core_has_error,
    .clear_error = core_clear_error,
    
    // Memory management
    .allocate = core_allocate,
    .deallocate = core_deallocate,
    .gc_collect = core_gc_collect,
};

// Interface registration function
void ember_core_register_interface(void) {
    ember_register_core_interface(&core_interface);
}