#include "compiler.h"
#include "scanner/scanner.h"
#include "value/value.h"
#include "object/object.h"
// added for free token
#include <stdlib.h>
// added for print token
#include <stdio.h>
// added for use va_list
#include <stdarg.h>
// addef for memcmp
#include <string.h>

static void init_parser(Scanner *scanner, Chunk *chunk, Parser *parser);
static void free_parser(Parser *parser); 
static void init_resolver(Resolver *resolver);
static void free_resolver(Resolver *resolver);
static void advance(Parser *parser);
static bool match (Parser *parser, int cnt, ...);
static void consume(TokenType type, const char *message, Parser *parser);
static bool check(Parser *parser, TokenType type);
static void parse_precedence(Precedence precedence, Compiler *compiler);
static void declarations(Compiler *compiler);
static void var_declaration(Compiler *compiler);
static void local_declaration(Compiler *compiler);
static void global_declaration(Compiler *complier);
static void declare_local(Compiler *compiler);
static bool token_equal(Token *a, Token *b);
static uint16_t declare_global(Parser *parser);
static void variable_initializer(Compiler *compiler);
static void define_local(Resolver *resolver);
static void define_global(Parser *parser, uint16_t idx);
static void statement(Compiler *compiler);
static void print_statement(Compiler *compiler);
static void block(Compiler *compiler);
static void begin_scope(Resolver *resolver);
static void end_scope(Compiler *compiler);
static void if_statement(Compiler *compiler);
static void while_statement(Compiler *compiler);
static void for_statement(Compiler *compiler);
static void expression_statement(Compiler *compiler);
static void expression(Compiler *compiler);
static void variable(Compiler *compiler, bool assign);
static int resolve_local(Compiler *compiler, Token* token);
static void grouping(Compiler *compiler, bool assign);
static void number(Compiler *compiler, bool assign);
static void unary(Compiler *compiler, bool assign);
static void binary(Compiler *parser, bool assign);
static void literal(Compiler *compiler, bool assign);
static void string(Compiler *compiler, bool assign);
static void and(Compiler *compiler, bool assign);
static void or(Compiler *compiler, bool assign);
static void xor(Compiler *compiler, bool assign);
static void emit_byte(uint8_t byte, Parser *parser);
static void emit_bytes(Parser *parser, int cnt, ...);
static void emit_return(Parser *parser);
static void emit_constant(Value value, Parser *parser);
static uint16_t make_constant(Value value, Parser *parser);
static int emit_jump(Parser *parser, uint8_t instruction);
static void emit_loop(Parser *parser, int start);
static void patch_jump(Parser *parser, int offset);
static void error_report(Token *token, const char *message, Parser *parser);
static void synchronize(Parser *parser);

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
    [CLOX_TOKEN_EQUAL]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_EQUAL_EQUAL]   = { NULL,     binary,  PREC_EQUALITY },
    [CLOX_TOKEN_GREATER]       = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_GREATER_EQUAL] = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS]          = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS_EQUAL]    = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_IDENTIFIER]    = { variable, NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_STRING]        = { string,   NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_NUMBER]        = { number,   NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_AND]           = { NULL,     and,     PREC_AND },
    [CLOX_TOKEN_CLASS]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_ELSE]          = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FALSE]         = { literal,  NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_FUN]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FOR]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_IF]            = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_NIL]           = { literal,  NULL,    PREC_PRIMARY },
    [CLOX_TOKEN_OR]            = { NULL,     or,      PREC_OR },
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
    [CLOX_TOKEN_XOR]           = { NULL,     xor,     PREC_XOR },
};

bool compile(const char *source, Chunk *chunk) {
    Parser parser;
    Scanner scanner;
    init_scanner(source, &scanner);
    init_parser(&scanner, chunk, &parser);
    Resolver resolver; 
    init_resolver(&resolver);
    Compiler compiler;
    compiler.parser = &parser;
    compiler.resolver = &resolver;
    
    advance(compiler.parser);
    while (!match(&parser, 1, CLOX_TOKEN_EOF)) {
        declarations(&compiler);
    }

    // just for represent expression result
    emit_return(&parser);
    free_scanner(&scanner);
    free_parser(&parser);
    free_resolver(&resolver);
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

static void init_resolver(Resolver *resolver) {
    resolver->local_count = 0;
    resolver->scope_depth = 0;
}

static void free_resolver(Resolver *resolver) {
    // nothing to do
    init_resolver(resolver);
}

static void advance(Parser *parser) {
    free(parser->previous);
    parser->previous = parser->current;
    for (;;) {
        parser->current = scan_token(parser->scanner);
        if (!check(parser, CLOX_TOKEN_ERROR)) break;
        error_report(parser->current, parser->current->lexeme, parser);
        free(parser->current);
    }
}

// check current token type
static bool match(Parser *parser, int cnt, ...) {
    va_list args;
    va_start(args, cnt);
    bool flag = false;
    for (int i = 0; i < cnt && !flag; i++) {
        if (check(parser, va_arg(args, TokenType))) {
            advance(parser);
            flag = true; 
        }
    }
    va_end(args);
    return flag;
}

static void consume(TokenType type, const char *message, Parser *parser) {
    if (check(parser, type)) advance(parser);
    else error_report(parser->current, message, parser);
}

static bool check(Parser *parser, TokenType type) {
    return parser->current->type == type;
}

static void declarations(Compiler *compiler) {
    if (match(compiler->parser, 1, CLOX_TOKEN_VAR)) var_declaration(compiler);
    else statement(compiler);
    if (compiler->parser->panic_mode) synchronize(compiler->parser);
}

static void var_declaration(Compiler *compiler) {
    consume(CLOX_TOKEN_IDENTIFIER, "Expect variable name.", compiler->parser);
    if (compiler->resolver->scope_depth > 0) local_declaration(compiler);
    else global_declaration(compiler);
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after variable declaration.", compiler->parser);
}

static void local_declaration(Compiler *compiler) {
    declare_local(compiler);
    variable_initializer(compiler);
    define_local(compiler->resolver);
}

static void global_declaration(Compiler *compiler) {
    uint16_t global_idx = declare_global(compiler->parser);
    variable_initializer(compiler);
    define_global(compiler->parser, global_idx);
}

static void declare_local(Compiler *compiler) {
    Token *identifier = compiler->parser->previous;
    if (compiler->resolver->local_count == MAX_STACK) error_report(identifier, "Too many local variables in function.", compiler->parser);
    else {
        for (int i = compiler->resolver->local_count - 1; i >= 0; i--) {
            Local *local = &compiler->resolver->locals[i];
            if (local->depth != -1 && local->depth < compiler->resolver->scope_depth) break;
            if (token_equal(identifier, &local->name)) error_report(identifier, "Already variable with this name in this scope.", compiler->parser);
        }
        Local *local = &compiler->resolver->locals[compiler->resolver->local_count++];
        local->name = *identifier;
        local->depth = -1;
    }
}

static bool token_equal(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->lexeme, b->lexeme, a->length) == 0;
}

static uint16_t declare_global(Parser *parser) {
    Token *identifier = parser->previous;
    return make_constant(OBJ_VALUE(new_string(identifier->lexeme, identifier->length)), parser);
}

static void variable_initializer(Compiler *compiler) {
    // initializer
    if (match(compiler->parser, 1, CLOX_TOKEN_EQUAL)) expression(compiler);
    else emit_byte(CLOX_OP_NIL, compiler->parser);
}

static void define_local(Resolver *resolver) {
    resolver->locals[resolver->local_count - 1].depth = resolver->scope_depth;
}

static void define_global(Parser *parser, uint16_t idx) {
    if (idx > UINT8_MAX) emit_bytes(parser, 3, CLOX_OP_DEFINE_GLOBAL_16, idx & 0xff, idx >> 8);
    else emit_bytes(parser, 2, CLOX_OP_DEFINE_GLOBAL, idx);
}

static void statement(Compiler *compiler) {
    if (match(compiler->parser, 1, CLOX_TOKEN_PRINT)) print_statement(compiler);
    else if (match(compiler->parser, 1, CLOX_TOKEN_LEFT_BRACE)) {
        begin_scope(compiler->resolver);
        block(compiler);
        end_scope(compiler);
    } else if (match(compiler->parser, 1, CLOX_TOKEN_IF)) if_statement(compiler);
    else if (match(compiler->parser, 1, CLOX_TOKEN_WHILE)) while_statement(compiler); 
    else if (match(compiler->parser, 1, CLOX_TOKEN_FOR)) for_statement(compiler);
    else expression_statement(compiler);
}

static void print_statement(Compiler *compiler) {
    expression(compiler);
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after value.", compiler->parser);
    emit_byte(CLOX_OP_PRINT, compiler->parser);
}

static void block(Compiler *compiler) {
    while (!check(compiler->parser, CLOX_TOKEN_RIGHT_BRACE) && !check(compiler->parser, CLOX_TOKEN_EOF)) declarations(compiler);
    consume(CLOX_TOKEN_RIGHT_BRACE, "Expect '}' after block.", compiler->parser);
}

static void begin_scope(Resolver *resolver) {
    resolver->scope_depth++;
}

static void end_scope(Compiler *compiler) {
    compiler->resolver->scope_depth--;
    int i = compiler->resolver->local_count - 1;
    for (; compiler->resolver->locals[i].depth > compiler->resolver->scope_depth && i >= 0; i--);
    int cnt = compiler->resolver->local_count - 1 - i;
    if (cnt > 0) emit_bytes(compiler->parser, 2, CLOX_OP_POP, compiler->resolver->local_count - 1 - i);
    compiler->resolver->local_count = i + 1;
}

static void if_statement(Compiler *compiler) {
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.", compiler->parser);
    // if condition 
    expression(compiler); 
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.", compiler->parser);

    int if_offset = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
    // pop condition expression on true
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    
    statement(compiler);
    int else_offset = emit_jump(compiler->parser, CLOX_OP_JUMP);
    patch_jump(compiler->parser, if_offset);
    // pop condition expression on false
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    
    if (match(compiler->parser, 1, CLOX_TOKEN_ELSE)) statement(compiler);

    patch_jump(compiler->parser, else_offset);
}

static void while_statement(Compiler *compiler) {
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'while'.", compiler->parser);
    int start = compiler->parser->chunk->count;
    expression(compiler);
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.", compiler->parser);
    int end_while = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
    // pop condition expression on true
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    statement(compiler);
    emit_loop(compiler->parser, start);
    patch_jump(compiler->parser, end_while);
    // pop condition expression on false
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
}

static void for_statement(Compiler *compiler) {
    // desugar 'for' into 'while', add a scope for initializer
    begin_scope(compiler->resolver);
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'for'.", compiler->parser);
    
    // initializer
    if (!match(compiler->parser, 1, CLOX_TOKEN_SEMICOLON)) {
        // variable declaration or expression (use statement to pop temporary value and consume ';')
        if (match(compiler->parser, 1, CLOX_TOKEN_VAR)) var_declaration(compiler);
        else expression_statement(compiler);
    }

    // condition
    int start = compiler->parser->chunk->count;
    int end_for = -1;
    if (!match(compiler->parser, 1, CLOX_TOKEN_SEMICOLON)) {
        expression(compiler);
        consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after loop condition.", compiler->parser);
        end_for = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
        // pop condition expression on true
        emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    }

    // increment
    if (!match(compiler->parser, 1, CLOX_TOKEN_RIGHT_PAREN)) {
        int body = emit_jump(compiler->parser, CLOX_OP_JUMP);
        int increment = compiler->parser->chunk->count;
        expression(compiler);
        consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.", compiler->parser);
        emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
        emit_loop(compiler->parser, start);
        start = increment;
        patch_jump(compiler->parser, body);
    }

    // body
    statement(compiler);
    emit_loop(compiler->parser, start);
    
    if (end_for != -1) {
        patch_jump(compiler->parser, end_for);
        // pop condition expression on false
        emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    }

    end_scope(compiler);
}

static void expression_statement(Compiler *compiler) {
    expression(compiler);
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after expression.", compiler->parser);
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
}

static void expression(Compiler *compiler) {
    parse_precedence(PREC_ASSIGNMENT, compiler);
}

static void parse_precedence(Precedence precedence, Compiler *compiler) {
    advance(compiler->parser);
    ParserRule *rule = &rules[compiler->parser->previous->type];
    parser_func prefix = rule->prefix;
    if (prefix == NULL) {
        error_report(compiler->parser->previous, "Expect expression.", compiler->parser);
        return;
    }
    bool assign = precedence <= PREC_ASSIGNMENT; 
    prefix(compiler, assign);
    while (precedence <= rules[compiler->parser->current->type].precedence) {
        advance(compiler->parser);
        parser_func infix = rules[compiler->parser->previous->type].infix;
        infix(compiler, assign);
    }
    if (assign && match(compiler->parser, 1, CLOX_TOKEN_EQUAL)) error_report(compiler->parser->previous, "Invalid assignment target.", compiler->parser);
}

static void variable(Compiler *compiler, bool assign) {
#define PARSE_VARIABLE(operation, idx, scope) do {\
        if ((idx) > UINT8_MAX) emit_bytes(compiler->parser, 3, CLOX_OP_##operation##_##scope##_16, (idx) & 0xff, (idx) >> 8);\
        else emit_bytes(compiler->parser, 2, CLOX_OP_##operation##_##scope, (idx));\
    } while(0);
    
    int local_idx = resolve_local(compiler, compiler->parser->previous);
    int global_idx = 0;
    if (local_idx == -1) global_idx = make_constant(OBJ_VALUE(new_string(compiler->parser->previous->lexeme, compiler->parser->previous->length)), compiler->parser); 
    if (assign && match(compiler->parser, 1, CLOX_TOKEN_EQUAL)) {
        expression(compiler);
        // set variables
        if (local_idx != -1) {
            // local set
            PARSE_VARIABLE(SET, local_idx, LOCAL);
        }
        else {
            // global set
            PARSE_VARIABLE(SET, global_idx, GLOBAL);
        }
    } else {
        // local get 
        if (local_idx != -1) {
            // local get
            PARSE_VARIABLE(GET, local_idx, LOCAL);
        }
        else {
            // global set
            PARSE_VARIABLE(GET, global_idx, GLOBAL);
        }
    }

#undef PARSE_VARIABLE
}

static int resolve_local(Compiler *compiler, Token* token) {
    for (int i = compiler->resolver->local_count - 1; i >= 0; i--) {
        Local *local = &compiler->resolver->locals[i];
        if (token_equal(token, &local->name)) {
            if (local->depth == -1) error_report(token, "Can't read local variable in its own initializer.", compiler->parser);
            return i;
        }
    }
    return -1;
}

static void grouping(Compiler *compiler, bool assign) {
    expression(compiler);
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.", compiler->parser);
}

static void number(Compiler *compiler, bool assign) {
    Value value = NUMBER_VALUE(strtod(compiler->parser->previous->lexeme, NULL));
    emit_constant(value, compiler->parser);
}

static void unary(Compiler *compiler, bool assign) {
    TokenType type = compiler->parser->previous->type;
    // right associate unary => precedence
    parse_precedence(PREC_UNARY, compiler);
    switch (type) {
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_NEGATE, compiler->parser);
            break;
        case CLOX_TOKEN_BANG:
            emit_byte(CLOX_OP_NOT, compiler->parser);
            break;
        default:
            // never reach here
            break;
    }
}

static void binary(Compiler *compiler, bool assign) {
    TokenType type = compiler->parser->previous->type;
    ParserRule *rule = &rules[type];
    // left associate binary => precedence + 1 
    parse_precedence((Precedence)(rule->precedence + 1), compiler);
    switch (type) {
        case CLOX_TOKEN_PLUS:
            emit_byte(CLOX_OP_ADD, compiler->parser);
            break;
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_SUBTRACT, compiler->parser);
            break;
        case CLOX_TOKEN_STAR:
            emit_byte(CLOX_OP_MULTIPLY, compiler->parser);
            break;
        case CLOX_TOKEN_SLASH:
            emit_byte(CLOX_OP_DIVIDE, compiler->parser);
            break;
        case CLOX_TOKEN_EQUAL_EQUAL:
            emit_byte(CLOX_OP_EQUAL, compiler->parser);
            break;
        case CLOX_TOKEN_BANG_EQUAL:
            emit_bytes(compiler->parser, 2, CLOX_OP_EQUAL, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_GREATER:
            emit_byte(CLOX_OP_GREATER, compiler->parser);
            break;
        case CLOX_TOKEN_GREATER_EQUAL:
            emit_bytes(compiler->parser, 2, CLOX_OP_LESS, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_LESS:
            emit_byte(CLOX_OP_LESS, compiler->parser);
            break;
        case CLOX_TOKEN_LESS_EQUAL:
            emit_bytes(compiler->parser, 2, CLOX_OP_GREATER, CLOX_OP_NOT);
            break;
        default:
            // never reach
            break;
    }
}

static void literal(Compiler *compiler, bool assign) {
    TokenType type = compiler->parser->previous->type;
    switch (type) {
        case CLOX_TOKEN_NIL:
            emit_byte(CLOX_OP_NIL, compiler->parser);
            break;
        case CLOX_TOKEN_TRUE:
            emit_byte(CLOX_OP_TRUE, compiler->parser);
            break;
        case CLOX_TOKEN_FALSE:
            emit_byte(CLOX_OP_FALSE, compiler->parser);
            break;
        default:
            // never reach
            break;
    }
}

static void string(Compiler *compiler, bool assign) {
    // remove double quote
    int length = compiler->parser->previous->length - 2;
    Value value = OBJ_VALUE(new_string(compiler->parser->previous->lexeme + 1, length));
    emit_constant(value, compiler->parser);
}

static void and(Compiler *compiler, bool assign) {
    int if_jump = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
    // pop on left operand is true
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    parse_precedence(PREC_AND, compiler);
    patch_jump(compiler->parser, if_jump);
}

static void or(Compiler *compiler, bool assign) {
    int else_jump = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(compiler->parser, CLOX_OP_JUMP);
    patch_jump(compiler->parser, else_jump);
    // pop on left operand is false
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    parse_precedence(PREC_OR, compiler);
    patch_jump(compiler->parser, end_jump);
}

static void xor(Compiler *compiler, bool assign) {
    parse_precedence(PREC_XOR, compiler);
    int if_jump = emit_jump(compiler->parser, CLOX_OP_JUMP_IF_FALSE);
    // pop on right operand is true
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    emit_byte(CLOX_OP_NOT, compiler->parser);
    int pop_jump = emit_jump(compiler->parser, CLOX_OP_JUMP);
    patch_jump(compiler->parser, if_jump);
    // pop on right operand is false
    emit_bytes(compiler->parser, 2, CLOX_OP_POP, 1);
    patch_jump(compiler->parser, pop_jump);
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

static void emit_constant(Value value, Parser *parser) {
    uint16_t idx = make_constant(value, parser);
    if (idx > UINT8_MAX) emit_bytes(parser, 3, CLOX_OP_CONSTANT_16, idx & 0xff, idx >> 8);
    else emit_bytes(parser, 2, CLOX_OP_CONSTANT, idx);
}

/** 
 * max size of constant pool is 65536 (0 ~ 65535)
 */
static uint16_t make_constant(Value value, Parser *parser) {
    int idx = append_constant(parser->chunk, value);
    if (idx > UINT16_MAX) error_report(parser->previous, "Too many constants in one chunk.", parser);
    return (uint16_t)idx;
}

static int emit_jump(Parser *parser, uint8_t instruction) {
    emit_bytes(parser, 3, instruction, 0xff, 0xff);
    return parser->chunk->count - 2;
}

// patch distance from current to @param: offset to @param:offset
// program at @param: offset, read 2 Bytes, may jump to current position
// -> offset       |
// -> offset + 1   |
//     ...         | ---- 
//     ...         |    | -> @return: jump (caled by current function)
// -> current      | ----
static void patch_jump(Parser *parser, int offset) {
    int jump = parser->chunk->count - offset - 2; 
    if (jump > UINT16_MAX) error_report(parser->previous, "Too much code to jump over.", parser);
    parser->chunk->code[offset] = jump & 0xff;
    parser->chunk->code[offset + 1] = (jump >> 8) & 0xff;
}

static void emit_loop(Parser *parser, int start) {
    int jump = parser->chunk->count - start + 3;
    emit_bytes(parser, 3, CLOX_OP_LOOP, jump & 0xff, (jump >> 8) & 0xff);
}

static void error_report(Token *token, const char *message, Parser *parser) {
    if (parser->panic_mode) return;
    parser->panic_mode = true;
    fprintf(stderr, "[line %2d column %2d Error]", token->location.line, token->location.column);
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

static void synchronize(Parser *parser) {
    parser->panic_mode = false;

    while (!check(parser, CLOX_TOKEN_EOF)) {
        if (parser->previous->type == CLOX_TOKEN_SEMICOLON) return;
        switch (parser->current->type) {
            case CLOX_TOKEN_CLASS:
            case CLOX_TOKEN_FUN:
            case CLOX_TOKEN_VAR:
            case CLOX_TOKEN_FOR:
            case CLOX_TOKEN_IF:
            case CLOX_TOKEN_WHILE:
            case CLOX_TOKEN_PRINT:
            case CLOX_TOKEN_RETURN: return;
            default: break;
        }
        advance(parser);
    }
}