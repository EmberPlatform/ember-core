#ifndef EMBER_LEXER_H
#define EMBER_LEXER_H

#include "ember.h"

// Lexer state
typedef struct {
    const char* start;
    const char* current;
    int line;
} lexer;

// Lexer functions
void init_scanner(const char* source);
ember_token scan_token(void);

// Scanner state management for string interpolation
lexer get_scanner_state(void);
void set_scanner_state(lexer state);

#endif // EMBER_LEXER_H