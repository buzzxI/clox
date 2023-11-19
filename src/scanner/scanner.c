#include "scanner.h"
#include "common.h"
// added for malloc token
#include <stdlib.h>
// addef for string operation
#include <string.h>

static Token* create_token(TokenType type, Scanner *scanner);
static Token* error_token(const char *message, Scanner *scanner);
static Token* string_token(Scanner *scanner);
static Token* number_token(Scanner *scanner);
static Token* identifier_token(Scanner *scanner);
static bool is_end(Scanner *scanner);
static char advance(Scanner *scanner);
static char peek(int offset, Scanner *scanner);
static bool match(char c, Scanner *scanner);
static void skip_whitespace(Scanner *scanner);
static int skip_comment(Scanner *scanner);
static bool is_digit(char c);
static bool is_alpha(char c);
static bool is_alpha_numeric(char c);
static void add_keyword(const char *keyword, TokenType type, Scanner *scanner);
static TokenType check_keyword(Scanner *scanner);
static void free_trie(Trie *trie);

void init_scanner(const char *source, Scanner *scanner) {
    scanner->current = source;
    scanner->start = source;
    scanner->line = 1;
    scanner->column = 0;
    scanner->cur_column = 0;
    // initialize trie
    scanner->keywords = (Trie*)malloc(sizeof(Trie));
    memset(scanner->keywords->children, 0, sizeof(scanner->keywords->children));
    scanner->keywords->type = CLOX_TOKEN_ERROR;

    // add keywords
    add_keyword("and", CLOX_TOKEN_AND, scanner);
    add_keyword("class", CLOX_TOKEN_CLASS, scanner);
    add_keyword("else", CLOX_TOKEN_ELSE, scanner);
    add_keyword("false", CLOX_TOKEN_FALSE, scanner);
    add_keyword("for", CLOX_TOKEN_FOR, scanner);
    add_keyword("fun", CLOX_TOKEN_FUN, scanner);
    add_keyword("if", CLOX_TOKEN_IF, scanner);
    add_keyword("nil", CLOX_TOKEN_NIL, scanner);
    add_keyword("or", CLOX_TOKEN_OR, scanner);
    add_keyword("print", CLOX_TOKEN_PRINT, scanner);
    add_keyword("return", CLOX_TOKEN_RETURN, scanner);
    add_keyword("super", CLOX_TOKEN_SUPER, scanner);
    add_keyword("this", CLOX_TOKEN_THIS, scanner);
    add_keyword("true", CLOX_TOKEN_TRUE, scanner);
    add_keyword("var", CLOX_TOKEN_VAR, scanner);
    add_keyword("while", CLOX_TOKEN_WHILE, scanner);
}

void free_scanner(Scanner *scanner) {
    free_trie(scanner->keywords);
}

Token* scan_token(Scanner *scanner) {
    skip_whitespace(scanner);
    while (1) {
        int is_comment = skip_comment(scanner);
        if (is_comment < 0) return error_token("unterminated comment", scanner);
        if (is_comment == 0) break;
    }
    
    scanner->start = scanner->current;
    scanner->column = scanner->cur_column;

    if (is_end(scanner)) return create_token(CLOX_TOKEN_EOF, scanner);

    char c = advance(scanner);
    switch (c) {
        // single character
        case '(': return create_token(CLOX_TOKEN_LEFT_PAREN, scanner);
        case ')': return create_token(CLOX_TOKEN_RIGHT_PAREN, scanner);
        case '{': return create_token(CLOX_TOKEN_LEFT_BRACE, scanner);
        case '}': return create_token(CLOX_TOKEN_RIGHT_BRACE, scanner);
        case ',': return create_token(CLOX_TOKEN_COMMA, scanner);
        case '.': return create_token(CLOX_TOKEN_DOT, scanner);
        case '-': {
            if (match('-', scanner)) return create_token(CLOX_TOKEN_MINUS_MINUS, scanner);
            if (match('=', scanner)) return create_token(CLOX_TOKEN_MINUS_EQUAL, scanner);
            return create_token(CLOX_TOKEN_MINUS, scanner);
        }
        case '+': {
            if (match('+', scanner)) return create_token(CLOX_TOKEN_PLUS_PLUS, scanner);
            if (match('=', scanner)) return create_token(CLOX_TOKEN_PLUS_EQUAL, scanner);
            return create_token(CLOX_TOKEN_PLUS, scanner);
        }
        case ';': return create_token(CLOX_TOKEN_SEMICOLON, scanner);
        case '/': return create_token(match('=', scanner) ? CLOX_TOKEN_SLASH_EQUAL : CLOX_TOKEN_SLASH, scanner);
        case '*': {
            if (match('*', scanner)) return create_token(CLOX_TOKEN_STAR_STAR, scanner);
            if (match('=', scanner)) return create_token(CLOX_TOKEN_STAR_EQUAL, scanner);
            return create_token(CLOX_TOKEN_STAR, scanner);
        }
        case '%': return create_token(match('=', scanner) ? CLOX_TOKEN_PERCENT_EQUAL : CLOX_TOKEN_PERCENT, scanner);
        // single or double characters
        case '!': return create_token(match('=', scanner) ? CLOX_TOKEN_BANG_EQUAL : CLOX_TOKEN_BANG, scanner);
        case '=': return create_token(match('=', scanner) ? CLOX_TOKEN_EQUAL_EQUAL : CLOX_TOKEN_EQUAL, scanner);
        case '>': return create_token(match('=', scanner) ? CLOX_TOKEN_GREATER_EQUAL : CLOX_TOKEN_GREATER, scanner);
        case '<': return create_token(match('=', scanner) ? CLOX_TOKEN_LESS_EQUAL : CLOX_TOKEN_LESS, scanner);
        // literals: identifier, string, number
        case '"': return string_token(scanner);
        default:
            if (is_digit(c)) return number_token(scanner);
            if (is_alpha(c)) return identifier_token(scanner);
            return error_token("unexpected character", scanner);
    }
}

/**
 * create a token with @param type 
 */
static Token* create_token(TokenType type, Scanner *scanner) {
    Token *token = (Token*)malloc(sizeof(Token));
    token->type = type;
    token->lexeme = scanner->start;
    token->length = scanner->current - scanner->start;
    token->location.line = scanner->line;
    token->location.column = scanner->column;
    return token;
} 

/**
 * create a error typed token with @param message  
 */
static Token* error_token(const char *message, Scanner *scanner) {
    Token *token = (Token*)malloc(sizeof(Token));
    token->type = CLOX_TOKEN_ERROR;
    token->lexeme = message;
    token->length = strlen(message);
    token->location.line = scanner->line;
    token->location.column = scanner->column;
    return token;
}

static Token* string_token(Scanner *scanner) {
    char c;
    // enclosed '"' has already been consumed by advance
    while (!is_end(scanner) && (c = advance(scanner)) != '"') {
        // allow multi-line string
        if (c == '\n') {
            scanner->cur_column = 0;
            scanner->line++;
        }
    }
    // unterminated string
    if (c != '"') return error_token("unterminated string", scanner);
    return create_token(CLOX_TOKEN_STRING, scanner);
}

static Token* number_token(Scanner *scanner) {
    char c;
    while (!is_end(scanner) && is_digit(c = peek(0, scanner))) advance(scanner);

    if (c == '.') {
        // consume '.'
        advance(scanner);
        if (is_digit(peek(0, scanner))) {
            while (!is_end(scanner) && is_digit(c = peek(0, scanner))) advance(scanner);
        } else return error_token("'.' without tailing number", scanner);
    }
    return create_token(CLOX_TOKEN_NUMBER, scanner);
}

static Token* identifier_token(Scanner *scanner) {
    char c;
    while (!is_end(scanner) && is_alpha_numeric(c = peek(0, scanner))) advance(scanner);
    TokenType keyword_type = check_keyword(scanner);
    return create_token(keyword_type != CLOX_TOKEN_ERROR ? keyword_type : CLOX_TOKEN_IDENTIFIER, scanner);
}

/**
 * @brief check if the scanner has reached the end of the source code 
 */
static bool is_end(Scanner *scanner) {
    return *scanner->current == '\0';
}

/**
 * @brief advance the scanner to the next character 
 */
static char advance(Scanner *scanner) {
    scanner->current++;
    scanner->cur_column++;
    return scanner->current[-1];
}

/**
 * @brief peek character @param offset from current without advancing the scanner 
 */
static char peek(int offset, Scanner *scanner) {
    const char *c = scanner->current;
    for (int i = 0; i < offset && *c != '\0'; i++, c++); 
    return *c;
}

/**
 * @brief check if the current character matches @param c, if so, advance the scanner 
 */
static bool match(char c, Scanner *scanner) {
    if (is_end(scanner)) return false;
    if (*scanner->current != c) return false;
    scanner->current++;
    return true;
}

static void skip_whitespace(Scanner *scanner) {
    for (;;) {
        char c = peek(0, scanner);
        switch (c) {
            case '\n':
                scanner->line++;
                scanner->cur_column = 0;
                /* implicit fallthrough */
                __attribute__((fallthrough));
            case ' ':
            case '\r':
            case '\t':
                advance(scanner);
                break; 
            default:
                return;
        }
    }
}

static int skip_comment(Scanner *scanner) {
    char c = peek(0, scanner);
    if (c == '/') {
        switch (peek(1, scanner)) {
            case '/': {
                advance(scanner);
                advance(scanner);
                // a comment goes until the end of the line
                while (!is_end(scanner) && peek(0, scanner) != '\n') advance(scanner);
                if (!is_end(scanner)) {
                    // consume '\n'
                    advance(scanner);
                    scanner->line++;
                    scanner->cur_column = 0;
                }
                return 1;
            }
            case '*': {
                advance(scanner);
                advance(scanner);
                int flag = 0;
                for (char next = peek(0, scanner), next_next = peek(1, scanner); 
                    next != '\0' && next_next != '\0';
                    advance(scanner), next = peek(0, scanner), next_next = peek(1, scanner)) {
                        if (next == '*' && next_next == '/') {
                            flag = 1;
                            break;
                        }
                        if (next == '\n') {
                            scanner->line++;
                            scanner->cur_column = 0;
                        }
                } 
                if (flag) {
                    // flip to next => '*'
                    advance(scanner);
                    // flip to next_next => '/'
                    advance(scanner);
                    return 1;
                } else return -1;
            }
            default: break;
        }
    }
    return 0;
}

static bool is_digit(char c) {
    return c >= '0' && c <= '9';
}

static bool is_alpha(char c) {
    return (c >= 'a' && c <= 'z') || (c >= 'A' && c <= 'Z');
}

static bool is_alpha_numeric(char c) {
    return is_digit(c) || is_alpha(c) || c == '_';
}

static void add_keyword(const char *keyword, TokenType type, Scanner *scanner) {
    Trie *node = scanner->keywords;
    for (const char *c = keyword; *c != '\0'; c++) {
        int i = *c - 'a';
        if (node->children[i] == NULL) {
            node->children[i] = (Trie*)malloc(sizeof(Trie));
            memset(node->children[i]->children, 0, sizeof(node->children[i]->children));
            node->children[i]->type = CLOX_TOKEN_ERROR;
        }
        node = node->children[i];
    }
    node->type = type;
}

static void free_trie(Trie* trie) {
    if (trie == NULL) return;
    for (int i = 0; i < 26; i++) free_trie(trie->children[i]);
    free(trie);
}

static TokenType check_keyword(Scanner *scanner) {
    Trie *node = scanner->keywords;
    for (const char *c = scanner->start; c != scanner->current; c++) {
        int i = *c - 'a';
        if (i < 0 || i > 25 || node->children[i] == NULL) return CLOX_TOKEN_ERROR;
        node = node->children[i];
    }
    return node->type;
}