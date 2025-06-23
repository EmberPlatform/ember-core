#include "../../include/ember.h"
#include "../runtime/value/value.h"
#include "error.h"
#include <stdio.h>

// VM operation handlers for Set operations
vm_operation_result vm_handle_set_new(ember_vm* vm) {
    ember_value set_val = ember_make_set(vm);
    if (set_val.type == EMBER_VAL_NIL) {
        ember_error* error = ember_error_runtime(vm, "Failed to create new Set");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    vm->stack[vm->stack_top++] = set_val;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_set_add(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value element = vm->stack[--vm->stack_top];
    ember_value set_val = vm->stack[vm->stack_top - 1];
    
    if (set_val.type != EMBER_VAL_SET) {
        ember_error* error = ember_error_runtime(vm, "Expected Set value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_set* set = AS_SET(set_val);
    int success __attribute__((unused)) = set_add(set, element);
    
    // Push result (the set itself for chaining)
    vm->stack[vm->stack_top++] = set_val;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_set_has(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value element = vm->stack[--vm->stack_top];
    ember_value set_val = vm->stack[--vm->stack_top];
    
    if (set_val.type != EMBER_VAL_SET) {
        ember_error* error = ember_error_runtime(vm, "Expected Set value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_set* set = AS_SET(set_val);
    int has_element = set_has(set, element);
    
    ember_value result = ember_make_bool(has_element);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_set_delete(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value element = vm->stack[--vm->stack_top];
    ember_value set_val = vm->stack[vm->stack_top - 1];
    
    if (set_val.type != EMBER_VAL_SET) {
        ember_error* error = ember_error_runtime(vm, "Expected Set value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_set* set = AS_SET(set_val);
    int was_deleted = set_delete(set, element);
    
    ember_value result = ember_make_bool(was_deleted);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_set_size(ember_vm* vm) {
    if (vm->stack_top < 1) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value set_val = vm->stack[--vm->stack_top];
    
    if (set_val.type != EMBER_VAL_SET) {
        ember_error* error = ember_error_runtime(vm, "Expected Set value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_set* set = AS_SET(set_val);
    ember_value result = ember_make_number(set->size);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_set_clear(ember_vm* vm) {
    if (vm->stack_top < 1) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value set_val = vm->stack[vm->stack_top - 1];
    
    if (set_val.type != EMBER_VAL_SET) {
        ember_error* error = ember_error_runtime(vm, "Expected Set value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_set* set = AS_SET(set_val);
    set_clear(set);
    
    // Return the set for chaining
    vm->stack[vm->stack_top++] = set_val;
    return VM_RESULT_OK;
}

// VM operation handlers for Map operations
vm_operation_result vm_handle_map_new(ember_vm* vm) {
    ember_value map_val = ember_make_map(vm);
    if (map_val.type == EMBER_VAL_NIL) {
        ember_error* error = ember_error_runtime(vm, "Failed to create new Map");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    vm->stack[vm->stack_top++] = map_val;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_set(ember_vm* vm) {
    if (vm->stack_top < 3) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in map operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value value = vm->stack[--vm->stack_top];
    ember_value key = vm->stack[--vm->stack_top];
    ember_value map_val = vm->stack[vm->stack_top - 1];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    int success __attribute__((unused)) = map_set(map, key, value);
    
    // Push result (the map itself for chaining)
    vm->stack[vm->stack_top++] = map_val;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_get(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value key = vm->stack[--vm->stack_top];
    ember_value map_val = vm->stack[--vm->stack_top];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    ember_value result = map_get(map, key);
    
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_has(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value key = vm->stack[--vm->stack_top];
    ember_value map_val = vm->stack[--vm->stack_top];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    int has_key = map_has(map, key);
    
    ember_value result = ember_make_bool(has_key);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_delete(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value key = vm->stack[--vm->stack_top];
    ember_value map_val = vm->stack[vm->stack_top - 1];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    int was_deleted = map_delete(map, key);
    
    ember_value result = ember_make_bool(was_deleted);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_size(ember_vm* vm) {
    if (vm->stack_top < 1) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value map_val = vm->stack[--vm->stack_top];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    ember_value result = ember_make_number(map->size);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_map_clear(ember_vm* vm) {
    if (vm->stack_top < 1) {
        ember_error* error = ember_error_runtime(vm, "Stack underflow in set operation");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value map_val = vm->stack[vm->stack_top - 1];
    
    if (map_val.type != EMBER_VAL_MAP) {
        ember_error* error = ember_error_runtime(vm, "Expected Map value");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_map* map = AS_MAP(map_val);
    map_clear(map);
    
    // Return the map for chaining
    vm->stack[vm->stack_top++] = map_val;
    return VM_RESULT_OK;
}

// VM operation handlers for Regex operations
