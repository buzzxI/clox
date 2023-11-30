#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value/value.h"

typedef enum {
    CLOX_OP_RETURN,
    CLOX_OP_CONSTANT,
    // constant with 2 bytes
    CLOX_OP_CONSTANT_16,
    CLOX_OP_TRUE,
    CLOX_OP_FALSE,
    CLOX_OP_NIL,
    CLOX_OP_NEGATE,
    CLOX_OP_ADD,
    CLOX_OP_SUBTRACT,
    CLOX_OP_MULTIPLY,
    CLOX_OP_DIVIDE,
    CLOX_OP_MODULO,
    CLOX_OP_POWER,
    CLOX_OP_NOT,
    CLOX_OP_EQUAL,
    CLOX_OP_GREATER,
    CLOX_OP_LESS,
    CLOX_OP_PRINT,
    CLOX_OP_POP,
    CLOX_OP_DEFINE_GLOBAL,
    CLOX_OP_DEFINE_GLOBAL_16,
    CLOX_OP_GET_GLOBAL,
    CLOX_OP_GET_GLOBAL_16,
    CLOX_OP_SET_GLOBAL,
    CLOX_OP_SET_GLOBAL_16,
    CLOX_OP_GET_LOCAL,
    CLOX_OP_GET_LOCAL_16,
    CLOX_OP_SET_LOCAL,
    CLOX_OP_SET_LOCAL_16,
    CLOX_OP_JUMP_IF_FALSE,
    CLOX_OP_JUMP,
    CLOX_OP_LOOP,
    CLOX_OP_CALL,
} OpCode;

typedef struct {
    uint8_t *code;      // bytecode
    int capacity;       // size of memory allocated
    int count;          // size of memory used
    ValueArray constant;// constant pool in bytecode 
    int *line_info;     // line info of bytecode
    int *column_info;   // column info of bytecode
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line, int column);
void free_chunk(Chunk *chunk);

// append a constant into chunk, returns its index of constant pool
int append_constant(Chunk *chunk, Value value);

#endif  // clox_chunk_h