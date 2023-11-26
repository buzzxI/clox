#ifndef clox_complier_h
#define clox_complier_h

#include "scanner/scanner.h"
#include "chunk/chunk.h"

#define MAX_STACK (UINT16_MAX + 1)

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
  PREC_XOR,         // xor
  PREC_OR,          // or
  PREC_AND,         // and
  PREC_EQUALITY,    // == !=
  PREC_COMPARISON,  // < > <= >=
  PREC_TERM,        // + -
  PREC_FACTOR,      // * /
  PREC_UNARY,       // ! -
  PREC_CALL,        // . ()
  PREC_PRIMARY      // literal
} Precedence;

typedef struct {
  Token *previous;
  Token *current; 
  bool had_error;
  bool panic_mode;

  Scanner *scanner;
  Chunk *chunk;
} Parser;

typedef struct {
  Token name;
  int depth;
} Local;

typedef struct {
  Local locals[MAX_STACK];
  int local_count;
  int scope_depth;
} Resolver;

typedef struct {
  Parser *parser;
  Resolver *resolver;
} Compiler;

typedef void (*parser_func)(Compiler *, bool);

typedef struct {
  parser_func prefix;
  parser_func infix;
  Precedence precedence;
} ParserRule;

// return 0 on success, -1 on error
bool compile(const char *source, Chunk *chunk);

#endif