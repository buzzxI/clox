#include "chunk.h"
#include "memory/memory.h"
#include "vm/vm.h"

void init_chunk(Chunk *chunk) {
    chunk->capacity = 0;
    chunk->count = 0;
    chunk->code = NULL;
    init_value_array(&chunk->constant);
    chunk->line_info = NULL;
    chunk->column_info = NULL;
}

void write_chunk(Chunk *chunk, uint8_t byte, int line, int column) {
    if (chunk->count + 1 > chunk->capacity) {
        int new_capacity = GROW_CAPACITY(chunk->capacity);
        chunk->code = GROW_ARRAY(uint8_t, chunk->code, chunk->capacity, new_capacity);
        chunk->capacity = new_capacity;
        chunk->line_info = GROW_ARRAY(int, chunk->line_info, chunk->capacity, new_capacity);
        chunk->column_info = GROW_ARRAY(int, chunk->column_info, chunk->capacity, new_capacity);
    }
    chunk->code[chunk->count] = byte;
    chunk->line_info[chunk->count] = line;
    chunk->column_info[chunk->count] = column;
    chunk->count++;
}

void free_chunk(Chunk *chunk) {
    free_value_array(&chunk->constant);
    FREE_ARRAY(uint8_t, chunk->code, chunk->capacity);
    FREE_ARRAY(int, chunk->line_info, chunk->capacity);
    FREE_ARRAY(int, chunk->column_info, chunk->capacity);
    init_chunk(chunk);
}

int append_constant(Chunk *chunk, Value value) {
    // while append a constant may trigger reallocate constant pool
    push_gc(value);
    write_value_array(&chunk->constant, value);
    pop_gc(value);
    return chunk->constant.count - 1;
}