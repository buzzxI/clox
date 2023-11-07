#include "chunk.h"
#include "memory.h"

void init_chunk(Chunk *chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constant);
}

void write_chunk(Chunk *chunk, uint8_t byte) {
    if (chunk->count + 1 > chunk->capacity) {
        int new_capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->capacity, new_capacity);
        chunk->capacity = new_capacity;
    }
    chunk->code[chunk->count] = byte;
    chunk->count++;
}

void free_chunk(Chunk *chunk) {
    free_value_array(&chunk->constant);
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    init_chunk(chunk);
}

int append_constant(Chunk *chunk, Value value) {
    write_value_array(&chunk->constant, value);
    return chunk->constant.count - 1;
}