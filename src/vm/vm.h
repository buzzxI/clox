#ifndef clox_vm_h
#define clox_vm_h
#include "chunk/chunk.h"

#define MAX_STACK 256

typedef struct {
    Chunk *chunk;
    uint8_t *pc;
    Value stack[MAX_STACK];
    Value *sp;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPLIE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_DEBUG,
} InterpreterResult;

void init_vm(VM *vm);
void free_vm(VM *vm);
InterpreterResult interpret(VM *vm, Chunk *chunk);
void push(VM *vm, Value value);
Value pop(VM *vm);


#endif // clox_vm_h