/**
 * Enhanced Import Parser for Ember
 * Supports multiple import styles and patterns
 */

#define _GNU_SOURCE

#include "parser.h"
#include "../../runtime/module_system.h"
#include "../../vm.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// Import styles supported:
// import "module"                          // Load entire module
// import { func1, func2 } from "module"   // Named imports
// import * as module from "module"        // Namespace import
// import default from "module"            // Default import
// import default, { named } from "module" // Mixed import
// const module = require("module")         // CommonJS style

// Parse import specifiers for ES6-style imports
typedef struct {
    char* name;
    char* alias;
} import_specifier;

typedef struct {
    import_specifier* specifiers;
    int count;
    int capacity;
} import_specifier_list;

static import_specifier_list* create_specifier_list(void) {
    import_specifier_list* list = malloc(sizeof(import_specifier_list));
    list->specifiers = malloc(sizeof(import_specifier) * 8);
    list->count = 0;
    list->capacity = 8;
    return list;
}

static void add_specifier(import_specifier_list* list, const char* name, const char* alias) {
    if (list->count >= list->capacity) {
        list->capacity *= 2;
        list->specifiers = realloc(list->specifiers, sizeof(import_specifier) * list->capacity);
    }
    
    list->specifiers[list->count].name = strdup(name);
    list->specifiers[list->count].alias = alias ? strdup(alias) : strdup(name);
    list->count++;
}

static void free_specifier_list(import_specifier_list* list) {
    for (int i = 0; i < list->count; i++) {
        free(list->specifiers[i].name);
        free(list->specifiers[i].alias);
    }
    free(list->specifiers);
    free(list);
}

// Generate bytecode to load module and extract named exports
static void emit_named_import(ember_chunk* chunk, const char* module_name, import_specifier_list* specifiers) {
    // Load the module
    ember_value module_name_val = ember_make_string(module_name);
    int module_const = add_constant(chunk, module_name_val);
    emit_bytes(chunk, OP_PUSH_CONST, module_const);
    
    // Call import function
    emit_bytes(chunk, OP_CALL, 1); // 1 argument (module name)
    
    // Extract each named export
    for (int i = 0; i < specifiers->count; i++) {
        // Duplicate the module object on stack
        if (i > 0) {
            emit_byte(chunk, OP_GET_LOCAL); // Get module from local variable
            emit_byte(chunk, 0); // Local slot for module
        }
        
        // Get the named property
        ember_value prop_name = ember_make_string(specifiers->specifiers[i].name);
        int prop_const = add_constant(chunk, prop_name);
        emit_bytes(chunk, OP_PUSH_CONST, prop_const);
        emit_byte(chunk, OP_HASH_MAP_GET);
        
        // Set as global variable with alias name
        ember_value alias_name = ember_make_string(specifiers->specifiers[i].alias);
        int alias_const = add_constant(chunk, alias_name);
        emit_bytes(chunk, OP_PUSH_CONST, alias_const);
        emit_byte(chunk, OP_SET_GLOBAL);
        emit_byte(chunk, OP_POP); // Pop the assigned value
    }
    
    // Store module in local for subsequent extractions
    if (specifiers->count > 1) {
        emit_byte(chunk, OP_SET_LOCAL);
        emit_byte(chunk, 0); // Local slot 0
    } else {
        emit_byte(chunk, OP_POP); // Pop module if only one import
    }
}

// Generate bytecode for namespace import (import * as name from "module")
static void emit_namespace_import(ember_chunk* chunk, const char* module_name, const char* namespace_name) {
    // Load the module
    ember_value module_name_val = ember_make_string(module_name);
    int module_const = add_constant(chunk, module_name_val);
    emit_bytes(chunk, OP_PUSH_CONST, module_const);
    
    // Call import function
    emit_bytes(chunk, OP_CALL, 1);
    
    // Set as global variable with namespace name
    ember_value namespace_val = ember_make_string(namespace_name);
    int namespace_const = add_constant(chunk, namespace_val);
    emit_bytes(chunk, OP_PUSH_CONST, namespace_const);
    emit_byte(chunk, OP_SET_GLOBAL);
    emit_byte(chunk, OP_POP);
}

// Generate bytecode for default import (import name from "module")
static void emit_default_import(ember_chunk* chunk, const char* module_name, const char* default_name) {
    // Load the module
    ember_value module_name_val = ember_make_string(module_name);
    int module_const = add_constant(chunk, module_name_val);
    emit_bytes(chunk, OP_PUSH_CONST, module_const);
    
    // Call import function
    emit_bytes(chunk, OP_CALL, 1);
    
    // Try to get 'default' export, or use entire module
    ember_value default_key = ember_make_string("default");
    int default_const = add_constant(chunk, default_key);
    emit_bytes(chunk, OP_PUSH_CONST, default_const);
    emit_byte(chunk, OP_HASH_MAP_GET);
    
    // Check if default export exists, otherwise use module itself
    // TODO: Add conditional logic for this
    
    // Set as global variable
    ember_value name_val = ember_make_string(default_name);
    int name_const = add_constant(chunk, name_val);
    emit_bytes(chunk, OP_PUSH_CONST, name_const);
    emit_byte(chunk, OP_SET_GLOBAL);
    emit_byte(chunk, OP_POP);
}

// Enhanced import statement parser
void enhanced_import_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Check for different import patterns
    if (check(TOKEN_LBRACE)) {
        // Named imports: import { name1, name2 } from "module"
        advance_parser(); // consume '{'
        
        import_specifier_list* specifiers = create_specifier_list();
        
        do {
            consume(TOKEN_IDENTIFIER, "Expected import name");
            ember_token name_token = parser->previous;
            
            char* name = malloc(name_token.length + 1);
            memcpy(name, name_token.start, name_token.length);
            name[name_token.length] = '\0';
            
            char* alias = NULL;
            if (match(TOKEN_IDENTIFIER) && parser->previous.length == 2 && 
                memcmp(parser->previous.start, "as", 2) == 0) {
                // import { name as alias }
                consume(TOKEN_IDENTIFIER, "Expected alias name");
                ember_token alias_token = parser->previous;
                alias = malloc(alias_token.length + 1);
                memcpy(alias, alias_token.start, alias_token.length);
                alias[alias_token.length] = '\0';
            }
            
            add_specifier(specifiers, name, alias);
            free(name);
            if (alias) free(alias);
            
        } while (match(TOKEN_COMMA));
        
        consume(TOKEN_RBRACE, "Expected '}' after import list");
        
        // Expect 'from'
        consume(TOKEN_IDENTIFIER, "Expected 'from'");
        if (parser->previous.length != 4 || memcmp(parser->previous.start, "from", 4) != 0) {
            error_at(&parser->previous, "Expected 'from' after import list");
            free_specifier_list(specifiers);
            return;
        }
        
        consume(TOKEN_STRING, "Expected module name");
        ember_token module_token = parser->previous;
        char* module_name = malloc(module_token.length - 1); // -2 for quotes +1 for null
        memcpy(module_name, module_token.start + 1, module_token.length - 2);
        module_name[module_token.length - 2] = '\0';
        
        emit_named_import(chunk, module_name, specifiers);
        
        free(module_name);
        free_specifier_list(specifiers);
        
    } else if (check(TOKEN_MULTIPLY)) {
        // Namespace import: import * as name from "module"
        advance_parser(); // consume '*'
        
        consume(TOKEN_IDENTIFIER, "Expected 'as'");
        if (parser->previous.length != 2 || memcmp(parser->previous.start, "as", 2) != 0) {
            error_at(&parser->previous, "Expected 'as' after '*'");
            return;
        }
        
        consume(TOKEN_IDENTIFIER, "Expected namespace name");
        ember_token namespace_token = parser->previous;
        char* namespace_name = malloc(namespace_token.length + 1);
        memcpy(namespace_name, namespace_token.start, namespace_token.length);
        namespace_name[namespace_token.length] = '\0';
        
        consume(TOKEN_IDENTIFIER, "Expected 'from'");
        if (parser->previous.length != 4 || memcmp(parser->previous.start, "from", 4) != 0) {
            error_at(&parser->previous, "Expected 'from' after namespace import");
            free(namespace_name);
            return;
        }
        
        consume(TOKEN_STRING, "Expected module name");
        ember_token module_token = parser->previous;
        char* module_name = malloc(module_token.length - 1);
        memcpy(module_name, module_token.start + 1, module_token.length - 2);
        module_name[module_token.length - 2] = '\0';
        
        emit_namespace_import(chunk, module_name, namespace_name);
        
        free(module_name);
        free(namespace_name);
        
    } else if (check(TOKEN_STRING)) {
        // Simple import: import "module"
        advance_parser();
        ember_token module_token = parser->previous;
        char* module_name = malloc(module_token.length - 1);
        memcpy(module_name, module_token.start + 1, module_token.length - 2);
        module_name[module_token.length - 2] = '\0';
        
        // Just load the module (side effects only)
        ember_value module_name_val = ember_make_string(module_name);
        int module_const = add_constant(chunk, module_name_val);
        emit_bytes(chunk, OP_PUSH_CONST, module_const);
        emit_bytes(chunk, OP_CALL, 1);
        emit_byte(chunk, OP_POP); // Discard result
        
        free(module_name);
        
    } else if (check(TOKEN_IDENTIFIER)) {
        // Default import: import name from "module"
        advance_parser();
        ember_token default_token = parser->previous;
        char* default_name = malloc(default_token.length + 1);
        memcpy(default_name, default_token.start, default_token.length);
        default_name[default_token.length] = '\0';
        
        consume(TOKEN_IDENTIFIER, "Expected 'from'");
        if (parser->previous.length != 4 || memcmp(parser->previous.start, "from", 4) != 0) {
            error_at(&parser->previous, "Expected 'from' after default import");
            free(default_name);
            return;
        }
        
        consume(TOKEN_STRING, "Expected module name");
        ember_token module_token = parser->previous;
        char* module_name = malloc(module_token.length - 1);
        memcpy(module_name, module_token.start + 1, module_token.length - 2);
        module_name[module_token.length - 2] = '\0';
        
        emit_default_import(chunk, module_name, default_name);
        
        free(module_name);
        free(default_name);
        
    } else {
        error_at(&parser->current, "Expected import pattern");
    }
}