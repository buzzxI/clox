#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value/value.h"

typedef enum {
    CLOX_OP_RETURN,
    CLOX_OP_CONSTANT,
    // constant with 2 bytes
    CLOX_OP_CONSTANT_16,
    CLOX_OP_NEGATE,
    CLOX_OP_ADD,
    CLOX_OP_SUBTRACT,
    CLOX_OP_MULTIPLY,
    CLOX_OP_DIVIDE,
} OpCode;

typedef struct {
    uint8_t *code;      // bytecode
    int capacity;       // size of memory allocated
    int count;          // size of memory used
    ValueArray constant;// constant pool in bytecode 
    int *line_info;     // line info of bytecode
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte, int line);
void free_chunk(Chunk *chunk);

// append a constant into chunk, returns its index of constant pool
int append_constant(Chunk *chunk, Value value);

#endif  // clox_chunk_h