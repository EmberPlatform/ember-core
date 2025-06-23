#include "lexer.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static lexer scanner;

void init_scanner(const char* source) {
    scanner.start = source;
    scanner.current = source;
    scanner.line = 1;
}

static char advance() {
    return *scanner.current++;
}

static char peek_char() {
    return *scanner.current;
}

static char peek_next_char() {
    if (*scanner.current == '\0') return '\0';
    return scanner.current[1];
}

static int is_at_end() {
    return *scanner.current == '\0';
}

static void skip_whitespace() {
    for (;;) {
        char c = peek_char();
        switch (c) {
            case ' ':
            case '\r':
            case '\t':
                advance();
                break;
            case '#':
                // Skip comment until end of line
                while (peek_char() != '\n' && !is_at_end()) {
                    advance();
                }
                break;
            case '/':
                // Check for double-slash comment
                if (peek_next_char() == '/') {
                    // Skip // comment until end of line
                    advance(); // Skip first /
                    advance(); // Skip second /
                    while (peek_char() != '\n' && !is_at_end()) {
                        advance();
                    }
                } else {
                    return; // Not a comment, let the main tokenizer handle it
                }
                break;
            default:
                return;
        }
    }
}

static ember_token make_token(ember_token_type type) {
    ember_token token;
    token.type = type;
    token.start = scanner.start;
    token.length = (int)(scanner.current - scanner.start);
    token.line = scanner.line;
    token.number = 0.0;
    return token;
}

static ember_token error_token(const char* message) {
    ember_token token;
    token.type = TOKEN_ERROR;
    token.start = message;
    token.length = (int)strlen(message);
    token.line = scanner.line;
    token.number = 0.0;
    return token;
}

static ember_token number() {
    while (isdigit(peek_char())) advance();
    
    // Look for decimal point
    if (peek_char() == '.' && isdigit(peek_next_char())) {
        advance(); // consume '.'
        while (isdigit(peek_char())) advance();
    }
    
    ember_token token = make_token(TOKEN_NUMBER);
    token.number = strtod(scanner.start, NULL);
    return token;
}

static ember_token string() {
    int has_interpolation = 0;
    
    // Scan through the entire string first to check for interpolation
    const char* check_ptr = scanner.current;
    while (*check_ptr != '"' && *check_ptr != '\0') {
        if (*check_ptr == '$' && *(check_ptr + 1) == '{') {
            has_interpolation = 1;
            break;
        }
        check_ptr++;
    }
    
    // Now parse the string, handling interpolation properly
    while (peek_char() != '"' && !is_at_end()) {
        if (peek_char() == '\n') scanner.line++;
        
        // Handle interpolation expressions that may contain quotes
        if (peek_char() == '$' && peek_next_char() == '{') {
            advance(); // Skip $
            advance(); // Skip {
            
            int brace_count = 1;
            while (brace_count > 0 && !is_at_end()) {
                char c = peek_char();
                if (c == '"') {
                    // Skip quoted string inside interpolation
                    advance(); // Skip opening quote
                    while (peek_char() != '"' && !is_at_end()) {
                        if (peek_char() == '\\') advance(); // Skip escape sequences
                        advance();
                    }
                    if (peek_char() == '"') advance(); // Skip closing quote
                } else if (c == '{') {
                    brace_count++;
                    advance();
                } else if (c == '}') {
                    brace_count--;
                    advance();
                } else {
                    advance();
                }
            }
        } else {
            advance();
        }
    }
    
    if (is_at_end()) return error_token("Unterminated string");
    
    // The closing quote
    advance();
    
    if (has_interpolation) {
        return make_token(TOKEN_INTERPOLATED_STRING);
    }
    
    return make_token(TOKEN_STRING);
}

static ember_token_type identifier_type() {
    switch (scanner.start[0]) {
        case 'a':
            if (scanner.current - scanner.start == 3 && 
                memcmp(scanner.start, "and", 3) == 0) {
                return TOKEN_AND;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "async", 5) == 0) {
                return TOKEN_ASYNC;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "await", 5) == 0) {
                return TOKEN_AWAIT;
            }
            if (scanner.current - scanner.start == 2 && 
                memcmp(scanner.start, "as", 2) == 0) {
                return TOKEN_AS;
            }
            break;
        case 'b':
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "break", 5) == 0) {
                return TOKEN_BREAK;
            }
            break;
        case 'c':
            if (scanner.current - scanner.start == 8 && 
                memcmp(scanner.start, "continue", 8) == 0) {
                return TOKEN_CONTINUE;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "catch", 5) == 0) {
                return TOKEN_CATCH;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "class", 5) == 0) {
                return TOKEN_CLASS;
            }
            if (scanner.current - scanner.start == 4 && 
                memcmp(scanner.start, "case", 4) == 0) {
                return TOKEN_CASE;
            }
            break;
        case 'd':
            if (scanner.current - scanner.start == 2 && 
                memcmp(scanner.start, "do", 2) == 0) {
                return TOKEN_DO;
            }
            if (scanner.current - scanner.start == 7 && 
                memcmp(scanner.start, "default", 7) == 0) {
                return TOKEN_DEFAULT;
            }
            break;
        case 'e': 
            if (scanner.current - scanner.start == 4 && 
                memcmp(scanner.start, "else", 4) == 0) {
                return TOKEN_ELSE;
            }
            if (scanner.current - scanner.start == 7 && 
                memcmp(scanner.start, "extends", 7) == 0) {
                return TOKEN_EXTENDS;
            }
            if (scanner.current - scanner.start == 6 && 
                memcmp(scanner.start, "export", 6) == 0) {
                return TOKEN_EXPORT;
            }
            break;
        case 'f':
            if (scanner.current - scanner.start == 2 && 
                memcmp(scanner.start, "fn", 2) == 0) {
                return TOKEN_FN;
            }
            if (scanner.current - scanner.start == 3 && 
                memcmp(scanner.start, "for", 3) == 0) {
                return TOKEN_FOR;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "false", 5) == 0) {
                return TOKEN_FALSE;
            }
            if (scanner.current - scanner.start == 7 && 
                memcmp(scanner.start, "finally", 7) == 0) {
                return TOKEN_FINALLY;
            }
            if (scanner.current - scanner.start == 8 && 
                memcmp(scanner.start, "function", 8) == 0) {
                return TOKEN_FUNCTION;
            }
            if (scanner.current - scanner.start == 4 && 
                memcmp(scanner.start, "from", 4) == 0) {
                return TOKEN_FROM;
            }
            break;
        case 'i':
            if (scanner.current - scanner.start == 2 && 
                memcmp(scanner.start, "if", 2) == 0) {
                return TOKEN_IF;
            }
            if (scanner.current - scanner.start == 6 && 
                memcmp(scanner.start, "import", 6) == 0) {
                return TOKEN_IMPORT;
            }
            break;
        case 'n':
            if (scanner.current - scanner.start == 3 && 
                memcmp(scanner.start, "not", 3) == 0) {
                return TOKEN_NOT;
            }
            if (scanner.current - scanner.start == 3 && 
                memcmp(scanner.start, "new", 3) == 0) {
                return TOKEN_NEW;
            }
            break;
        case 'o':
            if (scanner.current - scanner.start == 2 && 
                memcmp(scanner.start, "or", 2) == 0) {
                return TOKEN_OR;
            }
            break;
        case 'r':
            if (scanner.current - scanner.start == 6 && 
                memcmp(scanner.start, "return", 6) == 0) {
                return TOKEN_RETURN;
            }
            if (scanner.current - scanner.start == 7 && 
                memcmp(scanner.start, "require", 7) == 0) {
                return TOKEN_REQUIRE;
            }
            break;
        case 's':
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "super", 5) == 0) {
                return TOKEN_SUPER;
            }
            if (scanner.current - scanner.start == 6 && 
                memcmp(scanner.start, "switch", 6) == 0) {
                return TOKEN_SWITCH;
            }
            break;
        case 't':
            if (scanner.current - scanner.start == 4 && 
                memcmp(scanner.start, "true", 4) == 0) {
                return TOKEN_TRUE;
            }
            if (scanner.current - scanner.start == 3 && 
                memcmp(scanner.start, "try", 3) == 0) {
                return TOKEN_TRY;
            }
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "throw", 5) == 0) {
                return TOKEN_THROW;
            }
            if (scanner.current - scanner.start == 4 && 
                memcmp(scanner.start, "this", 4) == 0) {
                return TOKEN_THIS;
            }
            break;
        case 'w':
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "while", 5) == 0) {
                return TOKEN_WHILE;
            }
            break;
        case 'y':
            if (scanner.current - scanner.start == 5 && 
                memcmp(scanner.start, "yield", 5) == 0) {
                return TOKEN_YIELD;
            }
            break;
    }
    return TOKEN_IDENTIFIER;
}

static ember_token identifier() {
    while (isalnum(peek_char()) || peek_char() == '_') advance();
    return make_token(identifier_type());
}

ember_token scan_token() {
    skip_whitespace();
    scanner.start = scanner.current;
    
    if (is_at_end()) return make_token(TOKEN_EOF);
    
    char c = advance();
    
    if (isdigit(c)) return number();
    if (isalpha(c) || c == '_') return identifier();
    
    switch (c) {
        case '(': return make_token(TOKEN_LPAREN);
        case ')': return make_token(TOKEN_RPAREN);
        case '{': return make_token(TOKEN_LBRACE);
        case '}': return make_token(TOKEN_RBRACE);
        case '[': return make_token(TOKEN_LBRACKET);
        case ']': return make_token(TOKEN_RBRACKET);
        case ',': return make_token(TOKEN_COMMA);
        case '+':
            if (peek_char() == '+') {
                advance();
                return make_token(TOKEN_PLUS_PLUS);
            }
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_PLUS_EQUAL);
            }
            return make_token(TOKEN_PLUS);
        case '-':
            if (peek_char() == '-') {
                advance();
                return make_token(TOKEN_MINUS_MINUS);
            }
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_MINUS_EQUAL);
            }
            return make_token(TOKEN_MINUS);
        case '*':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_MULTIPLY_EQUAL);
            }
            return make_token(TOKEN_MULTIPLY);
        case '/':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_DIVIDE_EQUAL);
            }
            return make_token(TOKEN_DIVIDE);
        case '%': return make_token(TOKEN_MODULO);
        case '"': return string();
        case '=':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_EQUAL_EQUAL);
            }
            return make_token(TOKEN_EQUAL);
        case '!':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_NOT_EQUAL);
            }
            return make_token(TOKEN_NOT);
        case '<':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_LESS_EQUAL);
            }
            return make_token(TOKEN_LESS);
        case '>':
            if (peek_char() == '=') {
                advance();
                return make_token(TOKEN_GREATER_EQUAL);
            }
            return make_token(TOKEN_GREATER);
        case ':': return make_token(TOKEN_COLON);
        case '@': return make_token(TOKEN_AT);
        case ';': return make_token(TOKEN_SEMICOLON);
        case '.': return make_token(TOKEN_DOT);
        case '&':
            if (peek_char() == '&') {
                advance();
                return make_token(TOKEN_AND_AND);
            }
            return error_token("Unexpected character '&'");
        case '|':
            if (peek_char() == '|') {
                advance();
                return make_token(TOKEN_OR_OR);
            }
            return error_token("Unexpected character '|'");
        case '\n':
            scanner.line++;
            return make_token(TOKEN_NEWLINE);
    }
    
    return error_token("Unexpected character");
}

// Scanner state management for string interpolation
lexer get_scanner_state(void) {
    return scanner;
}

void set_scanner_state(lexer state) {
    scanner = state;
}