#include "vm_regex.h"
#include "ember.h"
#include "../vm.h"
#include "../runtime/value/value.h"
#include "error.h"
#include <stdlib.h>
#include <string.h>
#include <regex.h>

// Create a new regex object
ember_value ember_make_regex(ember_vm* vm, const char* pattern, ember_regex_flags flags) {
    ember_regex* regex = (ember_regex*)malloc(sizeof(ember_regex));
    if (!regex) {
        ember_error* error = ember_error_memory("Failed to allocate regex object");
        ember_vm_set_error(vm, error);
        return ember_make_nil();
    }
    
    regex->obj.type = OBJ_REGEX;
    regex->obj.is_marked = 0;
    regex->obj.next = vm->objects;
    vm->objects = (ember_object*)regex;
    
    // Copy pattern
    regex->pattern = (char*)malloc(strlen(pattern) + 1);
    if (!regex->pattern) {
        free(regex);
        ember_error* error = ember_error_memory("Failed to allocate regex pattern");
        ember_vm_set_error(vm, error);
        return ember_make_nil();
    }
    strcpy(regex->pattern, pattern);
    
    regex->flags = flags;
    regex->last_index = 0;
    
    // Compile the regex using POSIX regex
    regex_t* compiled = (regex_t*)malloc(sizeof(regex_t));
    if (!compiled) {
        free(regex->pattern);
        free(regex);
        ember_error* error = ember_error_memory("Failed to allocate compiled regex");
        ember_vm_set_error(vm, error);
        return ember_make_nil();
    }
    
    int cflags = REG_EXTENDED;
    if (flags & REGEX_CASE_INSENSITIVE) {
        cflags |= REG_ICASE;
    }
    if (flags & REGEX_MULTILINE) {
        cflags |= REG_NEWLINE;
    }
    
    int result = regcomp(compiled, pattern, cflags);
    if (result != 0) {
        char error_buffer[256];
        regerror(result, compiled, error_buffer, sizeof(error_buffer));
        
        free(regex->pattern);
        free(compiled);
        free(regex);
        
        ember_error* error = ember_error_runtime(vm, "Invalid regex pattern");
        ember_vm_set_error(vm, error);
        return ember_make_nil();
    }
    
    regex->compiled_regex = compiled;
    regex->groups = (ember_array*)ember_make_array(vm, 10).as.obj_val;
    
    ember_value regex_val;
    regex_val.type = EMBER_VAL_REGEX;
    regex_val.as.obj_val = (ember_object*)regex;
    return regex_val;
}

// Test if a string matches the regex
bool ember_regex_test(ember_vm* vm, ember_regex* regex, const char* text) {
    if (!regex || !regex->compiled_regex || !text) {
        return false;
    }
    
    regex_t* compiled = (regex_t*)regex->compiled_regex;
    regmatch_t match;
    
    int result = regexec(compiled, text, 1, &match, 0);
    return result == 0;
}

// Get match details from regex
ember_value ember_regex_match_function(ember_vm* vm, ember_regex* regex, const char* text) {
    if (!regex || !regex->compiled_regex || !text) {
        return ember_make_nil();
    }
    
    regex_t* compiled = (regex_t*)regex->compiled_regex;
    regmatch_t matches[10]; // Support up to 10 capture groups
    
    int result = regexec(compiled, text, 10, matches, 0);
    if (result != 0) {
        return ember_make_nil(); // No match
    }
    
    // Clear previous groups
    if (regex->groups) {
        regex->groups->length = 0;
    }
    
    // Create match result object
    ember_value match_obj = ember_make_hash_map(vm, 4);
    ember_hash_map* map = AS_HASH_MAP(match_obj);
    
    // Add match information
    ember_value match_key = ember_make_string_gc(vm, "match");
    int match_len = matches[0].rm_eo - matches[0].rm_so;
    char* match_str = (char*)malloc(match_len + 1);
    if (match_str) {
        strncpy(match_str, text + matches[0].rm_so, match_len);
        match_str[match_len] = '\0';
        ember_value match_val = ember_make_string_gc(vm, match_str);
        hash_map_set_with_vm(vm, map, match_key, match_val);
        free(match_str);
    }
    
    ember_value index_key = ember_make_string_gc(vm, "index");
    ember_value index_val = ember_make_number(matches[0].rm_so);
    hash_map_set_with_vm(vm, map, index_key, index_val);
    
    ember_value length_key = ember_make_string_gc(vm, "length");
    ember_value length_val = ember_make_number(match_len);
    hash_map_set_with_vm(vm, map, length_key, length_val);
    
    // Add capture groups
    ember_value groups_array = ember_make_array(vm, 10);
    ember_array* groups = AS_ARRAY(groups_array);
    
    for (int i = 1; i < 10 && matches[i].rm_so != -1; i++) {
        int group_len = matches[i].rm_eo - matches[i].rm_so;
        char* group_str = (char*)malloc(group_len + 1);
        if (group_str) {
            strncpy(group_str, text + matches[i].rm_so, group_len);
            group_str[group_len] = '\0';
            ember_value group_val = ember_make_string_gc(vm, group_str);
            array_push(groups, group_val);
            free(group_str);
        }
    }
    
    ember_value groups_key = ember_make_string_gc(vm, "groups");
    hash_map_set_with_vm(vm, map, groups_key, groups_array);
    
    return match_obj;
}

// Replace matches in string with replacement
ember_value ember_regex_replace(ember_vm* vm, ember_regex* regex, const char* text, const char* replacement) {
    if (!regex || !regex->compiled_regex || !text || !replacement) {
        return ember_make_string_gc(vm, text ? text : "");
    }
    
    regex_t* compiled = (regex_t*)regex->compiled_regex;
    
    // Simple implementation - find first match and replace
    regmatch_t match;
    int result = regexec(compiled, text, 1, &match, 0);
    
    if (result != 0) {
        // No match, return original string
        return ember_make_string_gc(vm, text);
    }
    
    // Calculate result string length
    int text_len = strlen(text);
    int replacement_len = strlen(replacement);
    int match_len = match.rm_eo - match.rm_so;
    int result_len = text_len - match_len + replacement_len;
    
    char* result_str = (char*)malloc(result_len + 1);
    if (!result_str) {
        ember_error* error = ember_error_memory("Failed to allocate replacement string");
        ember_vm_set_error(vm, error);
        return ember_make_nil();
    }
    
    // Build result string: before_match + replacement + after_match
    strncpy(result_str, text, match.rm_so);
    strcpy(result_str + match.rm_so, replacement);
    strcpy(result_str + match.rm_so + replacement_len, text + match.rm_eo);
    
    ember_value result_val = ember_make_string_gc(vm, result_str);
    free(result_str);
    
    return result_val;
}

// Split string by regex
ember_value ember_regex_split(ember_vm* vm, ember_regex* regex, const char* text) {
    if (!regex || !regex->compiled_regex || !text) {
        ember_value result = ember_make_array(vm, 1);
        ember_array* array = AS_ARRAY(result);
        array_push(array, ember_make_string_gc(vm, text ? text : ""));
        return result;
    }
    
    regex_t* compiled = (regex_t*)regex->compiled_regex;
    ember_value result = ember_make_array(vm, 10);
    ember_array* array = AS_ARRAY(result);
    
    const char* current = text;
    regmatch_t match;
    
    while (regexec(compiled, current, 1, &match, 0) == 0) {
        // Add text before match
        if (match.rm_so > 0) {
            char* part = (char*)malloc(match.rm_so + 1);
            if (part) {
                strncpy(part, current, match.rm_so);
                part[match.rm_so] = '\0';
                array_push(array, ember_make_string_gc(vm, part));
                free(part);
            }
        }
        
        // Move past the match
        current += match.rm_eo;
        
        // Prevent infinite loop on zero-length matches
        if (match.rm_so == match.rm_eo) {
            if (*current != '\0') {
                current++;
            } else {
                break;
            }
        }
    }
    
    // Add remaining text
    if (*current != '\0') {
        array_push(array, ember_make_string_gc(vm, current));
    }
    
    return result;
}

// VM operation handlers
vm_operation_result vm_handle_regex_new(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "OP_REGEX_NEW: Not enough arguments on stack");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value flags_val = vm->stack[--vm->stack_top];
    ember_value pattern_val = vm->stack[--vm->stack_top];
    
    if (!IS_STRING(pattern_val)) {
        ember_error* error = ember_error_runtime(vm, "Regex pattern must be a string");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_regex_flags flags = REGEX_NONE;
    if (IS_NUMBER(flags_val)) {
        flags = (ember_regex_flags)(int)AS_NUMBER(flags_val);
    }
    
    ember_value regex = ember_make_regex(vm, AS_STRING(pattern_val)->chars, flags);
    vm->stack[vm->stack_top++] = regex;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_regex_test(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "OP_REGEX_TEST: Not enough arguments on stack");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value text_val = vm->stack[--vm->stack_top];
    ember_value regex_val = vm->stack[--vm->stack_top];
    
    if (!IS_REGEX(regex_val) || !IS_STRING(text_val)) {
        ember_error* error = ember_error_runtime(vm, "Invalid arguments for regex test");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    bool matches = ember_regex_test(vm, AS_REGEX(regex_val), AS_STRING(text_val)->chars);
    vm->stack[vm->stack_top++] = ember_make_bool(matches);
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_regex_match(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "OP_REGEX_MATCH: Not enough arguments on stack");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value text_val = vm->stack[--vm->stack_top];
    ember_value regex_val = vm->stack[--vm->stack_top];
    
    if (!IS_REGEX(regex_val) || !IS_STRING(text_val)) {
        ember_error* error = ember_error_runtime(vm, "Invalid arguments for regex match");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value match_result = ember_regex_match_function(vm, AS_REGEX(regex_val), AS_STRING(text_val)->chars);
    vm->stack[vm->stack_top++] = match_result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_regex_replace(ember_vm* vm) {
    if (vm->stack_top < 3) {
        ember_error* error = ember_error_runtime(vm, "OP_REGEX_REPLACE: Not enough arguments on stack");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value replacement_val = vm->stack[--vm->stack_top];
    ember_value text_val = vm->stack[--vm->stack_top];
    ember_value regex_val = vm->stack[--vm->stack_top];
    
    if (!IS_REGEX(regex_val) || !IS_STRING(text_val) || !IS_STRING(replacement_val)) {
        ember_error* error = ember_error_runtime(vm, "Invalid arguments for regex replace");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value result = ember_regex_replace(vm, AS_REGEX(regex_val), 
                                           AS_STRING(text_val)->chars, 
                                           AS_STRING(replacement_val)->chars);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}

vm_operation_result vm_handle_regex_split(ember_vm* vm) {
    if (vm->stack_top < 2) {
        ember_error* error = ember_error_runtime(vm, "OP_REGEX_SPLIT: Not enough arguments on stack");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value text_val = vm->stack[--vm->stack_top];
    ember_value regex_val = vm->stack[--vm->stack_top];
    
    if (!IS_REGEX(regex_val) || !IS_STRING(text_val)) {
        ember_error* error = ember_error_runtime(vm, "Invalid arguments for regex split");
        ember_vm_set_error(vm, error);
        return VM_RESULT_ERROR;
    }
    
    ember_value result = ember_regex_split(vm, AS_REGEX(regex_val), AS_STRING(text_val)->chars);
    vm->stack[vm->stack_top++] = result;
    return VM_RESULT_OK;
}