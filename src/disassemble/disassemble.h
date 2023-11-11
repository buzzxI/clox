#ifndef clox_disassemble_h
#define clox_disassemble_h
#include "chunk/chunk.h"

void disassemble_chunk(Chunk *chunk, const char *name);
int disassemble_instruction(Chunk *chunk, int offset);

#endif  // clox_disassemble_h