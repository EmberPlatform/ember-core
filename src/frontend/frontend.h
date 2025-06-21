#ifndef EMBER_FRONTEND_H
#define EMBER_FRONTEND_H

#include "lexer/lexer.h"
#include "parser/parser.h"
#include "../vm.h"

// Lexer interface
void init_scanner(const char* source);
ember_token scan_token(void);

// Parser interface
int compile(ember_vm* vm, const char* source, ember_chunk* chunk);
void init_parser(void);
void error_at(ember_token* token, const char* message);
void error(const char* message);
void error_at_current(const char* message);

// Parser utilities
void advance_parser(void);
int check(ember_token_type type);
int match(ember_token_type type);
void consume(ember_token_type type, const char* message);

#endif