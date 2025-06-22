#ifndef EMBER_PARSER_H
#define EMBER_PARSER_H

#include "ember.h"
#include "../lexer/lexer.h"
#include "../../runtime/package/package.h"

// Loop context for break/continue tracking
typedef struct {
    int break_jumps[16];    // Array to store break jump locations
    int break_count;        // Number of break jumps to patch
    int continue_target;    // Where continue should jump to
} loop_context;

// Exception context for try/catch/finally tracking
typedef struct {
    int handler_index;      // Index of exception handler in VM
    int try_start;          // Start of try block
    int catch_start;        // Start of catch block (-1 if no catch)
    int finally_start;      // Start of finally block (-1 if no finally)
    int stack_depth;        // Stack depth when try block started
} exception_context;

// Parser state
typedef struct {
    ember_token current;
    ember_token previous;
    int had_error;
    int panic_mode;
    ember_vm* vm;  // VM reference for GC-managed allocations
    loop_context loop_stack[8];  // Stack of nested loop contexts
    int loop_depth;              // Current loop nesting depth
    exception_context exception_stack[8];  // Stack of nested exception contexts
    int exception_depth;         // Current exception nesting depth
    int in_async_function;       // Whether we're parsing an async function
    int in_generator_function;   // Whether we're parsing a generator function
} parser_state;

// Precedence levels
typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_OR,         // or
    PREC_AND,        // and
    PREC_EQUALITY,   // == !=
    PREC_COMPARISON, // < > <= >=
    PREC_TERM,       // + -
    PREC_FACTOR,     // * /
    PREC_UNARY,      // ! -
    PREC_CALL,       // . ()
    PREC_PRIMARY
} precedence;

// Parser functions
int compile(ember_vm* vm, const char* source, ember_chunk* chunk);
void init_parser(void);

// Expression parsing
void expression(ember_chunk* chunk);
void parse_precedence(precedence prec, ember_chunk* chunk);

// Literals and values
void number_literal(ember_chunk* chunk);
void string_literal(ember_chunk* chunk);
void interpolated_string_literal(ember_chunk* chunk);
void boolean_literal(ember_chunk* chunk);
void variable(ember_chunk* chunk);

// Statements
void statement(ember_vm* vm, ember_chunk* chunk);
void assignment_statement(ember_chunk* chunk);
void expression_statement(ember_chunk* chunk);
void if_statement(ember_vm* vm, ember_chunk* chunk);
void while_statement(ember_vm* vm, ember_chunk* chunk);
void for_statement(ember_vm* vm, ember_chunk* chunk);
void parse_loop_body(ember_vm* vm, ember_chunk* chunk);
void function_definition(ember_vm* vm, ember_chunk* chunk);
void return_statement(ember_chunk* chunk);
void import_statement(ember_vm* vm, ember_chunk* chunk);
void break_statement(ember_chunk* chunk);
void continue_statement(ember_chunk* chunk);
void try_statement(ember_vm* vm, ember_chunk* chunk);
void throw_statement(ember_chunk* chunk);
void class_declaration(ember_vm* vm, ember_chunk* chunk);
void method_definition(ember_vm* vm, ember_chunk* chunk);

// Async/await and generator support
void async_function_definition(ember_vm* vm, ember_chunk* chunk);
void await_expression(ember_chunk* chunk);
void yield_expression(ember_chunk* chunk);
void generator_function_definition(ember_vm* vm, ember_chunk* chunk);

// Binary operations
void binary(ember_chunk* chunk);
void logical_and_binary(ember_chunk* chunk);
void logical_or_binary(ember_chunk* chunk);

// Unary operations
void unary(ember_chunk* chunk);
void logical_not_unary(ember_chunk* chunk);
void prefix_increment(ember_chunk* chunk);
void prefix_decrement(ember_chunk* chunk);
void postfix_increment(ember_chunk* chunk);

// Function calls
void call(ember_chunk* chunk);

// OOP expressions  
void this_expression(ember_chunk* chunk);
void super_expression(ember_chunk* chunk);
void new_expression(ember_chunk* chunk);
void dot_expression(ember_chunk* chunk);

// Error handling
void error_at(ember_token* token, const char* message);
void error(const char* message);
void error_at_current(const char* message);

// Token management
void advance_parser(void);
int check(ember_token_type type);
int match(ember_token_type type);
void consume(ember_token_type type, const char* message);

// Parser state access (for modular parser components)
parser_state* get_parser_state(void);

// Grouping and arrays
void grouping(ember_chunk* chunk);
void array_literal(ember_chunk* chunk);
void array_index(ember_chunk* chunk);

// Hash maps
void hash_map_literal(ember_chunk* chunk);

#endif // EMBER_PARSER_H