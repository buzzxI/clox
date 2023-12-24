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
    // a linked list with dummy head
    Obj objs;
    Table strings;
    Table globals;
    // a linked list with dummy head
    UpvalueObj upvalues;
    // class initializer name 
    StringObj *init_string;

    // gray stack for traversal
    Obj* gray_stack[MAX_STACK];
    int gray_count;
    // int gray_capacity;
    // fields trigger gc
    size_t allocated_bytes;
    size_t next_gc;
    // a temporary stack for gc
    Value gc_stack[UINT8_COUNT];
    int gc_stack_cnt;
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
// push a value into gc stack
void push_gc(Value value);
// pop a value from gc stack
Value pop_gc();

#endif // clox_vm_h