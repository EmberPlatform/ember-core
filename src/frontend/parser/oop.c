#include "parser.h"
#include "../../runtime/value/value.h"
#include <string.h>
#include <stdlib.h>

// Forward declaration to get parser state from main parser
extern parser_state* get_parser_state(void);
extern void advance_parser(void);
extern int match(ember_token_type type);
extern int check(ember_token_type type);
extern void consume(ember_token_type type, const char* message);
extern void expression(ember_chunk* chunk);
extern void emit_byte(ember_chunk* chunk, uint8_t byte);
extern void emit_bytes(ember_chunk* chunk, uint8_t byte1, uint8_t byte2);
extern int add_constant(ember_chunk* chunk, ember_value constant);
extern void init_chunk(ember_chunk* chunk);
extern void statement(ember_vm* vm, ember_chunk* chunk);
extern void error_at(ember_token* token, const char* message);
extern void track_function_chunk(ember_vm* vm, ember_chunk* chunk);

// Helper function to emit constant operation
static void emit_constant(ember_chunk* chunk, ember_value value) {
    int constant = add_constant(chunk, value);
    if (constant < 256) {
        emit_bytes(chunk, OP_PUSH_CONST, (uint8_t)constant);
    } else {
        // Handle large constant pools if needed
        emit_bytes(chunk, OP_PUSH_CONST, (uint8_t)constant);
    }
}

// Parse class declaration: class Name [extends Superclass] { ... }
void class_declaration(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Consume class name
    consume(TOKEN_IDENTIFIER, "Expected class name");
    ember_token class_name = parser->previous;
    
    // Create class name string
    char* name_str = malloc(class_name.length + 1);
    memcpy(name_str, class_name.start, class_name.length);
    name_str[class_name.length] = '\0';
    
    // Check for inheritance
    bool has_superclass = false;
    if (match(TOKEN_EXTENDS)) {
        has_superclass = true;
        consume(TOKEN_IDENTIFIER, "Expected superclass name");
        
        // Load superclass onto stack
        ember_token superclass_name = parser->previous;
        char* super_name_str = malloc(superclass_name.length + 1);
        memcpy(super_name_str, superclass_name.start, superclass_name.length);
        super_name_str[superclass_name.length] = '\0';
        
        // Emit code to get superclass from globals
        ember_value super_name_val = ember_make_string(super_name_str);
        int super_name_idx = add_constant(chunk, super_name_val);
        emit_bytes(chunk, OP_GET_GLOBAL, super_name_idx);
        
        free(super_name_str);
    }
    
    // Create the class
    ember_value class_name_val = ember_make_string(name_str);
    int class_name_idx = add_constant(chunk, class_name_val);
    
    if (has_superclass) {
        emit_bytes(chunk, OP_INHERIT, class_name_idx);  // Pop superclass, push class
    } else {
        emit_bytes(chunk, OP_CLASS_DEF, class_name_idx);  // Create new class
    }
    
    // Parse class body (class is on stack)
    consume(TOKEN_LBRACE, "Expected '{' before class body");
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_FN)) {
            method_definition(vm, chunk);
        } else {
            advance_parser();
            // Skip unknown tokens in class body for now
        }
    }
    
    consume(TOKEN_RBRACE, "Expected '}' after class body");
    
    // Store class in global (class is still on stack)
    emit_bytes(chunk, OP_SET_GLOBAL, class_name_idx);
    emit_byte(chunk, OP_POP);  // Pop the class from stack
    free(name_str);
}

// Parse method definition: fn methodName(params) { ... }
void method_definition(ember_vm* vm, ember_chunk* chunk) {
    (void)vm; // Parameter reserved for future implementation
    parser_state* parser = get_parser_state();
    
    // Get method name
    consume(TOKEN_IDENTIFIER, "Expected method name");
    ember_token method_name = parser->previous;
    
    char* name_str = malloc(method_name.length + 1);
    memcpy(name_str, method_name.start, method_name.length);
    name_str[method_name.length] = '\0';
    
    // Parse parameters
    consume(TOKEN_LPAREN, "Expected '(' after method name");
    
    int param_count = 0;
    if (!check(TOKEN_RPAREN)) {
        do {
            param_count++;
            consume(TOKEN_IDENTIFIER, "Expected parameter name");
            
            if (param_count > EMBER_MAX_ARGS) {
                error_at(&parser->current, "Too many parameters");
                break;
            }
        } while (match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RPAREN, "Expected ')' after parameters");
    
    // Parse method body
    consume(TOKEN_LBRACE, "Expected '{' before method body");
    
    // Create a new chunk for the method
    ember_chunk* method_chunk = malloc(sizeof(ember_chunk));
    init_chunk(method_chunk);
    
    // Track this method chunk for cleanup
    track_function_chunk(vm, method_chunk);
    
    // Parse method body into the method chunk
    // Keep parsing until we find the closing brace
    for (;;) {
        // Skip newlines and semicolons
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Continue
        }
        
        // Check for end of method body
        if (check(TOKEN_RBRACE)) {
            break;
        }
        if (check(TOKEN_EOF)) {
            break;  // Allow EOF to end method parsing gracefully
        }
        
        // Parse a statement
        statement(vm, method_chunk);
        
        // After statement, consume newline or semicolon if present
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Consumed separator, continue
        } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            error_at(&get_parser_state()->current, "Expect newline, semicolon, or '}' after statement");
            break;
        }
    }
    
    consume(TOKEN_RBRACE, "Expected '}' after method body");
    
    // Add implicit return at end of method
    emit_byte(method_chunk, OP_RETURN);
    
    // Create method value and emit it
    ember_value method_val;
    method_val.type = EMBER_VAL_FUNCTION;
    method_val.as.func_val.chunk = method_chunk;
    method_val.as.func_val.name = name_str;
    
    emit_constant(chunk, method_val);
    
    // Emit method name
    ember_value method_name_val = ember_make_string(name_str);
    emit_constant(chunk, method_name_val);
    
    // Define the method on the class
    emit_byte(chunk, OP_METHOD_DEF);
    
    // Note: name_str is NOT freed here because it's referenced by the method value
    // The garbage collector will handle this when the method is cleaned up
}

// Parse 'this' expression
void this_expression(ember_chunk* chunk) {
    // Emit code to get the current instance
    emit_byte(chunk, OP_GET_LOCAL);
    emit_byte(chunk, 0); // 'this' is always at local slot 0 in methods
}

// Parse 'super' expression: super.methodName
void super_expression(ember_chunk* chunk) {
    consume(TOKEN_DOT, "Expected '.' after 'super'");
    consume(TOKEN_IDENTIFIER, "Expected method name after 'super.'");
    
    parser_state* parser = get_parser_state();
    ember_token method_name = parser->previous;
    
    char* name_str = malloc(method_name.length + 1);
    memcpy(name_str, method_name.start, method_name.length);
    name_str[method_name.length] = '\0';
    
    ember_value method_name_val = ember_make_string(name_str);
    int method_name_idx = add_constant(chunk, method_name_val);
    emit_bytes(chunk, OP_GET_SUPER, method_name_idx);
    
    free(name_str);
}

// Parse 'new' expression: new ClassName(args)
void new_expression(ember_chunk* chunk) {
    consume(TOKEN_IDENTIFIER, "Expected class name after 'new'");
    
    parser_state* parser = get_parser_state();
    ember_token class_name = parser->previous;
    
    char* name_str = malloc(class_name.length + 1);
    memcpy(name_str, class_name.start, class_name.length);
    name_str[class_name.length] = '\0';
    
    // Load class from globals
    ember_value class_name_val = ember_make_string(name_str);
    int class_name_idx = add_constant(chunk, class_name_val);
    emit_bytes(chunk, OP_GET_GLOBAL, class_name_idx);
    
    // Create instance
    emit_byte(chunk, OP_INSTANCE_NEW);
    
    // Parse constructor arguments if present
    if (match(TOKEN_LPAREN)) {
        int arg_count = 0;
        if (!check(TOKEN_RPAREN)) {
            do {
                expression(chunk);
                arg_count++;
                if (arg_count > EMBER_MAX_ARGS) {
                    error_at(&parser->current, "Too many arguments");
                    break;
                }
            } while (match(TOKEN_COMMA));
        }
        
        consume(TOKEN_RPAREN, "Expected ')' after arguments");
        
        // Call init method if it exists
        ember_value init_name = ember_make_string("init");
        emit_constant(chunk, init_name);
        emit_bytes(chunk, OP_INVOKE, (uint8_t)arg_count);
    }
    // If no parentheses, just create the instance without calling init
    
    free(name_str);
}

// Parse dot expression: object.property or object.method(args)
void dot_expression(ember_chunk* chunk) {
    consume(TOKEN_IDENTIFIER, "Expected property name after '.'");
    
    parser_state* parser = get_parser_state();
    ember_token property_name = parser->previous;
    
    char* name_str = malloc(property_name.length + 1);
    memcpy(name_str, property_name.start, property_name.length);
    name_str[property_name.length] = '\0';
    
    ember_value property_name_val = ember_make_string(name_str);
    
    if (match(TOKEN_LPAREN)) {
        // Method call
        int arg_count = 0;
        if (!check(TOKEN_RPAREN)) {
            do {
                expression(chunk);
                arg_count++;
                if (arg_count > EMBER_MAX_ARGS) {
                    error_at(&parser->current, "Too many arguments");
                    break;
                }
            } while (match(TOKEN_COMMA));
        }
        
        consume(TOKEN_RPAREN, "Expected ')' after arguments");
        emit_constant(chunk, property_name_val);
        emit_bytes(chunk, OP_INVOKE, (uint8_t)arg_count);
    } else {
        // Property access
        int property_name_idx = add_constant(chunk, property_name_val);
        emit_bytes(chunk, OP_GET_PROPERTY, property_name_idx);
    }
    
    free(name_str);
}