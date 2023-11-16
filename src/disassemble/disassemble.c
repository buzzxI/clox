#include "disassemble.h"
#include <stdio.h>

static int non_operand(const char *name, int offset);
static int single_operand(const char *name, Chunk *chunk, int offset);

void disassemble_chunk(Chunk* chunk, const char* name) {
    printf("== DEBUG: %s ==\n", name);
    for (int offset = 0; offset < chunk->count;) {
        offset = disassemble_instruction(chunk, offset);
    }
    printf("==  END: %s  ==\n", name);
}

int disassemble_instruction(Chunk *chunk, int offset) {
    // 0 is used for padding (at least 4 characters width), default is right-justified
    printf("%04d ", offset);
    // line info with 4 characters width
    if (offset > 0 && chunk->line_info[offset] == chunk->line_info[offset - 1]) printf("   | ");
    else printf("%4d ",chunk->line_info[offset]);
    uint8_t instruction = chunk->code[offset];
    switch (instruction) {
        case CLOX_OP_RETURN: 
            return non_operand("CLOX_OP_RETURN", offset);
        case CLOX_OP_CONSTANT:
            return single_operand("CLOX_OP_CONSTANT", chunk, offset);
        case CLOX_OP_NEGATE:
            return non_operand("CLOX_OP_NEGATE", offset);
        case CLOX_OP_ADD:
            return non_operand("CLOX_OP_ADD", offset);
        case CLOX_OP_SUBTRACT:
            return non_operand("CLOX_OP_SUBTRACT", offset);
        case CLOX_OP_MULTIPLY:
            return non_operand("CLOX_OP_MULTIPLY", offset);
        case CLOX_OP_DIVIDE:
            return non_operand("CLOX_OP_DIVIDE", offset);
        default:
            break;
    }
    return chunk->count;
}

static int non_operand(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int single_operand(const char *name, Chunk *chunk, int offset) {
    uint8_t idx = chunk->code[offset + 1];
    Value constant = chunk->constant.values[idx];
    // '%-16s' will padding string (at least 16 characters width), '-' is left-justified
    // '%4d' will padding number with blank (at least 4 characters width)
    printf("%-16s %4d '", name, idx);
    // %g => when number is greater than 1e6 or less than 1e-4 use %e, otherwise use %f
    printf("%g\n", constant);
    return offset + 2;
}