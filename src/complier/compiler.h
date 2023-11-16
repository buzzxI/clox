#ifndef clox_complier_h
#define clox_complier_h

#include "scanner/scanner.h"
#include "chunk/chunk.h"

typedef enum {
  PREC_NONE,
  PREC_ASSIGNMENT,  // =
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

typedef void (*parser_func)(Parser *);

typedef struct {
  parser_func prefix;
  parser_func infix;
  Precedence precedence;
} ParserRule;

// return 0 on success, -1 on error
bool compile(const char *source, Chunk *chunk);

#endif