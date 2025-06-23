#include "parser.h"
#include "../../vm.h"
#include "../../runtime/value/value.h"
#include "../lexer/lexer.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

// Forward declarations for OOP expressions (implemented in oop.c)
extern void this_expression(ember_chunk* chunk);
extern void super_expression(ember_chunk* chunk);
extern void new_expression(ember_chunk* chunk);
extern void dot_expression(ember_chunk* chunk);

void number_literal(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    double value = parser->previous.number;
    ember_value num_val = ember_make_number(value);
    int const_idx = add_constant(chunk, num_val);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, const_idx);
}

void string_literal(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    // Extract string content (skip quotes) with bounds checking
    if (parser->previous.length < 2) {
        // Invalid string token - should have at least opening and closing quotes
        return;
    }
    int length = parser->previous.length - 2;
    if (length < 0) {
        // Additional safety check
        return;
    }
    char* str = malloc(length + 1);
    if (str) {
        if (length > 0) {
            memcpy(str, parser->previous.start + 1, length);
        }
        str[length] = '\0';
    } else {
        // Memory allocation failed
        return;
    }
    
    // Use GC-managed string creation for proper concatenation support
    ember_value string_val = ember_make_string_gc(parser->vm, str);
    int const_idx = add_constant(chunk, string_val);
    free(str);
    
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, const_idx);
}

void interpolated_string_literal(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Extract string content (skip quotes) with bounds checking
    if (parser->previous.length < 2) {
        // Invalid string token - should have at least opening and closing quotes
        return;
    }
    int length = parser->previous.length - 2;
    if (length < 0) {
        // Additional safety check
        return;
    }
    char* str = malloc(length + 1);
    if (str) {
        if (length > 0) {
            memcpy(str, parser->previous.start + 1, length);
        }
        str[length] = '\0';
    } else {
        // Memory allocation failed
        return;
    }
    
    // For now, use the OP_STRING_INTERPOLATE opcode to handle the interpolation
    // Store the raw interpolated string as a constant and let the VM handle parsing
    ember_value string_val = ember_make_string_gc(parser->vm, str);
    int const_idx = add_constant(chunk, string_val);
    
    free(str);
    
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, const_idx);
    write_chunk(chunk, OP_STRING_INTERPOLATE);
}

void boolean_literal(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    int is_true = parser->previous.type == TOKEN_TRUE;
    ember_value bool_val = ember_make_bool(is_true);
    int const_idx = add_constant(chunk, bool_val);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, const_idx);
}

void variable(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    // Create string value from token
    int length = parser->previous.length;
    char* name = malloc(length + 1);
    memcpy(name, parser->previous.start, length);
    name[length] = '\0';
    
    ember_value name_val = ember_make_string_gc(parser->vm, name);
    int const_idx = add_constant(chunk, name_val);
    free(name);
    
    if (match(TOKEN_EQUAL)) {
        // Assignment: x = value
        expression(chunk);
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
    } else if (match(TOKEN_PLUS_EQUAL)) {
        // Compound assignment: x += value
        // Load current value
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        // Parse right side
        expression(chunk);
        // Add them
        write_chunk(chunk, OP_ADD);
        // Store result
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
    } else if (match(TOKEN_MINUS_EQUAL)) {
        // Compound assignment: x -= value
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        expression(chunk);
        write_chunk(chunk, OP_SUB);
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
    } else if (match(TOKEN_MULTIPLY_EQUAL)) {
        // Compound assignment: x *= value
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        expression(chunk);
        write_chunk(chunk, OP_MUL);
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
    } else if (match(TOKEN_DIVIDE_EQUAL)) {
        // Compound assignment: x /= value
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        expression(chunk);
        write_chunk(chunk, OP_DIV);
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
    } else if (match(TOKEN_PLUS_PLUS)) {
        // Postfix increment: x++
        // Load current value first (for return value)
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Load current value again for increment
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Add 1
        ember_value one = ember_make_number(1.0);
        int one_idx = add_constant(chunk, one);
        write_chunk(chunk, OP_PUSH_CONST);
        write_chunk(chunk, one_idx);
        write_chunk(chunk, OP_ADD);
        
        // Store new value
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
        write_chunk(chunk, OP_POP); // Pop the stored value
        
        // Original value is still on stack as return value
    } else if (match(TOKEN_MINUS_MINUS)) {
        // Postfix decrement: x--
        // Load current value first (for return value)
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Load current value again for decrement
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Subtract 1
        ember_value one = ember_make_number(1.0);
        int one_idx = add_constant(chunk, one);
        write_chunk(chunk, OP_PUSH_CONST);
        write_chunk(chunk, one_idx);
        write_chunk(chunk, OP_SUB);
        
        // Store new value
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
        write_chunk(chunk, OP_POP); // Pop the stored value
        
        // Original value is still on stack as return value
    } else if (match(TOKEN_LPAREN)) {
        // Function call: func(args)
        int arg_count = 0;
        
        if (!check(TOKEN_RPAREN)) {
            do {
                expression(chunk);
                arg_count++;
            } while (match(TOKEN_COMMA));
        }
        
        consume(TOKEN_RPAREN, "Expect ')' after arguments");
        
        // Load function name
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Call function
        write_chunk(chunk, OP_CALL);
        write_chunk(chunk, arg_count);
    } else {
        // Variable access
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
    }
}

void call(ember_chunk* chunk) {
    // Parse argument list
    int arg_count = 0;
    
    if (!check(TOKEN_RPAREN)) {
        do {
            expression(chunk);
            arg_count++;
        } while (match(TOKEN_COMMA));
    }
    
    consume(TOKEN_RPAREN, "Expect ')' after arguments");
    write_chunk(chunk, OP_CALL);
    write_chunk(chunk, arg_count);
}

void unary(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    ember_token_type operator_type = parser->previous.type;
    
    // Compile the operand
    parse_precedence(PREC_UNARY, chunk);
    
    // Emit the operator instruction
    switch (operator_type) {
        case TOKEN_MINUS: 
            // For negative numbers, push -1 and multiply
            {
                ember_value neg_one = ember_make_number(-1);
                int const_idx = add_constant(chunk, neg_one);
                write_chunk(chunk, OP_PUSH_CONST);
                write_chunk(chunk, const_idx);
                write_chunk(chunk, OP_MUL);
            }
            break;
        default:
            return; // Unreachable
    }
}

void binary(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    ember_token_type operator_type = parser->previous.type;
    
    precedence prec = PREC_TERM;
    switch (operator_type) {
        case TOKEN_MULTIPLY:
        case TOKEN_DIVIDE:
        case TOKEN_MODULO:
            prec = PREC_FACTOR;
            break;
        case TOKEN_EQUAL_EQUAL:
        case TOKEN_NOT_EQUAL:
            prec = PREC_EQUALITY;
            break;
        case TOKEN_GREATER:
        case TOKEN_GREATER_EQUAL:
        case TOKEN_LESS:
        case TOKEN_LESS_EQUAL:
            prec = PREC_COMPARISON;
            break;
        default:
            break;
    }
    
    parse_precedence((precedence)(prec + 1), chunk);
    
    switch (operator_type) {
        case TOKEN_PLUS: write_chunk(chunk, OP_ADD); break;
        case TOKEN_MINUS: write_chunk(chunk, OP_SUB); break;
        case TOKEN_MULTIPLY: write_chunk(chunk, OP_MUL); break;
        case TOKEN_DIVIDE: write_chunk(chunk, OP_DIV); break;
        case TOKEN_MODULO: write_chunk(chunk, OP_MOD); break;
        case TOKEN_EQUAL_EQUAL: write_chunk(chunk, OP_EQUAL); break;
        case TOKEN_NOT_EQUAL: write_chunk(chunk, OP_NOT_EQUAL); break;
        case TOKEN_GREATER: write_chunk(chunk, OP_GREATER); break;
        case TOKEN_GREATER_EQUAL: write_chunk(chunk, OP_GREATER_EQUAL); break;
        case TOKEN_LESS: write_chunk(chunk, OP_LESS); break;
        case TOKEN_LESS_EQUAL: write_chunk(chunk, OP_LESS_EQUAL); break;
        default: return; // Unreachable
    }
}

void logical_and_binary(ember_chunk* chunk) {
    // For logical AND (&&), we simply use the OP_AND opcode
    // The VM implementation handles the logical semantics
    parse_precedence((precedence)(PREC_AND + 1), chunk);
    write_chunk(chunk, OP_AND);
}

void logical_or_binary(ember_chunk* chunk) {
    // For logical OR (||), we simply use the OP_OR opcode which handles short-circuit
    parse_precedence((precedence)(PREC_OR + 1), chunk);
    write_chunk(chunk, OP_OR);
}

void logical_not_unary(ember_chunk* chunk) {
    parse_precedence(PREC_UNARY, chunk);
    write_chunk(chunk, OP_NOT);
}

void prefix_increment(ember_chunk* chunk) {
    // Parse the variable name that follows
    consume(TOKEN_IDENTIFIER, "Expect variable name after '++'");
    
    parser_state* parser = get_parser_state();
    int length = parser->previous.length;
    char* name = malloc(length + 1);
    memcpy(name, parser->previous.start, length);
    name[length] = '\0';
    
    ember_value name_val = ember_make_string_gc(parser->vm, name);
    int const_idx = add_constant(chunk, name_val);
    free(name);
    
    // Load current value
    write_chunk(chunk, OP_GET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Add 1
    ember_value one = ember_make_number(1.0);
    int one_idx = add_constant(chunk, one);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, one_idx);
    write_chunk(chunk, OP_ADD);
    
    // Store new value
    write_chunk(chunk, OP_SET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Leave the new value on stack for the expression result
}

void prefix_decrement(ember_chunk* chunk) {
    // Parse the variable name that follows
    consume(TOKEN_IDENTIFIER, "Expect variable name after '--'");
    
    parser_state* parser = get_parser_state();
    int length = parser->previous.length;
    char* name = malloc(length + 1);
    memcpy(name, parser->previous.start, length);
    name[length] = '\0';
    
    ember_value name_val = ember_make_string_gc(parser->vm, name);
    int const_idx = add_constant(chunk, name_val);
    free(name);
    
    // Load current value
    write_chunk(chunk, OP_GET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Subtract 1
    ember_value one = ember_make_number(1.0);
    int one_idx = add_constant(chunk, one);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, one_idx);
    write_chunk(chunk, OP_SUB);
    
    // Store new value
    write_chunk(chunk, OP_SET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Leave the new value on stack for the expression result
}


void grouping(ember_chunk* chunk) {
    expression(chunk);
    consume(TOKEN_RPAREN, "Expect ')' after expression");
}

void array_literal(ember_chunk* chunk) {
    int element_count = 0;
    
    // Skip newlines after opening bracket
    while (match(TOKEN_NEWLINE)) { /* skip */ }
    
    // Handle empty array
    if (!check(TOKEN_RBRACKET)) {
        do {
            // Skip newlines before element
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            
            expression(chunk);
            element_count++;
            
            // Skip newlines after element
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            
        } while (match(TOKEN_COMMA));
    }
    
    // Skip newlines before closing bracket
    while (match(TOKEN_NEWLINE)) { /* skip */ }
    consume(TOKEN_RBRACKET, "Expect ']' after array elements");
    
    // Emit instruction to create array with element count
    write_chunk(chunk, OP_ARRAY_NEW);
    write_chunk(chunk, element_count);
}

void array_index(ember_chunk* chunk) {
    expression(chunk);  // Parse index expression
    consume(TOKEN_RBRACKET, "Expect ']' after array index");
    
    // Check if this is an assignment: arr[index] = value
    if (match(TOKEN_EQUAL)) {
        // This is indexed assignment
        expression(chunk);  // Parse the value expression
        write_chunk(chunk, OP_ARRAY_SET);
    } else {
        // This is just array access
        write_chunk(chunk, OP_ARRAY_GET);
    }
}

void hash_map_literal(ember_chunk* chunk) {
    int pair_count = 0;
    
    // Skip newlines after opening brace
    while (match(TOKEN_NEWLINE)) { /* skip */ }
    
    // Handle empty hash map
    if (!check(TOKEN_RBRACE)) {
        do {
            // Skip newlines before key
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            
            // Parse key
            expression(chunk);
            
            // Skip newlines before colon
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            consume(TOKEN_COLON, "Expect ':' after hash map key");
            
            // Skip newlines after colon
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            
            // Parse value
            expression(chunk);
            pair_count++;
            
            // Skip newlines after value
            while (match(TOKEN_NEWLINE)) { /* skip */ }
            
        } while (match(TOKEN_COMMA) && (match(TOKEN_NEWLINE) || true));
    }
    
    // Skip newlines before closing brace
    while (match(TOKEN_NEWLINE)) { /* skip */ }
    consume(TOKEN_RBRACE, "Expect '}' after hash map entries");
    
    // Emit instruction to create hash map with pair count
    write_chunk(chunk, OP_HASH_MAP_NEW);
    write_chunk(chunk, pair_count);
}

// Parse rule table
typedef struct {
    void (*prefix)(ember_chunk* chunk);
    void (*infix)(ember_chunk* chunk);
    precedence prec;
} parse_rule;

parse_rule* get_rule(ember_token_type type);

void parse_precedence(precedence prec, ember_chunk* chunk) {
    advance_parser();
    
    parse_rule* prefix_rule = get_rule(get_parser_state()->previous.type);
    if (prefix_rule->prefix == NULL) {
        // Provide helpful error messages for common mistakes
        ember_token_type token_type = get_parser_state()->previous.type;
        if (token_type == TOKEN_DIVIDE) {
            // Check if this might be an attempt at // comment
            if (get_parser_state()->current.type == TOKEN_DIVIDE) {
                error("Use '#' not '//' for comments");
            } else {
                error("Division operator '/' cannot be used as prefix. Expect expression");
            }
        } else {
            error("Expect expression");
        }
        return;
    }
    
    prefix_rule->prefix(chunk);
    
    while (prec <= get_rule(get_parser_state()->current.type)->prec) {
        advance_parser();
        parse_rule* infix_rule = get_rule(get_parser_state()->previous.type);
        infix_rule->infix(chunk);
    }
}

void expression(ember_chunk* chunk) {
    parse_precedence(PREC_ASSIGNMENT, chunk);
}

static parse_rule rules[] = {
    [TOKEN_LPAREN]        = {grouping,   call,              PREC_CALL},
    [TOKEN_RPAREN]        = {NULL,       NULL,              PREC_NONE},
    [TOKEN_MINUS]         = {unary,      binary,            PREC_TERM},
    [TOKEN_PLUS]          = {NULL,       binary,            PREC_TERM},
    [TOKEN_MULTIPLY]      = {NULL,       binary,            PREC_FACTOR},
    [TOKEN_DIVIDE]        = {NULL,       binary,            PREC_FACTOR},
    [TOKEN_MODULO]        = {NULL,       binary,            PREC_FACTOR},
    [TOKEN_PLUS_PLUS]     = {prefix_increment, NULL,        PREC_UNARY},
    [TOKEN_MINUS_MINUS]   = {prefix_decrement, NULL,        PREC_UNARY},
    [TOKEN_PLUS_EQUAL]    = {NULL,       NULL,              PREC_NONE},
    [TOKEN_MINUS_EQUAL]   = {NULL,       NULL,              PREC_NONE},
    [TOKEN_MULTIPLY_EQUAL] = {NULL,       NULL,              PREC_NONE},
    [TOKEN_DIVIDE_EQUAL]  = {NULL,       NULL,              PREC_NONE},
    [TOKEN_NUMBER]        = {number_literal, NULL,          PREC_NONE},
    [TOKEN_STRING]        = {string_literal, NULL,          PREC_NONE},
    [TOKEN_INTERPOLATED_STRING] = {interpolated_string_literal, NULL, PREC_NONE},
    [TOKEN_IDENTIFIER]    = {variable,   NULL,              PREC_NONE},
    [TOKEN_EQUAL_EQUAL]   = {NULL,       binary,            PREC_EQUALITY},
    [TOKEN_NOT_EQUAL]     = {NULL,       binary,            PREC_EQUALITY},
    [TOKEN_GREATER]       = {NULL,       binary,            PREC_COMPARISON},
    [TOKEN_GREATER_EQUAL] = {NULL,       binary,            PREC_COMPARISON},
    [TOKEN_LESS]          = {NULL,       binary,            PREC_COMPARISON},
    [TOKEN_LESS_EQUAL]    = {NULL,       binary,            PREC_COMPARISON},
    [TOKEN_AND]           = {NULL,       logical_and_binary, PREC_AND},
    [TOKEN_OR]            = {NULL,       logical_or_binary, PREC_OR},
    [TOKEN_NOT]           = {logical_not_unary, NULL,       PREC_NONE},
    [TOKEN_AND_AND]       = {NULL,       logical_and_binary, PREC_AND},
    [TOKEN_OR_OR]         = {NULL,       logical_or_binary, PREC_OR},
    [TOKEN_TRUE]          = {boolean_literal, NULL,        PREC_NONE},
    [TOKEN_FALSE]         = {boolean_literal, NULL,        PREC_NONE},
    [TOKEN_LBRACKET]      = {array_literal, array_index,   PREC_CALL},
    [TOKEN_RBRACKET]      = {NULL,       NULL,              PREC_NONE},
    [TOKEN_LBRACE]        = {hash_map_literal, NULL,       PREC_NONE},
    [TOKEN_RBRACE]        = {NULL,       NULL,              PREC_NONE},
    [TOKEN_CLASS]         = {NULL,       NULL,              PREC_NONE},
    [TOKEN_EXTENDS]       = {NULL,       NULL,              PREC_NONE},
    [TOKEN_NEW]           = {new_expression, NULL,          PREC_NONE},
    [TOKEN_THIS]          = {this_expression, NULL,         PREC_NONE},
    [TOKEN_SUPER]         = {super_expression, NULL,        PREC_NONE},
    [TOKEN_DOT]           = {NULL,       dot_expression,     PREC_CALL},
    [TOKEN_ASYNC]         = {NULL,       NULL,              PREC_NONE},
    [TOKEN_AWAIT]         = {await_expression, NULL,        PREC_UNARY},
    [TOKEN_YIELD]         = {yield_expression, NULL,        PREC_UNARY},
    [TOKEN_EOF]           = {NULL,       NULL,              PREC_NONE},
};

// Await expression
void await_expression(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Check if we're in an async context
    if (!parser->in_async_function) {
        error("'await' can only be used inside async functions");
        return;
    }
    
    // Parse the expression to await
    parse_precedence(PREC_UNARY, chunk);
    
    // Emit AWAIT instruction
    write_chunk(chunk, OP_AWAIT);
}

// Yield expression  
void yield_expression(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Check if we're in a generator context
    if (!parser->in_generator_function) {
        error("'yield' can only be used inside generator functions");
        return;
    }
    
    // Parse the expression to yield (optional)
    if (!check(TOKEN_SEMICOLON) && !check(TOKEN_NEWLINE) && !check(TOKEN_RBRACE)) {
        parse_precedence(PREC_ASSIGNMENT, chunk);
    } else {
        // No expression provided, yield undefined
        ember_value nil_val = ember_make_nil();
        int const_idx = add_constant(chunk, nil_val);
        write_chunk(chunk, OP_PUSH_CONST);
        write_chunk(chunk, const_idx);
    }
    
    // Emit YIELD instruction
    write_chunk(chunk, OP_YIELD);
}

parse_rule* get_rule(ember_token_type type) {
    return &rules[type];
}