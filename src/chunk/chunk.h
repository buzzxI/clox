#ifndef clox_chunk_h
#define clox_chunk_h
#include "common.h"

typedef enum {
    OP_RETURN,
} OpCode;

typedef struct {
    uint8_t *code;  // bytecode
    int capacity;   // size of memory allocated
    int count;      // size of memory used
} Chunk;

void init_chunk(Chunk *chunk);
void write_chunk(Chunk *chunk, uint8_t byte);
void free_chunk(Chunk *chunk);

#endif  // clox_chunk_h