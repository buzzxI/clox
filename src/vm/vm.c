#include "vm.h"
#include "disassemble/disassemble.h"
#include "complier/compiler.h"
#include "common.h"
// added for print constants
#include <stdio.h>
// added for wrap format print
#include <stdarg.h>

static void reset_stack(VM *vm);
static InterpreterResult run(VM *vm);
static void runtime_error(VM *vm, const char *format, ...);
static void push(VM *vm, Value value);
static Value pop(VM *vm);
// static Value peek(VM *vm);

void init_vm(VM *vm) {
    reset_stack(vm);
    vm->chunk = NULL;
    vm->pc = NULL;
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
}

static void reset_stack(VM *vm) {
    vm->sp = vm->stack;
}

static InterpreterResult run(VM *vm) {
#define READ_BYTE() (*vm->pc++)
#define PEEK_BYTE() (*vm->pc)
#define READ_CONSTANT() (vm->chunk->constant.values[READ_BYTE()])
#define READ_CONSTANT_16() (vm->chunk->constant.values[(READ_BYTE() << 8) | (READ_BYTE())])
#define BINARY_OP(val_type, op) do {\
        Value b = pop(vm);\
        Value a = pop(vm);\
        if (!IS_NUMBER(a) || !IS_NUMBER(b)) {\
            runtime_error(vm, "operands must be numbers.");\
            return INTERPRET_RUNTIME_ERROR;\
        }\
        push(vm, val_type(a.data.number op b.data.number));\
    } while (false)
    for (;;) {
#ifdef CLOX_DEBUG_TRACE_EXECUTION
        printf("stack trace:[");
        for (Value *slot = vm->stack; slot < vm->sp; slot++) {
            print_value(*slot);
            printf(" ");
        } 
        printf("%%sp]\n");
        disassemble_instruction(vm->chunk, (int)(vm->pc - vm->chunk->code));
#endif
        uint8_t instruction = READ_BYTE();
        switch (instruction) {
            case CLOX_OP_RETURN:
                print_value(pop(vm));
                printf("\n");
                return INTERPRET_OK;
            case CLOX_OP_CONSTANT:
                push(vm, READ_CONSTANT());
                break;
            case CLOX_OP_CONSTANT_16:
                push(vm, READ_CONSTANT_16());
                break;
            case CLOX_OP_TRUE:
                push(vm, BOOL_VALUE(true));
                break;
            case CLOX_OP_FALSE:
                push(vm, BOOL_VALUE(false));
                break;
            case CLOX_OP_NIL:
                push(vm, NIL_VALUE);
                break;
            case CLOX_OP_NEGATE: {
                Value value = pop(vm);
                if (!IS_NUMBER(value)) {
                    runtime_error(vm, "operand for '-' must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(vm, NUMBER_VALUE(-value.data.number));
                break;
            }
            case CLOX_OP_ADD:
                BINARY_OP(NUMBER_VALUE, +);
                break;
            case CLOX_OP_SUBTRACT:
                BINARY_OP(NUMBER_VALUE, -);
                break;
            case CLOX_OP_MULTIPLY:
                BINARY_OP(NUMBER_VALUE, *);
                break;
            case CLOX_OP_DIVIDE:
                BINARY_OP(NUMBER_VALUE, /);
                break;
            case CLOX_OP_NOT:
                push(vm, BOOL_VALUE(is_false(pop(vm))));
                break;
            case CLOX_OP_EQUAL: {
                Value b = pop(vm);
                Value a = pop(vm);
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm->pc++;
                    push(vm, BOOL_VALUE(!values_equal(a, b)));
                } else push(vm, BOOL_VALUE(values_equal(a, b))); 
                break;
            }
            case CLOX_OP_GREATER:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm->pc++;
                    BINARY_OP(BOOL_VALUE, <=);
                } else BINARY_OP(BOOL_VALUE, >);
                break;
            case CLOX_OP_LESS:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm->pc++;
                    BINARY_OP(BOOL_VALUE, >=);
                } else BINARY_OP(BOOL_VALUE, <);
                break;
            default:
                break;
        }
    }
#undef READ_BYTE
#undef PEEK_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_16
#undef BINARY_OP
}

static void runtime_error(VM *vm, const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    // vm will increse pc after read an instruction
    int offset = vm->pc - vm->chunk->code - 1;
    int line = vm->chunk->line_info[offset];
    int column = vm->chunk->column_info[offset];
    fprintf(stderr, "[line %d, column %d] in script\n", line, column);
    reset_stack(vm);
}

static void push(VM *vm, Value value) {
    *vm->sp = value;
    vm->sp++;
}

static Value pop(VM *vm) {
    vm->sp--;
    return *vm->sp; 
}

// static Value peek(VM *vm) {
//     return vm->sp[-1];
// }