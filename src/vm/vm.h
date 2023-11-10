#ifndef clox_vm_h
#define clox_vm_h
#include "chunk.h"

typedef struct {
    Chunk *chunk;
} VM;

void init_vm(VM *vm);
void free_vm(VM *vm);

#endif // clox_vm_h