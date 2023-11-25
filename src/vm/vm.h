#ifndef clox_vm_h
#define clox_vm_h
#include "chunk/chunk.h"
#include "table/table.h"

#define MAX_STACK (UINT16_MAX + 1)

typedef struct {
    Chunk *chunk;
    uint8_t *pc;
    Value stack[MAX_STACK];
    Value *sp;
    Obj *objs;
    Table strings;
    Table globals;
} VM;

typedef enum {
    INTERPRET_OK,
    INTERPRET_COMPLIE_ERROR,
    INTERPRET_RUNTIME_ERROR,
    INTERPRET_DEBUG,
} InterpreterResult;

extern VM vm;

void init_vm();
void free_vm();
InterpreterResult interpret(const char *source);

#endif // clox_vm_h