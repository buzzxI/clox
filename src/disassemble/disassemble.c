#include "disassemble.h"
#include "object/object.h"
#include <stdio.h>

static int non_operand(const char *name, int offset);
static int single_operand(const char *name, Chunk *chunk, int offset);
static int double_operand(const char *name, Chunk *chunk, int offset);
static void print_idx(const char *name, int idx);
static int constant(const char *name, Chunk *chunk, int offset);
static int constant_16(const char *name, Chunk *chunk, int offset);
static void print_constant(const char *name, Chunk *chunk, int idx);
static int function(const char *name, Chunk *chunk, int offset);
static int function_16(const char *name, Chunk *chunk, int offset);
static int print_function(const char *name, Chunk *chunk, int offset, int idx);
static uint8_t single_byte(Chunk *chunk, int offset);
static uint16_t double_bytes(Chunk *chunk, int offset);

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
        case CLOX_OP_RETURN:            return non_operand("CLOX_OP_RETURN", offset);
        case CLOX_OP_CONSTANT:          return constant("CLOX_OP_CONSTANT", chunk, offset);
        case CLOX_OP_CONSTANT_16:       return constant_16("CLOX_OP_CONSTANT_16", chunk, offset);
        case CLOX_OP_TRUE:              return non_operand("CLOX_OP_TRUE", offset);
        case CLOX_OP_FALSE:             return non_operand("CLOX_OP_FALSE", offset);
        case CLOX_OP_NIL:               return non_operand("CLOX_OP_NIL", offset);
        case CLOX_OP_NEGATE:            return non_operand("CLOX_OP_NEGATE", offset);
        case CLOX_OP_ADD:               return non_operand("CLOX_OP_ADD", offset);
        case CLOX_OP_SUBTRACT:          return non_operand("CLOX_OP_SUBTRACT", offset);
        case CLOX_OP_MULTIPLY:          return non_operand("CLOX_OP_MULTIPLY", offset);
        case CLOX_OP_DIVIDE:            return non_operand("CLOX_OP_DIVIDE", offset);
        case CLOX_OP_MODULO:            return non_operand("CLOX_OP_MODULO", offset);
        case CLOX_OP_NOT:               return non_operand("CLOX_OP_NOT", offset);
        case CLOX_OP_GREATER:           return non_operand("CLOX_OP_GREATER", offset);
        case CLOX_OP_LESS:              return non_operand("CLOX_OP_LESS", offset);
        case CLOX_OP_EQUAL:             return non_operand("CLOX_OP_EQUAL", offset);
        case CLOX_OP_PRINT:             return non_operand("CLOX_OP_PRINT", offset);
        case CLOX_OP_POP:               return non_operand("CLOX_OP_POP", offset);
        case CLOX_OP_DEFINE_GLOBAL:     return constant("CLOX_OP_DEFINE_GLOBAL", chunk, offset);
        case CLOX_OP_DEFINE_GLOBAL_16:  return constant_16("CLOX_OP_DEFINE_GLOBAL_16", chunk, offset);
        case CLOX_OP_GET_GLOBAL:        return constant("CLOX_OP_GET_GLOBAL", chunk, offset);
        case CLOX_OP_GET_GLOBAL_16:     return constant_16("CLOX_OP_GET_GLOBAL_16", chunk, offset);
        case CLOX_OP_SET_GLOBAL:        return constant("CLOX_OP_SET_GLOBAL", chunk, offset);
        case CLOX_OP_SET_GLOBAL_16:     return constant_16("CLOX_OP_SET_GLOBAL_16", chunk, offset);
        case CLOX_OP_GET_LOCAL:         return single_operand("CLOX_OP_GET_LOCAL", chunk, offset);
        case CLOX_OP_GET_LOCAL_16:      return double_operand("CLOX_OP_GET_LOCAL_16", chunk, offset);
        case CLOX_OP_SET_LOCAL:         return single_operand("CLOX_OP_SET_LOCAL", chunk, offset);
        case CLOX_OP_SET_LOCAL_16:      return double_operand("CLOX_OP_SET_LOCAL_16", chunk, offset);
        case CLOX_OP_JUMP_IF_FALSE:     return double_operand("CLOX_OP_JUMP_IF_FALSE", chunk, offset);
        case CLOX_OP_JUMP:              return double_operand("CLOX_OP_JUMP", chunk, offset);
        case CLOX_OP_LOOP:              return double_operand("CLOX_OP_LOOP", chunk, offset);
        case CLOX_OP_CALL:              return single_operand("CLOX_OP_CALL", chunk, offset);
        case CLOX_OP_CLOSURE:           return function("CLOX_OP_CLOSURE", chunk, offset);
        case CLOX_OP_CLOSURE_16:        return function_16("CLOX_OP_CLOSURE_16", chunk, offset);
        case CLOX_OP_GET_UPVALUE:       return single_operand("CLOX_OP_GET_UPVALUE", chunk, offset);
        case CLOX_OP_GET_UPVALUE_16:    return double_operand("CLOX_OP_GET_UPVALUE_16", chunk, offset);
        case CLOX_OP_SET_UPVALUE:       return single_operand("CLOX_OP_SET_UPVALUE", chunk, offset);
        case CLOX_OP_SET_UPVALUE_16:    return double_operand("CLOX_OP_SET_UPVALUE_16", chunk, offset);
        case CLOX_OP_CLOSE_UPVALUE:     return non_operand("CLOX_OP_CLOSE_UPVALUE", offset);
        default: break;
    }
    return chunk->count;
}

static int non_operand(const char *name, int offset) {
    printf("%s\n", name);
    return offset + 1;
}

static int single_operand(const char *name, Chunk *chunk, int offset) {
    uint8_t idx = single_byte(chunk, offset + 1);
    print_idx(name, idx);
    return offset + 2;
}

static int double_operand(const char *name, Chunk *chunk, int offset) {
    uint16_t idx = double_bytes(chunk, offset + 1);
    print_idx(name, idx);
    return offset + 3;
}

static void print_idx(const char *name, int idx) {
    printf("%-24s %4d \n", name, idx);
}

static int constant(const char *name, Chunk *chunk, int offset) {
    uint8_t idx = single_byte(chunk, offset + 1);
    print_constant(name, chunk, idx); 
    return offset + 2;
}

static int constant_16(const char *name, Chunk *chunk, int offset) {
    uint16_t idx = double_bytes(chunk, offset + 1);
    print_constant(name, chunk, idx);
    return offset + 3;
}

static void print_constant(const char *name, Chunk *chunk, int idx) {
    Value constant = chunk->constant.values[idx];
    // '%-16s' will padding string (at least 16 characters width), '-' is left-justified
    // '%4d' will padding number with blank (at least 4 characters width)
    printf("%-24s %4d '", name, idx);
    print_value(constant);
    printf("\n");
}

static int function(const char *name, Chunk *chunk, int offset) {
    uint8_t idx = single_byte(chunk, offset + 1);
    offset += 2;
    return print_function(name, chunk, offset, idx);
}

static int function_16(const char *name, Chunk *chunk, int offset) {
    uint16_t idx = double_bytes(chunk, offset + 1);
    offset += 3;
    return print_function(name, chunk, offset, idx);
}

static int print_function(const char *name, Chunk *chunk, int offset, int idx) {
    FunctionObj *function = AS_FUNCTION(chunk->constant.values[idx]);
    // '%-16s' will padding string (at least 16 characters width), '-' is left-justified
    // '%4d' will padding number with blank (at least 4 characters width)
    printf("%-24s %4d '", name, idx);
    print_value(OBJ_VALUE(function));
    printf("\n");

    for (int i = 0; i < function->upvalue_count; i++, offset += 3) {
        uint8_t is_local = single_byte(chunk, offset);
        uint16_t idx = double_bytes(chunk, offset + 1);
        printf("%04d      | ", offset);
        printf("%27s '%d\n", is_local ? "local" : "upvalue", idx);
    }

    return offset;
}

static uint8_t single_byte(Chunk *chunk, int offset) {
    return chunk->code[offset];
}

static uint16_t double_bytes(Chunk *chunk, int offset) {
    uint8_t backword = chunk->code[offset];
    uint8_t foreward = chunk->code[offset + 1];
    return (foreward << 8) | backword;
}