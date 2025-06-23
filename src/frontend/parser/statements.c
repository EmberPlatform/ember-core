#include "parser.h"
#include "../../vm.h"
#include "../../runtime/value/value.h"
#include "../../runtime/package/package.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <math.h>

// Helper functions for increment expression generation
static void generate_postfix_increment(ember_chunk* chunk, ember_token* identifier, int delta) {
    // Create variable name string
    char* name = malloc(identifier->length + 1);
    memcpy(name, identifier->start, identifier->length);
    name[identifier->length] = '\0';
    
    ember_value name_val = ember_make_string_gc(get_parser_state()->vm, name);
    int const_idx = add_constant(chunk, name_val);
    free(name);
    
    // Load current value
    write_chunk(chunk, OP_GET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Add/subtract delta
    ember_value delta_val = ember_make_number((double)abs(delta));
    int delta_idx = add_constant(chunk, delta_val);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, delta_idx);
    
    if (delta > 0) {
        write_chunk(chunk, OP_ADD);
    } else {
        write_chunk(chunk, OP_SUB);
    }
    
    // Store new value
    write_chunk(chunk, OP_SET_GLOBAL);
    write_chunk(chunk, const_idx);
    // Note: OP_SET_GLOBAL leaves the value on stack, we should pop it
    // However, this needs to be conditional for for-loop continue statements
    write_chunk(chunk, OP_POP); // Pop the stored value
}

static void generate_prefix_increment(ember_chunk* chunk, ember_token* identifier, int delta) {
    // Similar to postfix, but the result value handling is different
    // For now, implement same as postfix since in for loops the result doesn't matter
    generate_postfix_increment(chunk, identifier, delta);
}

static void generate_compound_assignment(ember_chunk* chunk, ember_token* identifier, ember_token* operator, ember_token* value) {
    // Create variable name string
    char* name = malloc(identifier->length + 1);
    memcpy(name, identifier->start, identifier->length);
    name[identifier->length] = '\0';
    
    ember_value name_val = ember_make_string_gc(get_parser_state()->vm, name);
    int const_idx = add_constant(chunk, name_val);
    free(name);
    
    // Load current value
    write_chunk(chunk, OP_GET_GLOBAL);
    write_chunk(chunk, const_idx);
    
    // Push the operand value
    double operand_value = value->number;
    ember_value operand_val = ember_make_number(operand_value);
    int operand_idx = add_constant(chunk, operand_val);
    write_chunk(chunk, OP_PUSH_CONST);
    write_chunk(chunk, operand_idx);
    
    // Emit the appropriate operation
    switch (operator->type) {
        case TOKEN_PLUS_EQUAL:
            write_chunk(chunk, OP_ADD);
            break;
        case TOKEN_MINUS_EQUAL:
            write_chunk(chunk, OP_SUB);
            break;
        case TOKEN_MULTIPLY_EQUAL:
            write_chunk(chunk, OP_MUL);
            break;
        case TOKEN_DIVIDE_EQUAL:
            write_chunk(chunk, OP_DIV);
            break;
        default:
            // Fallback to addition
            write_chunk(chunk, OP_ADD);
            break;
    }
    
    // Store new value
    write_chunk(chunk, OP_SET_GLOBAL);
    write_chunk(chunk, const_idx);
    write_chunk(chunk, OP_POP); // Pop the stored value
}

void parse_loop_body(ember_vm* vm, ember_chunk* chunk) {
    if (match(TOKEN_LBRACE)) {
        // Parse block statement with multiple statements
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            // Skip empty lines/semicolons
            if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
                continue;
            }
            
            // Parse statement inside loop
            statement(vm, chunk);
            
            // After statement, consume separator if present
            if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
                // Consumed separator, continue
            } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                error("Expect newline, semicolon, or '}' after statement");
                break;
            }
        }
        
        if (check(TOKEN_RBRACE)) {
            advance_parser(); // Consume the closing brace
        } else {
            error("Expect '}' after loop body");
        }
    } else {
        // Parse single expression statement (original behavior)
        expression(chunk);
        write_chunk(chunk, OP_POP); // Pop body result from stack
    }
}

void while_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Set up loop context for break/continue
    if (parser->loop_depth >= 8) {
        error("Maximum loop nesting depth exceeded");
        return;
    }
    
    loop_context* loop_ctx = &parser->loop_stack[parser->loop_depth++];
    loop_ctx->break_count = 0;
    loop_ctx->continue_count = 0;
    
    int loop_start = chunk->count;
    loop_ctx->continue_target = loop_start;
    
    // Parse condition
    expression(chunk);
    
    // Jump out of loop if condition is false
    write_chunk(chunk, OP_JUMP_IF_FALSE);
    int exit_jump = chunk->count;
    write_chunk(chunk, 0); // Placeholder for jump offset (will be patched)
    
    // Parse body (supports both single expressions and block statements)
    parse_loop_body(vm, chunk);
    
    // Jump back to loop condition
    write_chunk(chunk, OP_LOOP);
    int loop_offset = chunk->count - loop_start + 2;
    write_chunk(chunk, loop_offset);
    
    // Patch the exit jump - jump to after the loop
    int exit_offset = chunk->count - exit_jump - 1;
    if (exit_offset < 0 || exit_offset > 255) {
        error("Jump offset too large for while loop");
        return;
    }
    chunk->code[exit_jump] = (uint8_t)exit_offset;
    
    // Patch all break statements to jump here
    for (int i = 0; i < loop_ctx->break_count; i++) {
        int break_jump = loop_ctx->break_jumps[i];
        chunk->code[break_jump] = chunk->count - break_jump - 1;
    }
    
    // Patch all continue statements to jump to loop start
    for (int i = 0; i < loop_ctx->continue_count; i++) {
        int continue_jump = loop_ctx->continue_jumps[i];
        // Calculate backward jump offset: jump from (continue_jump + 1) to continue_target
        int continue_offset = (continue_jump + 1) - loop_ctx->continue_target;
        if (continue_offset < 0 || continue_offset > 255) {
            error("Continue jump offset too large");
            continue;
        }
        chunk->code[continue_jump] = (uint8_t)continue_offset;
    }
    
    // Clean up loop context
    parser->loop_depth--;
    
    // While statements don't produce values - no need to push nil
}

void if_statement(ember_vm* vm, ember_chunk* chunk) {
    // Parse condition expression
    expression(chunk);
    
    // Jump to else clause if condition is false
    write_chunk(chunk, OP_JUMP_IF_FALSE);
    int then_jump = chunk->count;
    write_chunk(chunk, 0); // Placeholder for jump offset (will be patched)
    
    // Parse then body - support both single statement and block
    if (check(TOKEN_LBRACE)) {
        // Block statement
        consume(TOKEN_LBRACE, "Expect '{' for if block");
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
            
            // Parse statement inside control block
            statement(vm, chunk);
            
            // Handle statement separators within blocks
            if (!check(TOKEN_RBRACE)) {
                if (!match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
                    error("Expect newline or semicolon after statement in block");
                    return;
                }
            }
        }
        consume(TOKEN_RBRACE, "Expect '}' after if block");
    } else {
        // Single statement
        statement(vm, chunk);
    }
    
    // Jump over else clause
    write_chunk(chunk, OP_JUMP);
    int else_jump = chunk->count;
    write_chunk(chunk, 0); // Placeholder for jump offset (will be patched)
    
    // Patch the then jump to here (start of else clause)
    int then_offset = chunk->count - then_jump - 1;
    if (then_offset < 0 || then_offset > 255) {
        error("Jump offset too large for if statement");
        return;
    }
    chunk->code[then_jump] = (uint8_t)then_offset;
    
    // Parse optional else clause
    if (match(TOKEN_ELSE)) {
        if (check(TOKEN_LBRACE)) {
            // Block statement
            consume(TOKEN_LBRACE, "Expect '{' for else block");
            while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
                
                // Parse statement inside else block
                statement(vm, chunk);
                
                // Handle statement separators within blocks
                if (!check(TOKEN_RBRACE)) {
                    if (!match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
                        error("Expect newline or semicolon after statement in block");
                        return;
                    }
                }
            }
            consume(TOKEN_RBRACE, "Expect '}' after else block");
        } else {
            // Single statement
            statement(vm, chunk);
        }
    }
    
    // Patch the else jump to here (end of if statement)
    int else_offset = chunk->count - else_jump - 1;
    if (else_offset < 0 || else_offset > 255) {
        error("Jump offset too large for else clause");
        return;
    }
    chunk->code[else_jump] = (uint8_t)else_offset;
    
    // If statements don't produce values - no need to push nil
}

void break_statement(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    if (parser->loop_depth <= 0) {
        error("'break' can only be used inside loops");
        return;
    }
    
    // Emit OP_BREAK with placeholder offset
    write_chunk(chunk, OP_BREAK);
    int break_jump = chunk->count;
    write_chunk(chunk, 0); // Placeholder for jump offset (will be patched)
    
    // Store the break jump location for later patching
    loop_context* current_loop = &parser->loop_stack[parser->loop_depth - 1];
    if (current_loop->break_count < 16) {
        current_loop->break_jumps[current_loop->break_count++] = break_jump;
    } else {
        error("Too many break statements in single loop");
    }
}

void continue_statement(ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    if (parser->loop_depth <= 0) {
        error("'continue' can only be used inside loops");
        return;
    }
    
    // Emit OP_CONTINUE with placeholder offset
    write_chunk(chunk, OP_CONTINUE);
    int continue_jump = chunk->count;
    write_chunk(chunk, 0); // Placeholder for jump offset (will be patched)
    
    // Store the continue jump location for later patching
    loop_context* current_loop = &parser->loop_stack[parser->loop_depth - 1];
    if (current_loop->continue_count < 16) {
        current_loop->continue_jumps[current_loop->continue_count++] = continue_jump;
    } else {
        error("Too many continue statements in single loop");
    }
}

void for_statement(ember_vm* vm, ember_chunk* chunk) {
    // C-style for loop: for (init; condition; increment) { body }
    // Implementation: init; loop_start: if (!condition) goto end; body; increment; goto loop_start; end:
    
    consume(TOKEN_LPAREN, "Expect '(' after 'for'");
    
    // Parse and emit initialization expression
    if (!check(TOKEN_SEMICOLON)) {
        expression(chunk);
        write_chunk(chunk, OP_POP);  // Pop init result from stack
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after for loop initializer");
    
    // Set up loop context for break/continue statements
    parser_state* parser = get_parser_state();
    parser->loop_depth++;
    if (parser->loop_depth > 8) {  // Max loop depth from parser.h
        error("Too many nested loops");
        return;
    }
    
    loop_context* current_loop = &parser->loop_stack[parser->loop_depth - 1];
    current_loop->break_count = 0;
    current_loop->continue_count = 0;
    
    // Mark loop start (where condition is checked)
    int loop_start = chunk->count;
    
    // Parse and emit condition expression
    int exit_jump = -1;
    if (!check(TOKEN_SEMICOLON)) {
        expression(chunk);
        write_chunk(chunk, OP_JUMP_IF_FALSE);
        exit_jump = chunk->count;
        write_chunk(chunk, 0);  // Placeholder for jump offset (will be patched)
    }
    consume(TOKEN_SEMICOLON, "Expect ';' after for loop condition");
    
    // Save and collect increment expression tokens
    ember_token increment_tokens[64];  // Buffer for increment tokens
    int increment_token_count = 0;
    int increment_has_expression = 0;
    
    // Collect all tokens until closing parenthesis
    if (!check(TOKEN_RPAREN)) {
        increment_has_expression = 1;
        int paren_depth = 0;
        
        while (!check(TOKEN_EOF) && increment_token_count < 63) {
            if (check(TOKEN_LPAREN)) {
                paren_depth++;
            } else if (check(TOKEN_RPAREN)) {
                if (paren_depth == 0) {
                    break; // This is our closing paren
                }
                paren_depth--;
            }
            
            // Save current token
            increment_tokens[increment_token_count++] = parser->current;
            advance_parser();
        }
    }
    consume(TOKEN_RPAREN, "Expect ')' after for clauses");
    
    // Parse and emit loop body
    parse_loop_body(vm, chunk);
    
    // Track increment section start for continue statements
    int increment_start = -1;
    
    // Now emit the increment expression if it exists
    if (increment_has_expression && increment_token_count > 0) {
        // Mark the start of increment code for continue statements
        increment_start = chunk->count;
        
        // Generate bytecode for different increment patterns
        // Handle common patterns directly to avoid complex token replay
        if (increment_token_count == 2) {
            // Handle simple patterns: i++, i--, ++i, --i
            ember_token first = increment_tokens[0];
            ember_token second = increment_tokens[1];
            
            if (first.type == TOKEN_IDENTIFIER && second.type == TOKEN_PLUS_PLUS) {
                // Postfix increment: i++
                generate_postfix_increment(chunk, &first, 1);
            } else if (first.type == TOKEN_IDENTIFIER && second.type == TOKEN_MINUS_MINUS) {
                // Postfix decrement: i--
                generate_postfix_increment(chunk, &first, -1);
            } else if (first.type == TOKEN_PLUS_PLUS && second.type == TOKEN_IDENTIFIER) {
                // Prefix increment: ++i
                generate_prefix_increment(chunk, &second, 1);
            } else if (first.type == TOKEN_MINUS_MINUS && second.type == TOKEN_IDENTIFIER) {
                // Prefix decrement: --i
                generate_prefix_increment(chunk, &second, -1);
            }
        } else if (increment_token_count == 3) {
            // Handle compound assignments: i += 1, i -= 2, etc.
            ember_token var = increment_tokens[0];
            ember_token op = increment_tokens[1];
            ember_token val = increment_tokens[2];
            
            if (var.type == TOKEN_IDENTIFIER && val.type == TOKEN_NUMBER) {
                generate_compound_assignment(chunk, &var, &op, &val);
            }
        }
        
        // If we didn't handle it above with a specific pattern, try to handle as expression
        // For complex expressions, this is a fallback - in real implementation we'd want
        // to parse the full expression, but this covers the most common cases
    } else {
        // No increment expression, continue jumps to condition check
        current_loop->continue_target = loop_start;
    }
    
    // Jump back to condition check
    write_chunk(chunk, OP_LOOP);
    int loop_offset = chunk->count - loop_start + 2;
    write_chunk(chunk, loop_offset);
    
    // Patch exit jump if we had a condition
    if (exit_jump != -1) {
        chunk->code[exit_jump] = chunk->count - exit_jump - 1;
    }
    
    // Patch all break statements to jump here
    for (int i = 0; i < current_loop->break_count; i++) {
        int break_offset = current_loop->break_jumps[i];
        chunk->code[break_offset] = chunk->count - break_offset - 1;
    }
    
    // Patch all continue statements to jump to appropriate target
    int continue_target;
    if (increment_has_expression && increment_token_count > 0) {
        // For loops with increment: continue jumps to increment section
        continue_target = increment_start;
    } else {
        // For loops without increment: continue jumps to condition check
        continue_target = loop_start;
    }
    
    for (int i = 0; i < current_loop->continue_count; i++) {
        int continue_jump = current_loop->continue_jumps[i];
        
        if (increment_has_expression && increment_token_count > 0) {
            // For loops with increment: use forward jump (OP_JUMP) to increment section
            // Change OP_CONTINUE to OP_JUMP at continue_jump - 1
            chunk->code[continue_jump - 1] = OP_JUMP;
            int continue_offset = continue_target - continue_jump - 1;
            if (continue_offset < 0 || continue_offset > 255) {
                error("Continue jump offset too large");
                continue;
            }
            chunk->code[continue_jump] = (uint8_t)continue_offset;
        } else {
            // For loops without increment: use backward jump (OP_CONTINUE) to condition
            int continue_offset = (continue_jump + 1) - continue_target;
            if (continue_offset < 0 || continue_offset > 255) {
                error("Continue jump offset too large");
                continue;
            }
            chunk->code[continue_jump] = (uint8_t)continue_offset;
        }
    }
    
    // Clean up
    parser->loop_depth--;
    
    // For statements don't produce values - no need to push nil
}

void import_statement(ember_vm* vm, ember_chunk* chunk) {
    (void)chunk; // Parameter used in future implementation
    // Parse module name
    consume(TOKEN_IDENTIFIER, "Expect module name after 'import'");
    
    // Get module name
    int name_length = get_parser_state()->previous.length;
    char* module_name = malloc(name_length + 1);
    memcpy(module_name, get_parser_state()->previous.start, name_length);
    module_name[name_length] = '\0';
    
    // Version constraint parsing (optional)
    char version_constraint[EMBER_PACKAGE_MAX_VERSION_LEN] = "latest";
    if (match(TOKEN_AT)) {
        // Parse version constraint: import package@1.2.x
        // Version can be numbers, identifiers, and periods
        parser_state* parser = get_parser_state();
        
        // Manually parse version constraint to handle periods
        int version_length = 0;
        
        // Accept version patterns: 1.2.x, latest, 2.0.1, etc.
        while (!check(TOKEN_NEWLINE) && !check(TOKEN_SEMICOLON) && 
               !check(TOKEN_EOF) && !check(TOKEN_RBRACE) &&
               version_length < EMBER_PACKAGE_MAX_VERSION_LEN - 1) {
            
            char c = *parser->current.start;
            if (c == ' ' || c == '\t') break;  // Stop at whitespace
            
            version_constraint[version_length++] = c;
            advance_parser();
        }
        version_constraint[version_length] = '\0';
        
        // If nothing was parsed, default to latest
        if (version_length == 0) {
            strcpy(version_constraint, "latest");
        }
    }
    
    // Package management
    EmberPackage package;
    if (!ember_package_discover(module_name, &package)) {
        error("Failed to discover package");
        free(module_name);
        return;
    }
    
    // Copy version constraint
    strncpy(package.version, version_constraint, EMBER_PACKAGE_MAX_VERSION_LEN - 1);
    package.version[EMBER_PACKAGE_MAX_VERSION_LEN - 1] = '\0'; // Ensure null termination
    
    // Load package (this handles download if needed)
    if (!ember_package_load(&package)) {
        error("Failed to load package");
        free(module_name);
        return;
    }
    
    // Add to global package registry
    EmberPackageRegistry* registry = ember_package_get_global_registry();
    if (registry) {
        ember_package_registry_add(registry, &package);
    }
    
    printf("[IMPORT] Successfully imported %s@%s\n", module_name, version_constraint);
    
    // Legacy import call for existing functionality
    int result = ember_import_module(vm, module_name);
    if (result == -1) {
        // Legacy module system not found, using modern package system (silent)
    }
    
    // Import statements don't produce values - no need to push nil
    
    free(module_name);
}

void statement(ember_vm* vm, ember_chunk* chunk) {
    if (match(TOKEN_ASYNC)) {
        // Handle async function declaration
        consume(TOKEN_FN, "Expect 'fn' after 'async'");
        async_function_definition(vm, chunk);
    } else if (match(TOKEN_FN)) {
        // Check if this is a generator function (fn* syntax)
        if (check(TOKEN_MULTIPLY)) {
            advance_parser(); // consume '*'
            generator_function_definition(vm, chunk);
        } else {
            function_definition(vm, chunk);
        }
    } else if (match(TOKEN_FUNCTION)) {
        // Support for 'function' keyword
        function_definition(vm, chunk);
    } else if (match(TOKEN_CLASS)) {
        class_declaration(vm, chunk);
    } else if (match(TOKEN_RETURN)) {
        return_statement(chunk);
    } else if (match(TOKEN_IF)) {
        if_statement(vm, chunk);
    } else if (match(TOKEN_WHILE)) {
        while_statement(vm, chunk);
    } else if (match(TOKEN_FOR)) {
        for_statement(vm, chunk);
    } else if (match(TOKEN_BREAK)) {
        break_statement(chunk);
    } else if (match(TOKEN_CONTINUE)) {
        continue_statement(chunk);
    } else if (match(TOKEN_IMPORT)) {
        import_statement(vm, chunk);
    } else if (match(TOKEN_TRY)) {
        try_statement(vm, chunk);
    } else if (match(TOKEN_THROW)) {
        throw_statement(chunk);
    } else if (match(TOKEN_SWITCH)) {
        switch_statement(vm, chunk);
    } else {
        expression_statement(chunk);
    }
}

void return_statement(ember_chunk* chunk) {
    if (!check(TOKEN_NEWLINE) && !check(TOKEN_SEMICOLON) && !check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        expression(chunk);
    } else {
        // Return nil if no expression
        ember_value nil_val = ember_make_nil();
        int const_idx = add_constant(chunk, nil_val);
        write_chunk(chunk, OP_PUSH_CONST);
        write_chunk(chunk, const_idx);
    }
    write_chunk(chunk, OP_RETURN);
}

void assignment_statement(ember_chunk* chunk) {
    // Check for indexed assignment: arr[index] = value or map["key"] = value
    parser_state* parser = get_parser_state();
    ember_token identifier = parser->current;
    
    advance_parser();  // consume identifier
    
    if (match(TOKEN_LBRACKET)) {
        // This is indexed assignment: identifier[index] = value
        
        // Load the container (array or hash map)
        char* name = malloc(identifier.length + 1);
        memcpy(name, identifier.start, identifier.length);
        name[identifier.length] = '\0';
        
        ember_value name_val = ember_make_string(name);
        int const_idx = add_constant(chunk, name_val);
        free(name);
        
        write_chunk(chunk, OP_GET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Parse index expression
        expression(chunk);
        consume(TOKEN_RBRACKET, "Expect ']' after index");
        
        // Expect assignment
        consume(TOKEN_EQUAL, "Expect '=' in assignment");
        
        // Parse value expression
        expression(chunk);
        
        // Emit indexed assignment instruction
        write_chunk(chunk, OP_ARRAY_SET);
        
        // Pop the result to prevent stack buildup
        write_chunk(chunk, OP_POP);
    } else if (match(TOKEN_EQUAL)) {
        // Regular variable assignment: identifier = value
        char* name = malloc(identifier.length + 1);
        memcpy(name, identifier.start, identifier.length);
        name[identifier.length] = '\0';
        
        ember_value name_val = ember_make_string(name);
        int const_idx = add_constant(chunk, name_val);
        free(name);
        
        // Parse value expression
        expression(chunk);
        
        // Emit variable assignment instruction
        write_chunk(chunk, OP_SET_GLOBAL);
        write_chunk(chunk, const_idx);
        
        // Pop the result to prevent stack buildup  
        write_chunk(chunk, OP_POP);
    } else {
        // This is actually just an expression starting with an identifier
        // We need to backup and parse it as a regular expression
        parser->current = identifier; // Reset to identifier
        
        int start_count = chunk->count;
        expression(chunk);
        
        // Check if this was an assignment by looking for OP_SET_GLOBAL in the generated code
        bool has_assignment = false;
        for (int i = start_count; i < chunk->count; i++) {
            if (chunk->code[i] == OP_SET_GLOBAL) {
                has_assignment = true;
                break;
            }
        }
        
        // Pop assignment results to prevent stack buildup, but preserve other expression results
        if (has_assignment) {
            write_chunk(chunk, OP_POP);
        }
    }
}

void expression_statement(ember_chunk* chunk) {
    int start_count = chunk->count;
    expression(chunk);
    
    // Check if this was an assignment by looking for OP_SET_GLOBAL or OP_ARRAY_SET in the generated code
    bool has_assignment = false;
    for (int i = start_count; i < chunk->count; i++) {
        if (chunk->code[i] == OP_SET_GLOBAL || chunk->code[i] == OP_ARRAY_SET) {
            has_assignment = true;
            break;
        }
    }
    
    // Pop assignment results to prevent stack buildup, but preserve other expression results
    if (has_assignment) {
        write_chunk(chunk, OP_POP);
    }
}

void function_definition(ember_vm* vm, ember_chunk* chunk) {
    (void)chunk; // Parameter used in future implementation
    // Parse function name
    consume(TOKEN_IDENTIFIER, "Expect function name");
    
    // Get function name
    int name_length = get_parser_state()->previous.length;
    char* func_name = malloc(name_length + 1);
    memcpy(func_name, get_parser_state()->previous.start, name_length);
    func_name[name_length] = '\0';
    
    // Parse parameter list
    consume(TOKEN_LPAREN, "Expect '(' after function name");
    
    int param_count = 0;
    if (!check(TOKEN_RPAREN)) {
        do {
            consume(TOKEN_IDENTIFIER, "Expect parameter name");
            param_count++;
        } while (match(TOKEN_COMMA));
    }
    consume(TOKEN_RPAREN, "Expect ')' after parameters");
    
    // Parse function body
    consume(TOKEN_LBRACE, "Expect '{' before function body");
    
    // Create a new chunk for the function body
    ember_chunk* func_chunk = malloc(sizeof(ember_chunk));
    init_chunk(func_chunk);
    
    // Track this function chunk for cleanup
    track_function_chunk(vm, func_chunk);
    
    // Parse function body statements
    // Keep parsing until we find the closing brace
    for (;;) {
        // Skip newlines and semicolons
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Continue
        }
        
        // Check for end of function body
        if (check(TOKEN_RBRACE)) {
            break;
        }
        if (check(TOKEN_EOF)) {
            break;  // Allow EOF to end function parsing gracefully
        }
        
        // Parse a statement
        statement(vm, func_chunk);
        
        // After statement, consume newline or semicolon if present
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Consumed separator, continue
        } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            error("Expect newline, semicolon, or '}' after statement");
            break;
        }
    }
    
    if (check(TOKEN_RBRACE)) {
        advance_parser(); // Consume the closing brace
    } else {
        error("Expect '}' after function body");
    }
    
    // Add return instruction at end of function if not already present
    write_chunk(func_chunk, OP_RETURN);
    
    // Create function value
    ember_value func_val;
    func_val.type = EMBER_VAL_FUNCTION;
    func_val.as.func_val.chunk = func_chunk;
    func_val.as.func_val.name = func_name;
    
    // Store function in global scope
    vm->globals[vm->global_count].key = func_name;
    vm->globals[vm->global_count].value = func_val;
    vm->global_count++;
    
    // Function definition doesn't need to push values to main execution stack
}

void try_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Set up exception context
    if (parser->exception_depth >= 8) {
        error("Maximum exception nesting depth exceeded");
        return;
    }
    
    exception_context* exc_ctx = &parser->exception_stack[parser->exception_depth++];
    exc_ctx->try_start = chunk->count;
    exc_ctx->catch_start = -1;
    exc_ctx->finally_start = -1;
    exc_ctx->stack_depth = 0; // Will be set by VM at runtime
    
    // Emit TRY_BEGIN instruction
    write_chunk(chunk, OP_TRY_BEGIN);
    exc_ctx->handler_index = chunk->count;
    write_chunk(chunk, 0xFF); // Placeholder for handler index
    
    // Parse try block
    consume(TOKEN_LBRACE, "Expect '{' after 'try'");
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
        statement(vm, chunk);
        if (!check(TOKEN_RBRACE)) {
            if (!match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
                error("Expect newline or semicolon after statement in try block");
                return;
            }
        }
    }
    consume(TOKEN_RBRACE, "Expect '}' after try block");
    
    // Emit TRY_END instruction
    write_chunk(chunk, OP_TRY_END);
    
    // Handle catch block
    if (match(TOKEN_CATCH)) {
        exc_ctx->catch_start = chunk->count;
        
        // Optional exception variable binding
        char* exception_var = NULL;
        if (match(TOKEN_LPAREN)) {
            if (check(TOKEN_IDENTIFIER)) {
                ember_token var_token = parser->current;
                exception_var = malloc(var_token.length + 1);
                memcpy(exception_var, var_token.start, var_token.length);
                exception_var[var_token.length] = '\0';
                advance_parser();
            }
            consume(TOKEN_RPAREN, "Expect ')' after catch variable");
        }
        
        // Emit CATCH_BEGIN instruction
        write_chunk(chunk, OP_CATCH_BEGIN);
        if (exception_var) {
            // Add exception variable name as constant
            ember_value var_name = ember_make_string(exception_var);
            int const_idx = add_constant(chunk, var_name);
            write_chunk(chunk, const_idx);
            free(exception_var);
        } else {
            write_chunk(chunk, 0xFF); // No variable binding
        }
        
        // Parse catch block
        consume(TOKEN_LBRACE, "Expect '{' after 'catch'");
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
            statement(vm, chunk);
            if (!check(TOKEN_RBRACE)) {
                if (!match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
                    error("Expect newline or semicolon after statement in catch block");
                    return;
                }
            }
        }
        consume(TOKEN_RBRACE, "Expect '}' after catch block");
        
        // Emit CATCH_END instruction
        write_chunk(chunk, OP_CATCH_END);
    }
    
    // Handle finally block
    if (match(TOKEN_FINALLY)) {
        exc_ctx->finally_start = chunk->count;
        
        // Emit FINALLY_BEGIN instruction
        write_chunk(chunk, OP_FINALLY_BEGIN);
        
        // Parse finally block
        consume(TOKEN_LBRACE, "Expect '{' after 'finally'");
        while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
            statement(vm, chunk);
            if (!check(TOKEN_RBRACE)) {
                if (!match(TOKEN_NEWLINE) && !match(TOKEN_SEMICOLON) && !check(TOKEN_EOF)) {
                    error("Expect newline or semicolon after statement in finally block");
                    return;
                }
            }
        }
        consume(TOKEN_RBRACE, "Expect '}' after finally block");
        
        // Emit FINALLY_END instruction
        write_chunk(chunk, OP_FINALLY_END);
    }
    
    // Must have at least catch or finally
    if (exc_ctx->catch_start == -1 && exc_ctx->finally_start == -1) {
        error("'try' statement must have either 'catch' or 'finally' block");
    }
    
    // Clean up exception context
    parser->exception_depth--;
}

void throw_statement(ember_chunk* chunk) {
    // Parse the exception expression
    expression(chunk);
    
    // Emit THROW instruction
    write_chunk(chunk, OP_THROW);
}

// Async function definition
void async_function_definition(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Parse function name
    consume(TOKEN_IDENTIFIER, "Expect function name");
    ember_token name = parser->previous;
    
    // Create variable name string
    char* name_str = malloc(name.length + 1);
    memcpy(name_str, name.start, name.length);
    name_str[name.length] = '\0';
    
    ember_value name_val = ember_make_string_gc(vm, name_str);
    int const_idx = add_constant(chunk, name_val);
    (void)const_idx; // Reserved for future bytecode implementation
    free(name_str);
    
    // Parse parameters
    consume(TOKEN_LPAREN, "Expect '(' after function name");
    
    // Skip parameter parsing for now (could implement later)
    while (!check(TOKEN_RPAREN) && !check(TOKEN_EOF)) {
        advance_parser();
    }
    consume(TOKEN_RPAREN, "Expect ')' after parameters");
    
    // Parse function body with async context
    consume(TOKEN_LBRACE, "Expect '{' before function body");
    
    // Set async context
    int prev_async = parser->in_async_function;
    parser->in_async_function = 1;
    
    // Create a new chunk for the async function body
    ember_chunk* func_chunk = malloc(sizeof(ember_chunk));
    init_chunk(func_chunk);
    track_function_chunk(vm, func_chunk);
    
    // Parse function body statements
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Skip separators
        }
        if (check(TOKEN_RBRACE)) break;
        
        statement(vm, func_chunk);
        
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Consumed separator
        } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            error("Expect newline, semicolon, or '}' after statement");
            break;
        }
    }
    
    consume(TOKEN_RBRACE, "Expect '}' after async function body");
    
    // Restore async context
    parser->in_async_function = prev_async;
    
    // Add return instruction
    write_chunk(func_chunk, OP_RETURN);
    
    // Create async function value - for now, just store as regular function
    // In a full implementation, this would wrap the function to return a Promise
    ember_value func_val;
    func_val.type = EMBER_VAL_FUNCTION;
    func_val.as.func_val.chunk = func_chunk;
    func_val.as.func_val.name = NULL; // Avoid use-after-free
    
    // Store function directly in globals without stack operations
    char* name_cstr = (name_val.type == EMBER_VAL_STRING && name_val.as.obj_val) ? 
                      AS_CSTRING(name_val) : (char*)name_val.as.string_val;
    vm->globals[vm->global_count].key = malloc(strlen(name_cstr) + 1);
    strcpy(vm->globals[vm->global_count].key, name_cstr);
    vm->globals[vm->global_count].value = func_val;
    vm->global_count++;
}

// Generator function definition
void generator_function_definition(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Parse function name
    consume(TOKEN_IDENTIFIER, "Expect generator function name");
    ember_token name = parser->previous;
    
    // Create variable name string
    char* name_str = malloc(name.length + 1);
    memcpy(name_str, name.start, name.length);
    name_str[name.length] = '\0';
    
    ember_value name_val = ember_make_string_gc(vm, name_str);
    int const_idx = add_constant(chunk, name_val);
    (void)const_idx; // Reserved for future bytecode implementation
    free(name_str);
    
    // Parse parameters
    consume(TOKEN_LPAREN, "Expect '(' after generator function name");
    
    // Skip parameter parsing for now
    while (!check(TOKEN_RPAREN) && !check(TOKEN_EOF)) {
        advance_parser();
    }
    consume(TOKEN_RPAREN, "Expect ')' after parameters");
    
    // Parse function body with generator context
    consume(TOKEN_LBRACE, "Expect '{' before generator function body");
    
    // Set generator context
    int prev_generator = parser->in_generator_function;
    parser->in_generator_function = 1;
    
    // Create a new chunk for the generator function body
    ember_chunk* func_chunk = malloc(sizeof(ember_chunk));
    init_chunk(func_chunk);
    track_function_chunk(vm, func_chunk);
    
    // Parse function body statements
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        while (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Skip separators
        }
        if (check(TOKEN_RBRACE)) break;
        
        statement(vm, func_chunk);
        
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
            // Consumed separator
        } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
            error("Expect newline, semicolon, or '}' after statement");
            break;
        }
    }
    
    consume(TOKEN_RBRACE, "Expect '}' after generator function body");
    
    // Restore generator context
    parser->in_generator_function = prev_generator;
    
    // Add return instruction (generators auto-complete when they reach the end)
    write_chunk(func_chunk, OP_RETURN);
    
    // Create generator constructor - this would create a generator when called
    ember_value gen_constructor;
    gen_constructor.type = EMBER_VAL_FUNCTION;
    gen_constructor.as.func_val.chunk = func_chunk;
    gen_constructor.as.func_val.name = NULL; // Avoid use-after-free
    
    // Store generator function directly in globals without stack operations
    char* name_cstr = (name_val.type == EMBER_VAL_STRING && name_val.as.obj_val) ? 
                      AS_CSTRING(name_val) : (char*)name_val.as.string_val;
    vm->globals[vm->global_count].key = malloc(strlen(name_cstr) + 1);
    strcpy(vm->globals[vm->global_count].key, name_cstr);
    vm->globals[vm->global_count].value = gen_constructor;
    vm->global_count++;
}

// Switch statement implementation
void switch_statement(ember_vm* vm, ember_chunk* chunk) {
    parser_state* parser = get_parser_state();
    
    // Parse switch expression
    consume(TOKEN_LPAREN, "Expect '(' after 'switch'");
    expression(chunk);
    consume(TOKEN_RPAREN, "Expect ')' after switch expression");
    
    // Set up switch context for case tracking
    if (parser->loop_depth >= 8) {
        error("Maximum nesting depth exceeded");
        return;
    }
    
    loop_context* switch_ctx = &parser->loop_stack[parser->loop_depth++];
    switch_ctx->break_count = 0;
    
    // Parse switch body
    consume(TOKEN_LBRACE, "Expect '{' before switch body");
    
    int case_count = 0;
    int case_jumps[32];  // Store case jump locations
    int default_jump = -1;
    
    while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
        if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
        
        if (match(TOKEN_CASE)) {
            // Parse case value
            expression(chunk);
            
            // Emit case comparison
            write_chunk(chunk, OP_CASE);
            case_jumps[case_count] = chunk->count;
            write_chunk(chunk, 0); // Placeholder for jump offset
            case_count++;
            
            consume(TOKEN_COLON, "Expect ':' after case value");
            
            // Parse case body
            while (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) && 
                   !check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
                statement(vm, chunk);
                if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
                    // Consumed separator
                } else if (!check(TOKEN_CASE) && !check(TOKEN_DEFAULT) && 
                          !check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                    error("Expect newline, semicolon, or next case after statement");
                    break;
                }
            }
        } else if (match(TOKEN_DEFAULT)) {
            consume(TOKEN_COLON, "Expect ':' after 'default'");
            
            // Mark default case location
            default_jump = chunk->count;
            
            // Parse default body
            while (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) continue;
                statement(vm, chunk);
                if (match(TOKEN_NEWLINE) || match(TOKEN_SEMICOLON)) {
                    // Consumed separator
                } else if (!check(TOKEN_RBRACE) && !check(TOKEN_EOF)) {
                    error("Expect newline, semicolon, or '}' after statement");
                    break;
                }
            }
            break; // Default should be last
        } else {
            error("Expect 'case' or 'default' in switch statement");
            break;
        }
    }
    
    consume(TOKEN_RBRACE, "Expect '}' after switch body");
    
    // Emit default case jump if needed
    if (default_jump != -1) {
        write_chunk(chunk, OP_DEFAULT);
        write_chunk(chunk, default_jump - chunk->count - 1);
    }
    
    // Patch all break statements to jump here
    for (int i = 0; i < switch_ctx->break_count; i++) {
        int break_jump = switch_ctx->break_jumps[i];
        chunk->code[break_jump] = chunk->count - break_jump - 1;
    }
    
    // Clean up switch context
    parser->loop_depth--;
    
    // Switch statements don't produce values
}