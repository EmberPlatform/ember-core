/**
 * Enhanced Export Parser for Ember
 * Supports ES6-style export statements
 */

#define _GNU_SOURCE

#include "parser.h"
#include "../../runtime/module_system.h"
#include "../../vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Export statement types supported:
// export const name = value;           // Named export
// export function name() {}            // Function export
// export default value;                // Default export
// export { name1, name2 };            // Named export list

// Generate bytecode for export call
static void emit_export_call(ember_chunk* chunk, const char* export_name, int value_const_idx) {
    // Push export name
    ember_value name_val = ember_make_string(export_name);
    int name_const = add_constant(chunk, name_val);
    emit_bytes(chunk, OP_PUSH_CONST, name_const);
    
    // Push value
    emit_bytes(chunk, OP_PUSH_CONST, value_const_idx);
    
    // Call export function
    // For now, just store as global variable with export prefix
    emit_byte(chunk, OP_SET_GLOBAL);
    emit_byte(chunk, OP_POP); // Pop the stored value
}

// Parse export declaration
void export_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    if (match(TOKEN_DEFAULT)) {
        // export default ...
        if (match(TOKEN_FUNCTION) || match(TOKEN_FN)) {
            // export default function
            if (check(TOKEN_IDENTIFIER)) {
                // Named function - export the name as default
                advance_parser();
                ember_token name_token = parser->previous;
                
                // Parse function definition
                function_definition(vm, chunk);
                
                // Export the function as default
                char* name = malloc(name_token.length + 1);
                memcpy(name, name_token.start, name_token.length);
                name[name_token.length] = '\0';
                
                ember_value name_val = ember_make_string(name);
                int name_const = add_constant(chunk, name_val);
                emit_export_call(chunk, "default", name_const);
                
                free(name);
            } else {
                // Anonymous function - evaluate and export
                function_definition(vm, chunk);
                
                // The function is now on top of stack, store temporarily
                ember_value temp_val = ember_make_nil();
                int temp_const = add_constant(chunk, temp_val);
                emit_export_call(chunk, "default", temp_const);
            }
        } else {
            // export default expression
            expression(chunk);
            
            // The expression result is on stack, export it
            ember_value temp_val = ember_make_nil();
            int temp_const = add_constant(chunk, temp_val);
            emit_export_call(chunk, "default", temp_const);
        }
        
    } else if (match(TOKEN_LBRACE)) {
        // export { name1, name2, ... }
        do {
            consume(TOKEN_IDENTIFIER, "Expected export name");
            ember_token export_token = parser->previous;
            
            char* export_name = malloc(export_token.length + 1);
            memcpy(export_name, export_token.start, export_token.length);
            export_name[export_token.length] = '\0';
            
            char* alias_name = export_name;
            
            if (match(TOKEN_AS)) {
                // export { name as alias }
                consume(TOKEN_IDENTIFIER, "Expected alias name");
                ember_token alias_token = parser->previous;
                alias_name = malloc(alias_token.length + 1);
                memcpy(alias_name, alias_token.start, alias_token.length);
                alias_name[alias_token.length] = '\0';
            }
            
            // Get the value from global scope
            ember_value var_name = ember_make_string(export_name);
            int var_const = add_constant(chunk, var_name);
            emit_bytes(chunk, OP_PUSH_CONST, var_const);
            emit_byte(chunk, OP_GET_GLOBAL);
            
            // Export it with the alias name
            emit_export_call(chunk, alias_name, var_const);
            
            free(export_name);
            if (alias_name != export_name) {
                free(alias_name);
            }
            
        } while (match(TOKEN_COMMA));
        
        consume(TOKEN_RBRACE, "Expected '}' after export list");
        
        // Check for 'from' clause
        if (match(TOKEN_FROM)) {
            consume(TOKEN_STRING, "Expected module name");
            ember_token module_token = parser->previous;
            
            char* module_name = malloc(module_token.length - 1);
            memcpy(module_name, module_token.start + 1, module_token.length - 2);
            module_name[module_token.length - 2] = '\0';
            
            printf("Note: Re-export from \"%s\" - implementation pending\n", module_name);
            
            free(module_name);
        }
        
    } else if (match(TOKEN_MULTIPLY)) {
        // export * from "module"
        consume(TOKEN_FROM, "Expected 'from' after export *");
        consume(TOKEN_STRING, "Expected module name");
        ember_token module_token = parser->previous;
        
        char* module_name = malloc(module_token.length - 1);
        memcpy(module_name, module_token.start + 1, module_token.length - 2);
        module_name[module_token.length - 2] = '\0';
        
        printf("Note: export * from \"%s\" - implementation pending\n", module_name);
        
        free(module_name);
        
    } else if (check(TOKEN_IDENTIFIER)) {
        // Check for variable declaration keywords
        advance_parser();
        ember_token keyword_token = parser->previous;
        
        if ((keyword_token.length == 3 && memcmp(keyword_token.start, "var", 3) == 0) ||
            (keyword_token.length == 5 && memcmp(keyword_token.start, "const", 5) == 0) ||
            (keyword_token.length == 3 && memcmp(keyword_token.start, "let", 3) == 0)) {
            
            // export var/const/let name = value
            consume(TOKEN_IDENTIFIER, "Expected variable name");
            ember_token var_token = parser->previous;
            
            char* var_name = malloc(var_token.length + 1);
            memcpy(var_name, var_token.start, var_token.length);
            var_name[var_token.length] = '\0';
            
            consume(TOKEN_EQUAL, "Expected '=' after variable name");
            expression(chunk);
            
            // Set as global variable
            ember_value name_val = ember_make_string(var_name);
            int name_const = add_constant(chunk, name_val);
            emit_bytes(chunk, OP_PUSH_CONST, name_const);
            emit_byte(chunk, OP_SET_GLOBAL);
            
            // Also export it
            emit_export_call(chunk, var_name, name_const);
            
            free(var_name);
        } else {
            error_at(&parser->previous, "Unexpected token in export statement");
        }
        
    } else if (match(TOKEN_FUNCTION) || match(TOKEN_FN)) {
        // export function name() {}
        consume(TOKEN_IDENTIFIER, "Expected function name");
        ember_token name_token = parser->previous;
        
        char* func_name = malloc(name_token.length + 1);
        memcpy(func_name, name_token.start, name_token.length);
        func_name[name_token.length] = '\0';
        
        // Parse function definition
        function_definition(vm, chunk);
        
        // Export the function
        ember_value name_val = ember_make_string(func_name);
        int name_const = add_constant(chunk, name_val);
        emit_export_call(chunk, func_name, name_const);
        
        free(func_name);
        
    } else {
        error_at_current("Expected export declaration");
    }
}

// Enhanced export parsing with better error handling
void enhanced_export_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // For now, allow exports everywhere (in future, check module context)
    export_statement(vm, chunk);
}