#ifndef clox_scanner_h
#define clox_scanner_h

typedef enum {
    CLOX_TOKEN_ERROR, 
    /**
     * single-character token
     * '(', ')', '{', '}'，',', '.', '-', '+', ';', '*', '/'
     */ 
    CLOX_TOKEN_LEFT_PAREN, CLOX_TOKEN_RIGHT_PAREN, CLOX_TOKEN_LEFT_BRACE, CLOX_TOKEN_RIGHT_BRACE,
    CLOX_TOKEN_COMMA, CLOX_TOKEN_DOT, CLOX_TOKEN_MINUS, CLOX_TOKEN_PLUS, CLOX_TOKEN_SEMICOLON, CLOX_TOKEN_SLASH, CLOX_TOKEN_STAR,

    /**
     * single or double characters token
     * '!', '!=', '=', '==', '>', '>=', '<', '<='
     */
    CLOX_TOKEN_BANG, CLOX_TOKEN_BANG_EQUAL, CLOX_TOKEN_EQUAL, CLOX_TOKEN_EQUAL_EQUAL,
    CLOX_TOKEN_GREATER, CLOX_TOKEN_GREATER_EQUAL, CLOX_TOKEN_LESS, CLOX_TOKEN_LESS_EQUAL,

    /**
     * literals 
     */
    CLOX_TOKEN_IDENTIFIER, CLOX_TOKEN_STRING, CLOX_TOKEN_NUMBER,

    /**
     * keywords 
     */
    CLOX_TOKEN_AND, CLOX_TOKEN_CLASS, CLOX_TOKEN_ELSE, CLOX_TOKEN_FALSE, CLOX_TOKEN_FUN, CLOX_TOKEN_FOR, CLOX_TOKEN_IF, CLOX_TOKEN_NIL, CLOX_TOKEN_OR,
    CLOX_TOKEN_PRINT, CLOX_TOKEN_RETURN, CLOX_TOKEN_SUPER, CLOX_TOKEN_THIS, CLOX_TOKEN_TRUE, CLOX_TOKEN_VAR, CLOX_TOKEN_WHILE,
    CLOX_TOKEN_EOF,
} TokenType;

typedef struct {
    int line;
    int column;
} LocationInfo;

typedef struct {
    TokenType type;
    const char *lexeme;
    int length;
    LocationInfo location;
} Token;

typedef struct Trie {
    struct Trie *children[26];
    TokenType type;
} Trie;

typedef struct {
    const char *start;
    const char *current;
    int line;
    int cur_column;
    int column;
    Trie *keywords;
} Scanner;

void init_scanner(const char *source, Scanner *scanner);
void free_scanner(Scanner *scanner);
Token* scan_token(Scanner *scanner);

#endif