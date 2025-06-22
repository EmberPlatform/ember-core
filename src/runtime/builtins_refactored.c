/*
 * Ember Core Built-in Functions (Refactored)
 * 
 * This file provides only the core built-in functions that don't depend on stdlib.
 * Standard library functions are now registered via the interface system.
 */

#include "runtime.h"
#include "ember.h"
#include "../vm.h"
#include "value/value.h"
#include "../include/ember_interfaces.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <math.h>

// Helper function to print a value (recursive)
static void print_value(ember_value value) {
    if (value.type == EMBER_VAL_NUMBER) {
        printf("%g", value.as.number_val);
    } else if (value.type == EMBER_VAL_STRING) {
        if (IS_STRING(value)) {
            printf("%s", AS_CSTRING(value));
        } else if (value.as.string_val) {
            printf("%s", value.as.string_val);
        }
    } else if (value.type == EMBER_VAL_BOOL) {
        printf("%s", value.as.bool_val ? "true" : "false");
    } else if (value.type == EMBER_VAL_NIL) {
        printf("nil");
    } else if (value.type == EMBER_VAL_ARRAY) {
        ember_array* array = AS_ARRAY(value);
        printf("[");
        for (int j = 0; j < array->length; j++) {
            if (j > 0) printf(", ");
            print_value(array->elements[j]);
        }
        printf("]");
    } else if (value.type == EMBER_VAL_HASH_MAP) {
        ember_hash_map* map = AS_HASH_MAP(value);
        printf("{");
        int first = 1;
        for (int j = 0; j < map->capacity; j++) {
            if (map->entries[j].is_occupied) {
                if (!first) printf(", ");
                first = 0;
                print_value(map->entries[j].key);
                printf(": ");
                print_value(map->entries[j].value);
            }
        }
        printf("}");
    } else {
        printf("<unknown>");
    }
}

// Native print function
ember_value ember_native_print(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" "); // Space separator between arguments
        print_value(argv[i]);
    }
    printf("\n");
    return ember_make_nil();
}

// Math functions
ember_value ember_native_abs(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(fabs(argv[0].as.number_val));
}

ember_value ember_native_sqrt(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    double num = argv[0].as.number_val;
    if (num < 0) {
        return ember_make_nil(); // Invalid input
    }
    return ember_make_number(sqrt(num));
}

ember_value ember_native_floor(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(floor(argv[0].as.number_val));
}

ember_value ember_native_ceil(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(ceil(argv[0].as.number_val));
}

ember_value ember_native_round(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(round(argv[0].as.number_val));
}

ember_value ember_native_pow(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 2 || argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(pow(argv[0].as.number_val, argv[1].as.number_val));
}

ember_value ember_native_max(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc < 1) {
        return ember_make_nil();
    }
    
    double max_val = -INFINITY;
    for (int i = 0; i < argc; i++) {
        if (argv[i].type != EMBER_VAL_NUMBER) {
            return ember_make_nil();
        }
        if (argv[i].as.number_val > max_val) {
            max_val = argv[i].as.number_val;
        }
    }
    return ember_make_number(max_val);
}

ember_value ember_native_min(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc < 1) {
        return ember_make_nil();
    }
    
    double min_val = INFINITY;
    for (int i = 0; i < argc; i++) {
        if (argv[i].type != EMBER_VAL_NUMBER) {
            return ember_make_nil();
        }
        if (argv[i].as.number_val < min_val) {
            min_val = argv[i].as.number_val;
        }
    }
    return ember_make_number(min_val);
}

// Type checking functions
ember_value ember_native_type(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_string("unknown");
    }
    
    switch (argv[0].type) {
        case EMBER_VAL_NIL: return ember_make_string("nil");
        case EMBER_VAL_BOOL: return ember_make_string("bool");
        case EMBER_VAL_NUMBER: return ember_make_string("number");
        case EMBER_VAL_STRING: return ember_make_string("string");
        case EMBER_VAL_FUNCTION: return ember_make_string("function");
        case EMBER_VAL_NATIVE: return ember_make_string("native");
        case EMBER_VAL_ARRAY: return ember_make_string("array");
        case EMBER_VAL_HASH_MAP: return ember_make_string("hash_map");
        case EMBER_VAL_EXCEPTION: return ember_make_string("exception");
        case EMBER_VAL_CLASS: return ember_make_string("class");
        case EMBER_VAL_INSTANCE: return ember_make_string("instance");
        default: return ember_make_string("unknown");
    }
}

// Array/Map length function
ember_value ember_native_len(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    if (argv[0].type == EMBER_VAL_ARRAY) {
        ember_array* array = AS_ARRAY(argv[0]);
        return ember_make_number(array->length);
    } else if (argv[0].type == EMBER_VAL_HASH_MAP) {
        ember_hash_map* map = AS_HASH_MAP(argv[0]);
        return ember_make_number(map->length);
    } else if (argv[0].type == EMBER_VAL_STRING) {
        if (IS_STRING(argv[0])) {
            return ember_make_number(AS_STRING(argv[0])->length);
        } else if (argv[0].as.string_val) {
            return ember_make_number(strlen(argv[0].as.string_val));
        }
    }
    
    return ember_make_nil();
}

// Logic functions
ember_value ember_native_not(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    // Falsy values: nil, false, 0
    if (argv[0].type == EMBER_VAL_NIL) {
        return ember_make_bool(1);
    } else if (argv[0].type == EMBER_VAL_BOOL) {
        return ember_make_bool(!argv[0].as.bool_val);
    } else if (argv[0].type == EMBER_VAL_NUMBER) {
        return ember_make_bool(argv[0].as.number_val == 0.0);
    } else {
        return ember_make_bool(0); // All other values are truthy
    }
}

// Type conversion functions
ember_value ember_native_str(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    char buffer[256];
    switch (argv[0].type) {
        case EMBER_VAL_NIL:
            return ember_make_string("nil");
        case EMBER_VAL_BOOL:
            return ember_make_string(argv[0].as.bool_val ? "true" : "false");
        case EMBER_VAL_NUMBER:
            snprintf(buffer, sizeof(buffer), "%g", argv[0].as.number_val);
            return ember_make_string(buffer);
        case EMBER_VAL_STRING:
            return argv[0]; // Already a string
        default:
            return ember_make_string("<object>");
    }
}

ember_value ember_native_num(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    if (argv[0].type == EMBER_VAL_NUMBER) {
        return argv[0]; // Already a number
    } else if (argv[0].type == EMBER_VAL_STRING) {
        const char* str = NULL;
        if (IS_STRING(argv[0])) {
            str = AS_CSTRING(argv[0]);
        } else {
            str = argv[0].as.string_val;
        }
        
        if (str) {
            char* endptr;
            double num = strtod(str, &endptr);
            if (*endptr == '\0') { // Successful conversion
                return ember_make_number(num);
            }
        }
    } else if (argv[0].type == EMBER_VAL_BOOL) {
        return ember_make_number(argv[0].as.bool_val ? 1.0 : 0.0);
    }
    
    return ember_make_nil();
}

ember_value ember_native_int(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    ember_value num_val = ember_native_num(vm, argc, argv);
    if (num_val.type == EMBER_VAL_NUMBER) {
        return ember_make_number(floor(num_val.as.number_val));
    }
    return ember_make_nil();
}

ember_value ember_native_bool(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1) {
        return ember_make_nil();
    }
    
    // Truthy check
    if (argv[0].type == EMBER_VAL_NIL) {
        return ember_make_bool(0);
    } else if (argv[0].type == EMBER_VAL_BOOL) {
        return argv[0]; // Already a bool
    } else if (argv[0].type == EMBER_VAL_NUMBER) {
        return ember_make_bool(argv[0].as.number_val != 0.0);
    } else {
        return ember_make_bool(1); // All other values are truthy
    }
}

// Register core built-in functions
void ember_register_core_builtins(ember_vm* vm) {
    // Math functions
    ember_register_func(vm, "print", ember_native_print);
    ember_register_func(vm, "abs", ember_native_abs);
    ember_register_func(vm, "sqrt", ember_native_sqrt);
    ember_register_func(vm, "floor", ember_native_floor);
    ember_register_func(vm, "ceil", ember_native_ceil);
    ember_register_func(vm, "round", ember_native_round);
    ember_register_func(vm, "pow", ember_native_pow);
    ember_register_func(vm, "max", ember_native_max);
    ember_register_func(vm, "min", ember_native_min);
    
    // Type functions
    ember_register_func(vm, "type", ember_native_type);
    ember_register_func(vm, "len", ember_native_len);
    ember_register_func(vm, "not", ember_native_not);
    
    // Type conversion functions
    ember_register_func(vm, "str", ember_native_str);
    ember_register_func(vm, "num", ember_native_num);
    ember_register_func(vm, "int", ember_native_int);
    ember_register_func(vm, "bool", ember_native_bool);
}

// Initialize the platform with stdlib integration
void ember_platform_init_with_stdlib(ember_vm* vm) {
    // Register core built-ins first
    ember_register_core_builtins(vm);
    
    // Get stdlib interface if available
    const ember_stdlib_interface_t* stdlib = ember_get_stdlib_interface();
    if (stdlib && stdlib->init_all) {
        const ember_core_interface_t* core = ember_get_core_interface();
        if (core) {
            // Initialize stdlib with core interface
            int result = stdlib->init_all(vm, core);
            if (result == 0) {
                printf("Standard library integration successful\n");
            } else {
                printf("Warning: Standard library integration failed\n");
            }
        }
    } else {
        printf("Info: Running with core functions only (stdlib not available)\n");
    }
}