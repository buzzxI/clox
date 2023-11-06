#include <stdio.h>
#include "debug.h"

static int non_operand(const char *name, int offset);

int main(int argc, const char* argv[]) {
    Chunk chunk;
    init_chunk(&chunk);
    write_chunk(&chunk, OP_RETURN);
    disassemble_chunk(&chunk, "test chunk");
    free_chunk(&chunk);
    return 0;
}

void disassemble_chunk(Chunk* chunk, const char* name) {
    printf("== DEBUG: %s ==\n", name);
    for (int offset = 0; offset < chunk->count; offset = disassemble_instruction(chunk, offset));
    printf("==  END: %s  ==\n", name);
}

int disassemble_instruction(Chunk *chunk, int offset) {
    // padding to at least 4 characters width
    printf("%04d ", offset);
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case OP_RETURN: 
            return non_operand("OP_RETURN", offset);
            break;
        default:
            break;
    }
}

static int non_operand(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}