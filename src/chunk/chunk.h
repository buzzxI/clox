#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"
#include "value.h"

typedef enum {
    OP_RETURN,
    OP_CONSTANT,
} OpCode;

typedef struct {
    uint8_t *code;      // bytecode
    int capacity;       // size of memory allocated
    int count;          // size of memory used
    ValueArray constant;// constant pool in bytecode 
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);
void free_chunk(Chunk *chunk);

// append a constant into chunk, returns its index of constant pool
int append_constant(Chunk *chunk, Value value);

#endif  // clox_chunk_h