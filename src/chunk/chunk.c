#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
}

void write_chunk(Chunk *chunk, uint8_t byte) {
    if (chunk->count + 1 > chunk->capacity) {
        if (chunk->capacity == 0) {
            int new_capacity = GROW_CAPACITY(chunk->capacity);
            chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->capacity, new_capacity);
            chunk->capacity = new_capacity; 
        } 
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
}