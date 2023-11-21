#include "compiler.h"
#include "scanner/scanner.h"
#include "value/value.h"
#include "object/object.h"
// added for free token
#include <stdlib.h>
// added for print token
#include <stdio.h>
#include <stdarg.h>

static void init_parser(Scanner *scanner, Chunk *chunk, Parser *parser);
static void free_parser(Parser *parser); 
static void advance(Parser *parser);
static bool match (Parser *parser, int cnt, ...);
static void consume(TokenType type, const char *message, Parser *parser);
static void parse_precedence(Precedence precedence, Parser *parser);
static void declarations(Parser *parser);
static void var_declaration(Parser *parser);
static void statement(Parser *parser);
static void print_statement(Parser *parser);
static void expression_statement(Parser *parser);
static void expression(Parser *parser);
static void grouping(Parser *parser);
static void number(Parser *parser);
static void unary(Parser *parser);
static void binary(Parser *parser);
static void literal(Parser *parser);
static void string(Parser *parser);
static void emit_byte(uint8_t byte, Parser *parser);
static void emit_bytes(Parser *parser, int cnt, ...);
static void emit_return(Parser *parser);
static void emit_constant(Value value, Parser *parser);
static void error_report(Token *token, const char *message, Parser *parser);

ParserRule rules[] = {
    [CLOX_TOKEN_ERROR]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_LEFT_PAREN]    = { grouping, NULL,    PREC_NONE },
    [CLOX_TOKEN_RIGHT_PAREN]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_LEFT_BRACE]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_RIGHT_BRACE]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_COMMA]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_DOT]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_MINUS]         = { unary,    binary,  PREC_TERM },
    [CLOX_TOKEN_PLUS]          = { NULL,     binary,  PREC_TERM },
    [CLOX_TOKEN_SEMICOLON]     = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SLASH]         = { NULL,     binary,  PREC_FACTOR },
    [CLOX_TOKEN_STAR]          = { NULL,     binary,  PREC_FACTOR },
    [CLOX_TOKEN_BANG]          = { unary,    NULL,    PREC_UNARY },
    [CLOX_TOKEN_BANG_EQUAL]    = { NULL,     binary,  PREC_EQUALITY },
    [CLOX_TOKEN_EQUAL]         = { NULL,     NULL,    PREC_ASSIGNMENT },
    [CLOX_TOKEN_EQUAL_EQUAL]   = { NULL,     binary,  PREC_EQUALITY },
    [CLOX_TOKEN_GREATER]       = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_GREATER_EQUAL] = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS]          = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS_EQUAL]    = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_IDENTIFIER]    = { NULL,     NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_STRING]        = { string,   NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_NUMBER]        = { number,   NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_AND]           = { NULL,     binary,  PREC_AND },
    [CLOX_TOKEN_CLASS]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_ELSE]          = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FALSE]         = { literal,  NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_FUN]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FOR]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_IF]            = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_NIL]           = { literal,  NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_OR]            = { NULL,     binary,  PREC_OR },
    [CLOX_TOKEN_PRINT]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_RETURN]        = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SUPER]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_THIS]          = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_TRUE]          = { literal,  NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_VAR]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_WHILE]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_EOF]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_STAR_STAR]     = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PERCENT]       = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PLUS_PLUS]     = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_MINUS_MINUS]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PLUS_EQUAL]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_MINUS_EQUAL]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_STAR_EQUAL]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SLASH_EQUAL]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PERCENT_EQUAL] = { NULL,     NULL,    PREC_NONE },
};

bool compile(const char *source, Chunk *chunk) {
    Parser parser;
    Scanner scanner;
    init_scanner(source, &scanner);
    init_parser(&scanner, chunk, &parser);
    advance(&parser);

    while (!match(&parser, 1, CLOX_TOKEN_EOF)) declarations(&parser);

    // just for represent expression result
    emit_return(&parser);
    free_scanner(&scanner);
    free_parser(&parser);
    return !parser.had_error;
}

static void init_parser(Scanner *scanner, Chunk *chunk, Parser *parser) {
    parser->previous = NULL;
    parser->current = NULL;
    parser->had_error = false;
    parser->panic_mode = false;
    
    parser->scanner = scanner;
    parser->chunk = chunk;
}

static void free_parser(Parser *parser) {
    free(parser->previous);
    free(parser->current);
}

static void advance(Parser *parser) {
    free(parser->previous);
    parser->previous = parser->current;
    for (;;) {
        parser->current = scan_token(parser->scanner);
        if (parser->current->type != CLOX_TOKEN_ERROR) break;
        error_report(parser->current, parser->current->lexeme, parser);
        free(parser->current);
    }
}

// check current token type
static bool match(Parser *parser, int cnt, ...) {
    va_list args;
    va_start(args, cnt);
    for (int i = 0; i < cnt; i++) {
        if (parser->current->type == va_arg(args, TokenType)) {
            advance(parser);
            va_end(args);
            return true;
        }
    }
    va_end(args);
    return false;
}

static void consume(TokenType type, const char *message, Parser *parser) {
    if (parser->current->type == type) advance(parser);
    else error_report(parser->current, message, parser);
}

static void parse_precedence(Precedence precedence, Parser *parser) {
    advance(parser);
    ParserRule *rule = &rules[parser->previous->type];
    parser_func prefix = rule->prefix;
    if (prefix == NULL) {
        error_report(parser->previous, "Expect expression.", parser);
        return;
    }
    prefix(parser);
    while (precedence <= rules[parser->current->type].precedence) {
        advance(parser);
        parser_func infix = rules[parser->previous->type].infix;
        infix(parser);
    }
}

static void declarations(Parser *parser) {
    if (match(parser, 1, CLOX_TOKEN_VAR)) var_declaration(parser);
    else statement(parser);
}

static void var_declaration(Parser *parser) {

    consume(CLOX_TOKEN_IDENTIFIER, "Expect variable name.", parser);
}

static void statement(Parser *parser) {
    if (match(parser, 1, CLOX_TOKEN_PRINT)) print_statement(parser);
    else expression_statement(parser);
}

static void print_statement(Parser *parser) {
    expression(parser);
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after value.", parser);
    emit_byte(CLOX_OP_PRINT, parser);
}

static void expression_statement(Parser *parser) {
    expression(parser);
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after expression.", parser);
}

static void expression(Parser *parser) {
    parse_precedence(PREC_ASSIGNMENT, parser);
}

static void grouping(Parser *parser) {
    expression(parser);
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.", parser);
}

static void number(Parser *parser) {
    Value value = NUMBER_VALUE(strtod(parser->previous->lexeme, NULL));
    emit_constant(value, parser);
}

static void unary(Parser *parser) {
    TokenType type = parser->previous->type;
    // right associate unary => precedence
    parse_precedence(PREC_UNARY, parser);
    switch (type) {
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_NEGATE, parser);
            break;
        case CLOX_TOKEN_BANG:
            emit_byte(CLOX_OP_NOT, parser);
            break;
        default:
            // never reach here
            break;
    }
}

static void binary(Parser *parser) {
    TokenType type = parser->previous->type;
    ParserRule *rule = &rules[type];
    // left associate binary => precedence + 1 
    parse_precedence((Precedence)(rule->precedence + 1), parser);
    switch (type) {
        case CLOX_TOKEN_PLUS:
            emit_byte(CLOX_OP_ADD, parser);
            break;
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_SUBTRACT, parser);
            break;
        case CLOX_TOKEN_STAR:
            emit_byte(CLOX_OP_MULTIPLY, parser);
            break;
        case CLOX_TOKEN_SLASH:
            emit_byte(CLOX_OP_DIVIDE, parser);
            break;
        case CLOX_TOKEN_EQUAL_EQUAL:
            emit_byte(CLOX_OP_EQUAL, parser);
            break;
        case CLOX_TOKEN_BANG_EQUAL:
            emit_bytes(parser, 2, CLOX_OP_EQUAL, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_GREATER:
            emit_byte(CLOX_OP_GREATER, parser);
            break;
        case CLOX_TOKEN_GREATER_EQUAL:
            emit_bytes(parser, 2, CLOX_OP_LESS, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_LESS:
            emit_byte(CLOX_OP_LESS, parser);
            break;
        case CLOX_TOKEN_LESS_EQUAL:
            emit_bytes(parser, 2, CLOX_OP_GREATER, CLOX_OP_NOT);
            break;
        default:
            // never reach
            break;
    }
}

static void literal(Parser *parser) {
    TokenType type = parser->previous->type;
    switch (type) {
        case CLOX_TOKEN_NIL:
            emit_byte(CLOX_OP_NIL, parser);
            break;
        case CLOX_TOKEN_TRUE:
            emit_byte(CLOX_OP_TRUE, parser);
            break;
        case CLOX_TOKEN_FALSE:
            emit_byte(CLOX_OP_FALSE, parser);
            break;
        default:
            // never reach
            break;
    }
}

static void string(Parser *parser) {
    // remove double quote
    int length = parser->previous->length - 2;
    Value value = OBJ_VALUE(new_string(parser->previous->lexeme + 1, length));
    emit_constant(value, parser);
}

static void emit_byte(uint8_t byte, Parser *parser) {
    write_chunk(parser->chunk, byte, parser->previous->location.line, parser->previous->location.column);
}

static void emit_bytes(Parser *parser, int cnt, ...) {
    va_list args;
    va_start(args, cnt);
    for (int i = 0; i < cnt; i++) emit_byte(va_arg(args, int), parser);
    va_end(args);
}

static void emit_return(Parser *parser) {
    emit_byte(CLOX_OP_RETURN, parser);
}

/**
 * OP_CONSTANT with 1 byte
 * OP_CONSTANT_16 with 2 bytes
 * max size of constant pool is 65536 (0 ~ 65535)
 */
static void emit_constant(Value value, Parser *parser) {
    int idx = append_constant(parser->chunk, value);
    if (idx <= UINT8_MAX) emit_bytes(parser, 2, CLOX_OP_CONSTANT, idx);
    else if (idx <= UINT16_MAX) emit_bytes(parser, 3, CLOX_OP_CONSTANT_16, idx >> 8, idx & 0xff);
    else error_report(parser->previous, "Too many constants in one chunk.", parser);
}

static void error_report(Token *token, const char *message, Parser *parser) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    fprintf(stderr, "[line %4d column %2d Error]", token->location.line, token->location.column);
    switch (token->type) {
        case CLOX_TOKEN_EOF:
            fprintf(stderr, " at end");
            break;
        case CLOX_TOKEN_ERROR:
            // nothing to do
            break;
        default:
            fprintf(stderr, " at '%.*s'", token->length, token->lexeme);
            break;
    }
    fprintf(stderr, " : %s\n", message);
    parser->had_error = true;
}