#include "compiler.h"
#include "scanner/scanner.h"
#include "chunk/chunk.h"
#include "object/object.h"
#include "memory/memory.h"
#ifdef CLOX_DEBUG_DISASSEMBLE
#include "disassemble/disassemble.h"
#endif // CLOX_DEBUG_DISASSEMBLE
// added for print token
#include <stdio.h>
// added for use va_list
#include <stdarg.h>
// added for memcmp
#include <string.h>
// added for strtod
#include <stdlib.h>

typedef enum {
    TYPE_SCRIPT,
    TYPE_FUNCTION,
    TYPE_METHOD,
    TYPE_INITIALIZER,
} FunctionType;

typedef enum {
    PREC_NONE,
    PREC_ASSIGNMENT,  // =
    PREC_XOR,         // xor
    PREC_OR,          // or
    PREC_AND,         // and
    PREC_EQUALITY,    // == !=
    PREC_COMPARISON,  // < > <= >=
    PREC_TERM,        // + -
    PREC_FACTOR,      // * / %
    PREC_UNARY,       // ! -
    PREC_EXPONENT,    // **
    PREC_CALL,        // . ()
    PREC_PRIMARY      // literal
} Precedence;

typedef struct {
    Token *previous;
    Token *current; 
    bool had_error;
    bool panic_mode;

    Scanner *scanner;
} Parser;

typedef struct {
    Token name;
    int depth;
    bool captured;
} Local;

typedef struct {
    uint16_t idx;
    bool is_local;
} UpValue;

typedef struct Resolver {
    struct Resolver *enclose;

    Local *locals;
    int local_cnt;
    int local_capacity;
    int scope_depth;

    UpValue *upvalues;
    int upvalues_capacity;

    FunctionObj *function;
    FunctionType type;
} Resolver;

typedef struct ClassResolver {
    struct ClassResolver *enclose;
    bool has_super;
} ClassResolver;

typedef void (*parser_func)(bool);

typedef struct {
    parser_func prefix;
    parser_func infix;
    Precedence precedence;
} ParserRule;

// there is only one parser in the whole program
Parser *parser;
// however, there may be multiple resolvers
Resolver *current_resolver = NULL;
ClassResolver *current_class = NULL;

static void init_parser(const char *source);
static void free_parser(); 
static void init_resolver(FunctionType type);
static FunctionObj* free_resolver();
static Chunk* current_chunk();
static void advance();
static bool match (TokenType type);
static void consume(TokenType type, const char *message);
static bool check(TokenType type);
static void parse_precedence(Precedence precedence);
static void declarations();
static void var_declaration();
static void local_declaration();
static void global_declaration();
static void declare_local();
static bool token_equal(Token *a, Token *b);
static void add_local(Token *identifier);
static uint16_t declare_global();
static void variable_initializer();
static void define_local();
static void define_global(uint16_t idx);
static void function_declaration();
static void function();
static void class_declaration();
static void method();
static void statement();
static void print_statement();
static void block();
static void begin_scope();
static void end_scope();
static void if_statement();
static void while_statement();
static void for_statement();
static void return_statement();
static void expression_statement();
static void expression();
static void variable(bool assign);
static void named_variable(Token *variable, bool assign);
static int resolve_local(Token *token, Resolver *resolver);
static int resolve_upvalue(Token *token, Resolver *resolver);
static int add_upvalue(int local, bool is_local, Resolver *resolver);
static void grouping(bool assign);
static void call(bool assign);
static void dot(bool assign);
static uint8_t argument_list();
static void number(bool assign);
static void unary(bool assign);
static void binary(bool assign);
static void literal(bool assign);
static void string(bool assign);
static void and(bool assign);
static void or(bool assign);
static void xor(bool assign);
static void this(bool assign);
static void super(bool assign);
static void emit_byte(uint8_t byte);
static void emit_bytes(int cnt, ...);
static void emit_nil_return();
static void emit_return();
static void emit_constant(Value value);
static uint16_t make_constant(Value value);
static uint16_t identifier_constant(Token* identifier);
static int emit_jump(uint8_t instruction);
static void emit_loop(int start);
static void patch_jump(int offset);
static void error_report(Token *token, const char *message);
static void synchronize();

const Token THIS_TOKEN = {.lexeme = "this", .length = 4};
const Token SUPER_TOKEN = {.lexeme = "super", .length = 5};

ParserRule rules[] = {
    [CLOX_TOKEN_ERROR]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_LEFT_PAREN]    = { grouping, call,    PREC_CALL },
    [CLOX_TOKEN_RIGHT_PAREN]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_LEFT_BRACE]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_RIGHT_BRACE]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_COMMA]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_DOT]           = { NULL,     dot,     PREC_CALL },
    [CLOX_TOKEN_MINUS]         = { unary,    binary,  PREC_TERM },
    [CLOX_TOKEN_PLUS]          = { NULL,     binary,  PREC_TERM },
    [CLOX_TOKEN_SEMICOLON]     = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SLASH]         = { NULL,     binary,  PREC_FACTOR },
    [CLOX_TOKEN_STAR]          = { NULL,     binary,  PREC_FACTOR },
    [CLOX_TOKEN_BANG]          = { unary,    NULL,    PREC_NONE },
    [CLOX_TOKEN_BANG_EQUAL]    = { NULL,     binary,  PREC_EQUALITY },
    [CLOX_TOKEN_EQUAL]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_EQUAL_EQUAL]   = { NULL,     binary,  PREC_EQUALITY },
    [CLOX_TOKEN_GREATER]       = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_GREATER_EQUAL] = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS]          = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_LESS_EQUAL]    = { NULL,     binary,  PREC_COMPARISON },
    [CLOX_TOKEN_IDENTIFIER]    = { variable, NULL,    PREC_NONE },
    [CLOX_TOKEN_STRING]        = { string,   NULL,    PREC_NONE },
    [CLOX_TOKEN_NUMBER]        = { number,   NULL,    PREC_NONE },
    [CLOX_TOKEN_AND]           = { NULL,     and,     PREC_AND },
    [CLOX_TOKEN_CLASS]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_ELSE]          = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FALSE]         = { literal,  NULL,    PREC_NONE },
    [CLOX_TOKEN_FUN]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_FOR]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_IF]            = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_NIL]           = { literal,  NULL,    PREC_NONE },
    [CLOX_TOKEN_OR]            = { NULL,     or,      PREC_OR },
    [CLOX_TOKEN_PRINT]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_RETURN]        = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SUPER]         = { super,    NULL,    PREC_NONE },
    [CLOX_TOKEN_THIS]          = { this,     NULL,    PREC_NONE },
    [CLOX_TOKEN_TRUE]          = { literal,  NULL,    PREC_NONE },
    [CLOX_TOKEN_VAR]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_WHILE]         = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_EOF]           = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PERCENT]       = { NULL,     binary,  PREC_FACTOR },
    [CLOX_TOKEN_STAR_STAR]     = { NULL,     binary,  PREC_EXPONENT },
    [CLOX_TOKEN_PLUS_PLUS]     = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_MINUS_MINUS]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PLUS_EQUAL]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_MINUS_EQUAL]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_STAR_EQUAL]    = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_SLASH_EQUAL]   = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_PERCENT_EQUAL] = { NULL,     NULL,    PREC_NONE },
    [CLOX_TOKEN_XOR]           = { NULL,     xor,     PREC_XOR },
};

FunctionObj* compile(const char *source) {
    init_parser(source);
    init_resolver(TYPE_SCRIPT);

    advance();
    while (!match(CLOX_TOKEN_EOF)) {
        declarations();
    }
    
    FunctionObj *rst = free_resolver(); 
    free_parser();
    return rst;
}

void mark_compiler_roots() {
    Resolver *resolver = current_resolver;
    // functions are roots
    while (resolver != NULL) {
        mark_obj((Obj*)resolver->function);
        resolver = resolver->enclose;
    }
}

static void init_parser(const char *source) {
    parser = ALLOCATE(Parser, 1);
    parser->previous = NULL;
    parser->current = NULL;
    parser->had_error = false;
    parser->panic_mode = false;
    parser->scanner = init_scanner(source);
}

static void free_parser() {
    FREE(Token, parser->previous);
    FREE(Token, parser->current);
    free_scanner(parser->scanner);
    parser->scanner = NULL;
    FREE(Parser, parser);
}

static void init_resolver(FunctionType type) {
    Resolver *resolver = ALLOCATE(Resolver, 1); 

    resolver->scope_depth = 0;

    // first local is reserved for implicit function (self)
    resolver->local_capacity = GROW_CAPACITY(0);
    resolver->locals = GROW_ARRAY(Local, NULL, 0, resolver->local_capacity);
    resolver->local_cnt = 0;
    Local *local = &resolver->locals[resolver->local_cnt++];
    local->depth = 0;
    // the first slot is this for methods and initializer
    if (type == TYPE_METHOD || type == TYPE_INITIALIZER) {
        local->name.lexeme = "this";
        local->name.length = 4;
    } else {
        local->name.lexeme = "";
        local->name.length = 0;
    }
    // slot 0 is reserved for implicit function (self) => not captured 
    local->captured = false;

    resolver->upvalues = NULL;
    resolver->upvalues_capacity = 0;

    resolver->function = new_function();
    // overwrite current resovler before new a function name
    resolver->enclose = current_resolver;
    current_resolver = resolver;

    if (type != TYPE_SCRIPT) {
        // function name
        resolver->function->name = new_string(parser->previous->lexeme, parser->previous->length);
    }
    resolver->type = type;
}

static FunctionObj* free_resolver() {
    emit_nil_return();
    
    Resolver *resolver = current_resolver;
    current_resolver = current_resolver->enclose;

    FunctionObj *function = resolver->function;

    bool error = parser->had_error;
#ifdef  CLOX_DEBUG_DISASSEMBLE 
    if (!error) disassemble_chunk(&function->chunk, function->name == NULL ? "clox script" : function->name->str);
#endif  // CLOX_DEBUG_DISASSEMBLE

    // resolve upvalues
    if (current_resolver != NULL) {
        // emit closure instruction
        uint16_t idx = make_constant(OBJ_VALUE(function));
        if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_CLOSURE_16, idx & 0xff, idx >> 8);
        else emit_bytes(2, CLOX_OP_CLOSURE, idx);

        for (int i = 0; i < function->upvalue_cnt; i++) {
            emit_byte(resolver->upvalues[i].is_local ? 1 : 0);
            emit_bytes(2, resolver->upvalues[i].idx & 0xff, resolver->upvalues[i].idx >> 8);
        }
    }

    FREE(UpValue, resolver->upvalues);
    FREE(Local, resolver->locals);
    FREE(Resolver, resolver);
    return error ? NULL : function;
}

static Chunk* current_chunk() {
    return &current_resolver->function->chunk;
}

static void advance() {
    FREE(Token, parser->previous);
    parser->previous = parser->current;
    for (;;) {
        parser->current = scan_token(parser->scanner);
        if (!check(CLOX_TOKEN_ERROR)) break;
        error_report(parser->current, parser->current->lexeme);
        FREE(Token, parser->current);
    }
}

// check current token type
static bool match(TokenType type) {
    if (check(type)) {
        advance();
        return true;
    }
    return false;
}

static void consume(TokenType type, const char *message) {
    if (!match(type)) error_report(parser->current, message); 
}

static bool check(TokenType type) {
    return parser->current->type == type;
}

static void declarations() {
    if (match(CLOX_TOKEN_VAR)) var_declaration();
    else if(match(CLOX_TOKEN_FUN)) function_declaration();
    else if (match(CLOX_TOKEN_CLASS)) class_declaration();
    else statement();
    if (parser->panic_mode) synchronize();
}

static void var_declaration() {
    consume(CLOX_TOKEN_IDENTIFIER, "Expect variable name.");
    if (current_resolver->scope_depth > 0) local_declaration();
    else global_declaration();
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after variable declaration.");
}

static void local_declaration() {
    declare_local();
    variable_initializer();
    define_local();
}

static void global_declaration() {
    uint16_t global_idx = declare_global();
    variable_initializer();
    define_global(global_idx);
}

static void declare_local() {
    Token *identifier = parser->previous;
    for (int i = current_resolver->local_cnt - 1; i >= 0; i--) {
        Local *local = &current_resolver->locals[i];
        if (local->depth != -1 && local->depth < current_resolver->scope_depth) break;
        if (token_equal(identifier, &local->name)) error_report(identifier, "Already variable with this name in this scope.");
    }

    add_local(identifier); 
}

static bool token_equal(Token *a, Token *b) {
    if (a->length != b->length) return false;
    return memcmp(a->lexeme, b->lexeme, a->length) == 0;
}

static void add_local(Token *identifier) {
    // dynamic allocate slot
    if (current_resolver->local_cnt + 1 > current_resolver->local_capacity) {
        int old_capcaity = current_resolver->local_capacity;
        current_resolver->local_capacity = GROW_CAPACITY(old_capcaity);
        current_resolver->locals = GROW_ARRAY(Local, current_resolver->locals, old_capcaity, current_resolver->local_capacity);
    }

    Local *local = &current_resolver->locals[current_resolver->local_cnt++];
    local->name = *identifier;
    local->depth = current_resolver->scope_depth;
    // by default all variables are not captured
    local->captured = false;
}

static uint16_t declare_global() {
    return identifier_constant(parser->previous);
}

static void variable_initializer() {
    // initializer
    if (match(CLOX_TOKEN_EQUAL)) expression();
    else emit_byte(CLOX_OP_NIL);
}

static void define_local() {
    current_resolver->locals[current_resolver->local_cnt - 1].depth = current_resolver->scope_depth;
}

static void define_global(uint16_t idx) {
    if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_DEFINE_GLOBAL_16, idx & 0xff, idx >> 8);
    else emit_bytes(2, CLOX_OP_DEFINE_GLOBAL, idx);
}

static void function_declaration() {
    consume(CLOX_TOKEN_IDENTIFIER, "Expect function name.");
    // declare and define function before compile body
    if (current_resolver->scope_depth > 0) {
        declare_local();
        define_local();
        function(TYPE_FUNCTION);
    } else {
        uint16_t idx = declare_global();
        function(TYPE_FUNCTION);
        define_global(idx);
    }
}

static void function(FunctionType type) {
    init_resolver(type);
    begin_scope();
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after function name.");
    if (!check(CLOX_TOKEN_RIGHT_PAREN)) {
        do {
            current_resolver->function->arity++;
            // limit function paratemer count
            if (current_resolver->function->arity == 256) error_report(parser->current, "Can't have more than 255 parameters.");
            consume(CLOX_TOKEN_IDENTIFIER, "Expect parameter name.");
            declare_local();
            define_local();
        } while (match(CLOX_TOKEN_COMMA));
    }
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after function parameters list.");
    consume(CLOX_TOKEN_LEFT_BRACE, "Expect '{' before function body.");

    block();
    // optional
    end_scope();

    free_resolver(); 
}

static void class_declaration() {
    consume(CLOX_TOKEN_IDENTIFIER, "Expect class name.");
    Token class_name = *parser->previous;
    // append class name into constant pool (no matter it is local or global)
    uint16_t idx = identifier_constant(&class_name);
    // class name
    if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_CLASS_16, idx & 0xff, idx >> 8);
    else emit_bytes(2, CLOX_OP_CLASS, idx);
    
    // if class is local -> klass remain in stack (referenced as local variable)
    // if class is global -> klass poped by define_global
    if (current_resolver->scope_depth) {
        declare_local();
        define_local();
    } else {
        // define class as global
        define_global(idx);
    }

    ClassResolver class;
    class.enclose = current_class;
    class.has_super = false;
    current_class = &class;

    if (match(CLOX_TOKEN_LESS)) {
        consume(CLOX_TOKEN_IDENTIFIER, "Expect superclass name.");
        if (token_equal(&class_name, parser->previous)) error_report(parser->previous, "A class can't inherit from itself.");

        // load superclass from local/upvalue/global to stack
        variable(false);
        // leave supclass on stack and use "super" to reference
        begin_scope();
        add_local(&SUPER_TOKEN);
        define_local();
        
        // load current class from local/upvalue/global to stack
        named_variable(&class_name, false);

        emit_byte(CLOX_OP_INHERIT);
        current_class->has_super = true;
    }

    consume(CLOX_TOKEN_LEFT_BRACE, "Expect '{' before class body.");

    // reload klass to bind method
    named_variable(&class_name, false);

    // methods
    while (!check(CLOX_TOKEN_EOF) && !check(CLOX_TOKEN_RIGHT_BRACE)) method();

    consume(CLOX_TOKEN_RIGHT_BRACE, "Expect '}' after class body.");

    // pop klass out of stack
    emit_byte(CLOX_OP_POP);

    if (current_class->has_super) end_scope();
    current_class = current_class->enclose;
}

static void method() {
    // method name
    consume(CLOX_TOKEN_IDENTIFIER, "Expect method name.");
    uint16_t identifier = make_constant(OBJ_VALUE(new_string(parser->previous->lexeme, parser->previous->length)));

    // initializer
    if (parser->previous->length == 4 && memcmp("init", parser->previous->lexeme, parser->previous->length) == 0) function(TYPE_INITIALIZER);
    // normal body
    else function(TYPE_METHOD);
    
    if (identifier > UINT8_MAX) emit_bytes(3, CLOX_OP_METHOD_16, identifier & 0xff, identifier >> 8);
    else emit_bytes(2, CLOX_OP_METHOD, identifier);
}

static void statement() {
    if (match(CLOX_TOKEN_PRINT)) print_statement();
    else if (match(CLOX_TOKEN_LEFT_BRACE)) {
        begin_scope(current_resolver);
        block();
        end_scope();
    } else if (match(CLOX_TOKEN_IF)) if_statement();
    else if (match(CLOX_TOKEN_WHILE)) while_statement(); 
    else if (match(CLOX_TOKEN_FOR)) for_statement();
    else if (match(CLOX_TOKEN_RETURN)) return_statement();
    else expression_statement();
}

static void print_statement() {
    expression();
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after value.");
    emit_byte(CLOX_OP_PRINT);
}

static void block() {
    while (!check(CLOX_TOKEN_RIGHT_BRACE) && !check(CLOX_TOKEN_EOF)) declarations();
    consume(CLOX_TOKEN_RIGHT_BRACE, "Expect '}' after block.");
}

static void begin_scope() {
    current_resolver->scope_depth++;
}

static void end_scope() {
    current_resolver->scope_depth--;
    int i = current_resolver->local_cnt - 1;
    for (int i = current_resolver->local_cnt - 1; i >= 0; i--) {
        Local *local = &current_resolver->locals[i];
        if (local->depth <= current_resolver->scope_depth) break;
        // pop upvalue
        if (local->captured) emit_byte(CLOX_OP_CLOSE_UPVALUE);
        // pop local variable
        else emit_byte(CLOX_OP_POP);
    }
    current_resolver->local_cnt = i + 1;
}

static void if_statement() {
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'if'.");
    // if condition 
    expression(); 
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");

    int if_offset = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    // pop condition expression on true
    emit_byte(CLOX_OP_POP);
    
    statement();
    int else_offset = emit_jump(CLOX_OP_JUMP);
    patch_jump(if_offset);
    // pop condition expression on false
    emit_byte(CLOX_OP_POP);
    
    if (match(CLOX_TOKEN_ELSE)) statement();

    patch_jump(else_offset);
}

static void while_statement() {
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'while'.");
    int start = current_chunk()->count;
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after condition.");
    int end_while = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    // pop condition expression on true
    emit_byte(CLOX_OP_POP);
    statement();
    emit_loop(start);
    patch_jump(end_while);
    // pop condition expression on false
    emit_byte(CLOX_OP_POP);
}

static void for_statement() {
    // desugar 'for' into 'while', add a scope for initializer
    begin_scope();
    consume(CLOX_TOKEN_LEFT_PAREN, "Expect '(' after 'for'.");
    
    // initializer
    if (!match(CLOX_TOKEN_SEMICOLON)) {
        // variable declaration or expression (use statement to pop temporary value and consume ';')
        if (match(CLOX_TOKEN_VAR)) var_declaration();
        else expression_statement();
    }

    // condition
    int start = current_chunk()->count;
    int end_for = -1;
    if (!match(CLOX_TOKEN_SEMICOLON)) {
        expression();
        consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after loop condition.");
        end_for = emit_jump(CLOX_OP_JUMP_IF_FALSE);
        // pop condition expression on true
        emit_byte(CLOX_OP_POP);
    }

    // increment
    if (!match(CLOX_TOKEN_RIGHT_PAREN)) {
        int body = emit_jump(CLOX_OP_JUMP);
        int increment = current_chunk()->count;
        expression();
        consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after for clauses.");
        emit_byte(CLOX_OP_POP);
        emit_loop(start);
        start = increment;
        patch_jump(body);
    }

    // body
    statement();
    emit_loop(start);
    
    if (end_for != -1) {
        patch_jump(end_for);
        // pop condition expression on false
        emit_byte(CLOX_OP_POP);
    }

    end_scope();
}

static void return_statement() {
    if (current_resolver->type == TYPE_SCRIPT) error_report(parser->previous, "Can't return from top-level code.");
    else {
        if (match(CLOX_TOKEN_SEMICOLON)) emit_nil_return();
        else {
            if (current_resolver->type == TYPE_INITIALIZER) error_report(parser->previous, "Can't return a value from an initializer.");
            else {
                expression();        
                consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after return statement");
                emit_return();
            }
        }
    }
}

static void expression_statement() {
    expression();
    consume(CLOX_TOKEN_SEMICOLON, "Expect ';' after expression.");
    emit_byte(CLOX_OP_POP);
}

static void expression() {
    parse_precedence(PREC_ASSIGNMENT);
}

static void parse_precedence(Precedence precedence) {
    advance();
    ParserRule *rule = &rules[parser->previous->type];
    parser_func prefix = rule->prefix;
    if (prefix == NULL) {
        error_report(parser->previous, "Expect expression.");
        return;
    }
    bool assign = precedence <= PREC_ASSIGNMENT; 
    prefix(assign);
    while (precedence <= rules[parser->current->type].precedence) {
        advance();
        parser_func infix = rules[parser->previous->type].infix;
        infix(assign);
    }
    if (assign && match(CLOX_TOKEN_EQUAL)) error_report(parser->previous, "Invalid assignment target.");
}

static void variable(bool assign) {
    named_variable(parser->previous, assign);
}

static void named_variable(Token *variable, bool assign) {
#define PARSE_VARIABLE(operation, idx, scope) do {\
        if ((idx) > UINT8_MAX) emit_bytes(3, CLOX_OP_##operation##_##scope##_16, (idx) & 0xff, (idx) >> 8);\
        else emit_bytes(2, CLOX_OP_##operation##_##scope, (idx));\
    } while(0);
    
    int local_idx = resolve_local(variable, current_resolver);
    int upvalue_idx = -1;
    int global_idx = -1;
    if (local_idx == -1) {
        upvalue_idx = resolve_upvalue(variable, current_resolver);
        if (upvalue_idx == -1) global_idx = make_constant(OBJ_VALUE(new_string(variable->lexeme, variable->length)));
    }
    if (assign && match(CLOX_TOKEN_EQUAL)) {
        expression();
        // set variables
        if (local_idx != -1) {
            // local set
            PARSE_VARIABLE(SET, local_idx, LOCAL);
        } else if (upvalue_idx != -1) {
            // upvalue set
            PARSE_VARIABLE(SET, upvalue_idx, UPVALUE);
        } else {
            // global set
            PARSE_VARIABLE(SET, global_idx, GLOBAL);
        }
    } else {
        // local get 
        if (local_idx != -1) {
            // local get
            PARSE_VARIABLE(GET, local_idx, LOCAL);
        } else if (upvalue_idx != -1) {
            // upvalue get
            PARSE_VARIABLE(GET, upvalue_idx, UPVALUE);
        } else {
            // global set
            PARSE_VARIABLE(GET, global_idx, GLOBAL);
        }
    }
#undef PARSE_VARIABLE
}

static int resolve_local(Token *token, Resolver *resolver) {
    for (int i = resolver->local_cnt - 1; i >= 0; i--) {
        Local *local = &resolver->locals[i];
        if (token_equal(token, &local->name)) {
            if (local->depth == -1) error_report(token, "Can't read local variable in its own initializer.");
            return i;
        }
    }
    return -1;
}

static int resolve_upvalue(Token *token, Resolver *resolver) {
    if (resolver->enclose == NULL) return -1;

    int local = resolve_local(token, resolver->enclose);
    if (local != -1) {
        // inner function upvalue a local variable => capture it
        resolver->enclose->locals[local].captured = true;
        return add_upvalue(local, true, resolver);
    } 
    
    int upvalue = resolve_upvalue(token, resolver->enclose);
    if (upvalue != -1) return add_upvalue(upvalue, false, resolver);
    
    return -1;
}

static int add_upvalue(int local, bool is_local, Resolver *resolver) {
    int upvalue_cnt = resolver->function->upvalue_cnt;
    for (int i = 0; i < upvalue_cnt; i++) {
        UpValue *upvalue = &resolver->upvalues[i];
        if (upvalue->idx == local && upvalue->is_local == is_local) return i;
    }

    if (upvalue_cnt + 1 > resolver->upvalues_capacity) {
        int old_capacity = resolver->upvalues_capacity;
        resolver->upvalues_capacity = GROW_CAPACITY(old_capacity);
        resolver->upvalues = GROW_ARRAY(UpValue, resolver->upvalues, old_capacity, resolver->upvalues_capacity);
    }

    UpValue *upvalue = &resolver->upvalues[upvalue_cnt];
    upvalue->idx = local;
    upvalue->is_local = is_local;
    return resolver->function->upvalue_cnt++;
}

static void grouping(bool assign) {
    expression();
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after expression.");
}

static void call(bool assign) {
    uint8_t arg_cnt = argument_list();
    emit_bytes(2, CLOX_OP_CALL, arg_cnt);
}

static void dot(bool assign) {
    consume(CLOX_TOKEN_IDENTIFIER, "Expect property name after '.'.");
    // append previous token into constant pool
    uint16_t idx = identifier_constant(parser->previous); 
    if (assign && match(CLOX_TOKEN_EQUAL)) {
        expression();
        // set property
        if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_SET_PROPERTY_16, idx & 0xff, idx >> 8);
        else emit_bytes(2, CLOX_OP_SET_PROPERTY, idx);
    } else {
        if (match(CLOX_TOKEN_LEFT_PAREN)) {
            // method call
            uint8_t arg_cnt = argument_list();
            if (idx > UINT8_MAX) emit_bytes(4, CLOX_OP_INVOKE_16, idx & 0xff, idx >> 8, arg_cnt);
            else emit_bytes(3, CLOX_OP_INVOKE, idx, arg_cnt);
        } else {
            // get property
            if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_GET_PROPERTY_16, idx & 0xff, idx >> 8);
            else emit_bytes(2, CLOX_OP_GET_PROPERTY, idx);
        }
    }
}

static uint8_t argument_list() {
    int arg_cnt = 0;
    if (!check(CLOX_TOKEN_RIGHT_PAREN)) {
        do {
            // add an argument into stack
            expression();
            if (arg_cnt == 256) error_report(parser->current, "Can't have more than 255 arguments.");
            arg_cnt++;
        } while (match(CLOX_TOKEN_COMMA));
    }
    consume(CLOX_TOKEN_RIGHT_PAREN, "Expect ')' after arguments.");
    return (uint8_t)arg_cnt;
}

static void number(bool assign) {
    Value value = NUMBER_VALUE(strtod(parser->previous->lexeme, NULL));
    emit_constant(value);
}

static void unary(bool assign) {
    TokenType type = parser->previous->type;
    // right associate unary => precedence
    parse_precedence(PREC_UNARY);
    switch (type) {
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_NEGATE);
            break;
        case CLOX_TOKEN_BANG:
            emit_byte(CLOX_OP_NOT);
            break;
        default:
            // never reach here
            break;
    }
}

static void binary(bool assign) {
    TokenType type = parser->previous->type;
    ParserRule *rule = &rules[type];
    // left associate binary => precedence + 1 
    parse_precedence((Precedence)(rule->precedence + 1));
    switch (type) {
        case CLOX_TOKEN_PLUS:
            emit_byte(CLOX_OP_ADD);
            break;
        case CLOX_TOKEN_MINUS:
            emit_byte(CLOX_OP_SUBTRACT);
            break;
        case CLOX_TOKEN_STAR:
            emit_byte(CLOX_OP_MULTIPLY);
            break;
        case CLOX_TOKEN_SLASH:
            emit_byte(CLOX_OP_DIVIDE);
            break;
        case CLOX_TOKEN_PERCENT:
            emit_byte(CLOX_OP_MODULO);
            break;
        case CLOX_TOKEN_STAR_STAR:
            emit_byte(CLOX_OP_POWER);
            break;
        case CLOX_TOKEN_EQUAL_EQUAL:
            emit_byte(CLOX_OP_EQUAL);
            break;
        case CLOX_TOKEN_BANG_EQUAL:
            emit_bytes(2, CLOX_OP_EQUAL, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_GREATER:
            emit_byte(CLOX_OP_GREATER);
            break;
        case CLOX_TOKEN_GREATER_EQUAL:
            emit_bytes(2, CLOX_OP_LESS, CLOX_OP_NOT);
            break;
        case CLOX_TOKEN_LESS:
            emit_byte(CLOX_OP_LESS);
            break;
        case CLOX_TOKEN_LESS_EQUAL:
            emit_bytes(2, CLOX_OP_GREATER, CLOX_OP_NOT);
            break;
        default:
            // never reach
            break;
    }
}

static void literal(bool assign) {
    TokenType type = parser->previous->type;
    switch (type) {
        case CLOX_TOKEN_NIL:
            emit_byte(CLOX_OP_NIL);
            break;
        case CLOX_TOKEN_TRUE:
            emit_byte(CLOX_OP_TRUE);
            break;
        case CLOX_TOKEN_FALSE:
            emit_byte(CLOX_OP_FALSE);
            break;
        default:
            // never reach
            break;
    }
}

static void string(bool assign) {
    // remove double quote
    int length = parser->previous->length - 2;
    Value value = OBJ_VALUE(new_string(parser->previous->lexeme + 1, length));
    emit_constant(value);
}

static void and(bool assign) {
    int if_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    // pop on left operand is true
    emit_byte(CLOX_OP_POP);
    parse_precedence(PREC_AND);
    patch_jump(if_jump);
}

static void or(bool assign) {
    int else_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    int end_jump = emit_jump(CLOX_OP_JUMP);
    patch_jump(else_jump);
    // pop on left operand is false
    emit_byte(CLOX_OP_POP);
    parse_precedence(PREC_OR);
    patch_jump(end_jump);
}

static void xor(bool assign) {
    parse_precedence(PREC_XOR);
    int if_jump = emit_jump(CLOX_OP_JUMP_IF_FALSE);
    // pop on right operand is true
    emit_byte(CLOX_OP_POP);
    emit_byte(CLOX_OP_NOT);
    int pop_jump = emit_jump(CLOX_OP_JUMP);
    patch_jump(if_jump);
    // pop on right operand is false
    emit_byte(CLOX_OP_POP);
    patch_jump(pop_jump);
}

// just treat this as a variable, let @function: variable handles all parsing
static void this(bool assign) {
    if (current_class == NULL) {
        error_report(parser->previous, "Can't use 'this' outside of a class.");
        return;
    }
    // load current instance into stack
    // this cannot be reassigned to any other values
    variable(false);
}

static void super(bool assign) {
    if (current_class == NULL) {
        error_report(parser->previous, "Can't use 'super' outside of a class.");
        return;
    } else if (!current_class->has_super) {
        error_report(parser->previous, "Can't use 'super' in a class with no superclass.");
        return;
    }
    consume(CLOX_TOKEN_DOT, "Expect '.' after 'super'.");
    consume(CLOX_TOKEN_IDENTIFIER, "Expect superclass method name.");
    // append property into constanct pool
    uint16_t idx = identifier_constant(parser->previous);
    // load current instance
    named_variable(&THIS_TOKEN, false);
    
    if (match(CLOX_TOKEN_LEFT_PAREN)) {
        uint8_t arg_cnt = argument_list();
        // load super klass
        named_variable(&SUPER_TOKEN, false);
        if (idx > UINT8_MAX) emit_bytes(4, CLOX_OP_INVOKE_SUPER_16, idx & 0xff, idx >> 8, arg_cnt);
        else emit_bytes(3, CLOX_OP_INVOKE_SUPER, idx, arg_cnt);
    } else {
        // load super klass
        named_variable(&SUPER_TOKEN, false);
        if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_GET_SUPER_16, idx & 0xff, idx >> 8);
        else emit_bytes(2, CLOX_OP_GET_SUPER, idx);
    }
}

static void emit_byte(uint8_t byte) {
    write_chunk(current_chunk(), byte, parser->previous->location.line, parser->previous->location.column);
}

static void emit_bytes(int cnt, ...) {
    va_list args;
    va_start(args, cnt);
    for (int i = 0; i < cnt; i++) emit_byte(va_arg(args, int));
    va_end(args);
}

static void emit_nil_return() {
    // for initializer, slot 0 is instance
    if (current_resolver->type == TYPE_INITIALIZER) emit_bytes(2, CLOX_OP_GET_LOCAL, 0);
    else emit_byte(CLOX_OP_NIL);
    emit_return();
}

static void emit_return() {
    emit_byte(CLOX_OP_RETURN);
}

static void emit_constant(Value value) {
    uint16_t idx = make_constant(value);
    if (idx > UINT8_MAX) emit_bytes(3, CLOX_OP_CONSTANT_16, idx & 0xff, idx >> 8);
    else emit_bytes(2, CLOX_OP_CONSTANT, idx);
}

/** 
 * max size of constant pool is 65536 (0 ~ 65535)
 */
static uint16_t make_constant(Value value) {
    int idx = append_constant(current_chunk(), value);
    if (idx > UINT16_MAX) error_report(parser->previous, "Too many constants in one chunk.");
    return (uint16_t)idx;
}

static uint16_t identifier_constant(Token* identifier) {
    StringObj *identifier_string = new_string(identifier->lexeme, identifier->length);
    return make_constant(OBJ_VALUE(identifier_string));
}

static int emit_jump(uint8_t instruction) {
    emit_bytes(3, instruction, 0xff, 0xff);
    return current_chunk()->count - 2;
}

// patch distance from current to @param: offset to @param:offset
// program at @param: offset, read 2 Bytes, may jump to current position
// -> offset       |
// -> offset + 1   |
//     ...         | ---- 
//     ...         |    | -> @return: jump (caled by current function)
// -> current      | ----
static void patch_jump(int offset) {
    Chunk *chunk = current_chunk();
    int jump = chunk->count - offset - 2;
    if (jump > UINT16_MAX) error_report(parser->previous, "Too much code to jump over.");
    chunk->code[offset] = jump & 0xff;
    chunk->code[offset + 1] = (jump >> 8) & 0xff;
}

// jump to @param: start
// -> start                  | ----
//     ...                   |    |
//     ...                   |    | -> @return: jump (caled by current function)
// -> current (CLOX_OP_LOOP) |    | 
// -> low bits               |    |
// -> high bits              | ----
static void emit_loop(int start) {
    int jump = current_chunk()->count - start + 3;
    emit_bytes(3, CLOX_OP_LOOP, jump & 0xff, (jump >> 8) & 0xff);
}

static void error_report(Token *token, const char *message) {
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

static void synchronize() {
    parser->panic_mode = false;

    while (!check(CLOX_TOKEN_EOF)) {
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
        advance();
    }
}