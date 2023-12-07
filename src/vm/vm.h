#ifndef clox_vm_h
#define clox_vm_h
#include "chunk/chunk.h"
#include "object/object.h"
#include "table/table.h"

#define UINT8_COUNT (UINT8_MAX + 1)
#define FRAMES_MAX (UINT8_COUNT)
#define MAX_STACK ((FRAMES_MAX) * (UINT8_COUNT))

typedef struct {
    ClosureObj *closure;
    uint8_t *pc;
    Value *slots; 
} CallFrame;

typedef struct {
    CallFrame frames[FRAMES_MAX];
    int frame_cnt;
    Value stack[MAX_STACK];
    Value *sp;
    Obj *objs;
    Table strings;
    Table globals;
    // a linked list with dummy head
    UpvalueObj upvalues;
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