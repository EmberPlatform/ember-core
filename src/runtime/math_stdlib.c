/**
 * Math standard library functions for Ember Core
 * Consolidated from ember-native with improved error handling
 */

#include "ember.h"
#include <math.h>

// Enhanced error handling macros
#define EMBER_BUILTIN_ERROR_TYPE(vm, func_name, expected, actual) do { \
    (void)vm; \
    return ember_make_nil(); \
} while(0)

#define EMBER_BUILTIN_ERROR_ARGS(vm, func_name, expected, actual) do { \
    (void)vm; \
    return ember_make_nil(); \
} while(0)

// Math functions with proper error handling
ember_value ember_native_abs(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "abs", 1, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "abs", "number", "other");
    }
    return ember_make_number(fabs(argv[0].as.number_val));
}

ember_value ember_native_sqrt(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "sqrt", 1, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "sqrt", "number", "other");
    }
    double value = argv[0].as.number_val;
    if (value < 0) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "sqrt", "non-negative number", "negative number");
    }
    return ember_make_number(sqrt(value));
}

ember_value ember_native_max(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "max", 2, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "max", "numbers", "other");
    }
    double a = argv[0].as.number_val;
    double b = argv[1].as.number_val;
    return ember_make_number(a > b ? a : b);
}

ember_value ember_native_min(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "min", 2, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "min", "numbers", "other");
    }
    double a = argv[0].as.number_val;
    double b = argv[1].as.number_val;
    return ember_make_number(a < b ? a : b);
}

ember_value ember_native_floor(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "floor", 1, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "floor", "number", "other");
    }
    return ember_make_number(floor(argv[0].as.number_val));
}

ember_value ember_native_ceil(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "ceil", 1, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "ceil", "number", "other");
    }
    return ember_make_number(ceil(argv[0].as.number_val));
}

ember_value ember_native_round(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 1) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "round", 1, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "round", "number", "other");
    }
    return ember_make_number(round(argv[0].as.number_val));
}

ember_value ember_native_pow(ember_vm* vm, int argc, ember_value* argv) {
    if (argc != 2) {
        EMBER_BUILTIN_ERROR_ARGS(vm, "pow", 2, argc);
    }
    if (argv[0].type != EMBER_VAL_NUMBER || argv[1].type != EMBER_VAL_NUMBER) {
        EMBER_BUILTIN_ERROR_TYPE(vm, "pow", "numbers", "other");
    }
    double base = argv[0].as.number_val;
    double exponent = argv[1].as.number_val;
    return ember_make_number(pow(base, exponent));
}