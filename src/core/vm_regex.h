#ifndef VM_REGEX_H
#define VM_REGEX_H

#include "ember.h"

// Forward declarations
typedef struct ember_vm ember_vm;

// Regex creation and manipulation functions
ember_value ember_make_regex(ember_vm* vm, const char* pattern, ember_regex_flags flags);
bool ember_regex_test(ember_vm* vm, ember_regex* regex, const char* text);
ember_value ember_regex_match_function(ember_vm* vm, ember_regex* regex, const char* text);
ember_value ember_regex_replace(ember_vm* vm, ember_regex* regex, const char* text, const char* replacement);
ember_value ember_regex_split(ember_vm* vm, ember_regex* regex, const char* text);

// VM operation handlers for regex opcodes
vm_operation_result vm_handle_regex_new(ember_vm* vm);
vm_operation_result vm_handle_regex_test(ember_vm* vm);
vm_operation_result vm_handle_regex_match(ember_vm* vm);
vm_operation_result vm_handle_regex_replace(ember_vm* vm);
vm_operation_result vm_handle_regex_split(ember_vm* vm);

#endif // VM_REGEX_H