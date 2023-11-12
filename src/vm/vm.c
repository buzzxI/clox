#include "vm.h"
#include "disassemble/disassemble.h"
#include "complier/compiler.h"
#include "common.h"
// added for print constants
#include <stdio.h>

static void reset_stack(VM *vm);
static InterpreterResult run(VM *vm);

void init_vm(VM *vm) {
    reset_stack(vm);
}

void free_vm(VM *vm) {

}

InterpreterResult interpret(const char *source) {
    VM vm;
    init_vm(&vm);
    compile(source);

    // Chunk chunk;
    // init_chunk(&chunk);
    // // append 1.2
    // write_chunk(&chunk, OP_CONSTANT, 123);
    // int idx = append_constant(&chunk, 1.2);
    // write_chunk(&chunk, idx, 123);
    // // append 3.4
    // write_chunk(&chunk, OP_CONSTANT, 123);
    // idx = append_constant(&chunk, 3.4);
    // write_chunk(&chunk, idx, 123);
    // // append OP_ADD
    // write_chunk(&chunk, OP_ADD, 123);
    // // append 5.6
    // write_chunk(&chunk, OP_CONSTANT, 123);
    // idx = append_constant(&chunk, 5.6);
    // write_chunk(&chunk, idx, 123);
    // // append OP_DIVIDE
    // write_chunk(&chunk, OP_DIVIDE, 123);
    // // append OP_NEGATE
    // write_chunk(&chunk, OP_NEGATE, 123);
    // // append OP_RETUNR
    // write_chunk(&chunk, OP_RETURN, 123); 
    // InterpreterResult rst = interpret(&vm, &chunk);
    // printf("running rst: %d\n", rst);
    // free_chunk(&chunk);
    free_vm(&vm);
}

// InterpreterResult interpret(VM *vm, Chunk *chunk) {
//     vm->chunk = chunk;
//     vm->pc = vm->chunk->code;
// #ifdef  CLOX_DEBUG_DISASSEMBLE 
//         disassemble_chunk(chunk, "clox dump");
//         return INTERPRET_DEBUG;
// #else
//     return run(vm);
// #endif  // CLOX_DEBUG_DISASSEMBLE
// }

void push(VM *vm, Value value) {
    *vm->sp = value;
    vm->sp++;
}

Value pop(VM *vm) {
    vm->sp--;
    return *vm->sp; 
}

static void reset_stack(VM *vm) {
    vm->sp = vm->stack;
}

static InterpreterResult run(VM *vm) {
#define READ_BYTE() (*vm->pc++)
#define READ_CONSTANT() (vm->chunk->constant.values[READ_BYTE()])
#define BINARY_OP(op) do {\
        Value b = pop(vm);\
        Value a = pop(vm);\
        push(vm, a op b);\
    } while (false)
    for (;;) {
#ifdef CLOX_DEBUG_TRACE_EXECUTION
        printf("stack trace: ");
        for (Value *slot = vm->stack; slot < vm->sp; slot++) printf("[%g]", *slot);
        printf(" sp\n");
        disassemble_instruction(vm->chunk, (int)(vm->pc - vm->chunk->code));
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case OP_RETURN:
                printf("%g\n", pop(vm));
                return INTERPRET_OK;
            case OP_CONSTANT:
                push(vm, READ_CONSTANT());
                break;
            case OP_NEGATE:
                push(vm, -pop(vm));
                break; 
            case OP_ADD:
                BINARY_OP(+);
                break;
            case OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case OP_DIVIDE:
                BINARY_OP(/);
                break;
            default:
                break;
        }
    }
#undef READ_CONSTANT
#undef READ_BYTE
#undef BINARY_OP
}