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
    Chunk chunk;
    init_chunk(&chunk);
    if (!compile(source, &chunk)) {
        free_chunk(&chunk);
        return INTERPRET_COMPLIE_ERROR;
    }
    vm.chunk = &chunk;
    vm.pc = chunk.code;
    InterpreterResult rst;
#ifdef  CLOX_DEBUG_DISASSEMBLE 
    disassemble_chunk(&chunk, "clox dump");
    rst = INTERPRET_DEBUG;
#else
    rst = run(&vm);
#endif  // CLOX_DEBUG_DISASSEMBLE
    free_chunk(&chunk);
    return rst; 
    // // append 1.2
    // write_chunk(&chunk, CLOX_OP_CONSTANT, 123);
    // int idx = append_constant(&chunk, 1.2);
    // write_chunk(&chunk, idx, 123);
    // // append 3.4
    // write_chunk(&chunk, CLOX_OP_CONSTANT, 123);
    // idx = append_constant(&chunk, 3.4);
    // write_chunk(&chunk, idx, 123);
    // // append CLOX_OP_ADD
    // write_chunk(&chunk, CLOX_OP_ADD, 123);
    // // append 5.6
    // write_chunk(&chunk, CLOX_OP_CONSTANT, 123);
    // idx = append_constant(&chunk, 5.6);
    // write_chunk(&chunk, idx, 123);
    // // append CLOX_OP_DIVIDE
    // write_chunk(&chunk, CLOX_OP_DIVIDE, 123);
    // // append CLOX_OP_NEGATE
    // write_chunk(&chunk, CLOX_OP_NEGATE, 123);
    // // append CLOX_OP_RETUNR
    // write_chunk(&chunk, CLOX_OP_RETURN, 123); 
}

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
#define READ_CONSTANT_16() (vm->chunk->constant.values[(READ_BYTE() << 8) | (READ_BYTE())])
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
            case CLOX_OP_RETURN:
                printf("%g\n", pop(vm));
                return INTERPRET_OK;
            case CLOX_OP_CONSTANT:
                push(vm, READ_CONSTANT());
                break;
            case CLOX_OP_CONSTANT_16:
                push(vm, READ_CONSTANT_16());
                break;
            case CLOX_OP_NEGATE:
                push(vm, -pop(vm));
                break; 
            case CLOX_OP_ADD:
                BINARY_OP(+);
                break;
            case CLOX_OP_SUBTRACT:
                BINARY_OP(-);
                break;
            case CLOX_OP_MULTIPLY:
                BINARY_OP(*);
                break;
            case CLOX_OP_DIVIDE:
                BINARY_OP(/);
                break;
            default:
                break;
        }
    }
#undef READ_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_16
#undef BINARY_OP
}