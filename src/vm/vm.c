#include "common.h"
#include "vm.h"
#include "disassemble/disassemble.h"
#include "complier/compiler.h"
#include "object/object.h"
// added for print constants
#include <stdio.h>
// added for wrap format print
#include <stdarg.h>

static void reset_stack();
static InterpreterResult run();
static void* read_bytes(int num);
static void runtime_error(const char *format, ...);
static void push(Value value);
static Value pop();
static Value peek(int distance);

VM vm;

void init_vm() {
    reset_stack();
    vm.chunk = NULL;
    vm.pc = NULL;
    vm.objs = NULL;
    init_table(&vm.strings);
    init_table(&vm.globals);
}

void free_vm() {
    free_objs();
    free_table(&vm.strings);
    free_table(&vm.globals);
}

InterpreterResult interpret(const char *source) {
    init_vm();
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
    rst = run();
#endif  // CLOX_DEBUG_DISASSEMBLE
    free_chunk(&chunk);
    free_vm();
    return rst; 
}

static void reset_stack() {
    vm.sp = vm.stack;
}

static InterpreterResult run() {
#define PEEK_BYTE()         (*vm.pc)
#define READ_CONSTANT()     (vm.chunk->constant.values[(*(uint8_t*)read_bytes(1))])
#define READ_CONSTANT_16()  (vm.chunk->constant.values[(*(uint16_t*)(read_bytes(2)))])
#define BINARY_OP(val_type, op) do {\
        Value b = pop();\
        Value a = pop();\
        if (!IS_NUMBER(a) || !IS_NUMBER(b)) {\
            runtime_error("operands must be numbers.");\
            return INTERPRET_RUNTIME_ERROR;\
        }\
        push(val_type(AS_NUMBER(a) op AS_NUMBER(b)));\
    } while (false)
    for (;;) {
#ifdef CLOX_DEBUG_TRACE_EXECUTION
        printf("stack trace:[");
        for (Value *slot = vm.stack; slot < vm.sp; slot++) {
            print_value(*slot);
            printf(" ");
        } 
        printf("%%sp]\n");
        disassemble_instruction(vm.chunk, (int)(vm.pc - vm.chunk->code));
#endif
        uint8_t *instruction = read_bytes(1);
        if (instruction == NULL) {
            runtime_error("running out of file.");
            return INTERPRET_RUNTIME_ERROR;
        }
        switch (*instruction) {
            case CLOX_OP_RETURN: return INTERPRET_OK;
            case CLOX_OP_CONSTANT:
                push(READ_CONSTANT());
                break;
            case CLOX_OP_CONSTANT_16:
                push(READ_CONSTANT_16());
                break;
            case CLOX_OP_TRUE:
                push(BOOL_VALUE(true));
                break;
            case CLOX_OP_FALSE:
                push(BOOL_VALUE(false));
                break;
            case CLOX_OP_NIL:
                push(NIL_VALUE);
                break;
            case CLOX_OP_NEGATE: {
                Value value = pop();
                if (!IS_NUMBER(value)) {
                    runtime_error("operand for '-' must be a number.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VALUE(-AS_NUMBER(value)));
                break;
            }
            case CLOX_OP_ADD: {
                Value b = pop();
                Value a = pop();
                if (IS_STRING(a) && IS_STRING(b)) push(append_string(a, b));
                else if (IS_NUMBER(a) && IS_NUMBER(b)) push(NUMBER_VALUE(AS_NUMBER(a) + AS_NUMBER(b)));
                else {
                    runtime_error("operands must be two numbers or two strings.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                break;
            }
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
                push(BOOL_VALUE(is_false(pop())));
                break;
            case CLOX_OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm.pc++;
                    push(BOOL_VALUE(!values_equal(a, b)));
                } else push(BOOL_VALUE(values_equal(a, b))); 
                break;
            }
            case CLOX_OP_GREATER:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm.pc++;
                    BINARY_OP(BOOL_VALUE, <=);
                } else BINARY_OP(BOOL_VALUE, >);
                break;
            case CLOX_OP_LESS:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    vm.pc++;
                    BINARY_OP(BOOL_VALUE, >=);
                } else BINARY_OP(BOOL_VALUE, <);
                break;
            case CLOX_OP_PRINT:
                print_value(pop());
                printf("\n");
                break;
            case CLOX_OP_POP:
                uint8_t *num = read_bytes(1);
                for (int i = 0; i < *num; i++) pop();
                break;
            case CLOX_OP_DEFINE_GLOBAL:{
                StringObj *identifier = AS_STRING(READ_CONSTANT());
                table_put(identifier, pop(), &vm.globals);
                break;
            }
            case CLOX_OP_DEFINE_GLOBAL_16: {
                StringObj *identifier = AS_STRING(READ_CONSTANT_16());
                table_put(identifier, pop(), &vm.globals);
                break;
            }
            case CLOX_OP_GET_GLOBAL: {
                StringObj *identifier = AS_STRING(READ_CONSTANT());
                Value value;
                if (!table_get(identifier, &value, &vm.globals)) {
                    runtime_error("undefined variable '%s'.", identifier->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case CLOX_OP_GET_GLOBAL_16: {
                StringObj *identifier = AS_STRING(READ_CONSTANT_16());
                Value value;
                if (!table_get(identifier, &value, &vm.globals)) {
                    runtime_error("undefined variable '%s'.", identifier->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(value);
                break;
            }
            case CLOX_OP_SET_GLOBAL: {
                StringObj *identifier = AS_STRING(READ_CONSTANT());
                if (!table_get(identifier, NULL, &vm.globals)) {
                    runtime_error("undefined variable '%s'.", identifier->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                table_put(identifier, peek(0), &vm.globals);
                break;
            }
            case CLOX_OP_SET_GLOBAL_16: {
                StringObj *identifier = AS_STRING(READ_CONSTANT_16());
                if (!table_get(identifier, NULL, &vm.globals)) {
                    runtime_error("undefined variable '%s'.", identifier->str);
                    return INTERPRET_RUNTIME_ERROR;
                }
                table_put(identifier, peek(0), &vm.globals);
                break;
            }
            case CLOX_OP_GET_LOCAL: {
                uint8_t *slot = read_bytes(1);
                push(vm.stack[*slot]);
                break;
            }
            case CLOX_OP_GET_LOCAL_16: {
                uint16_t *slot = read_bytes(2);
                push(vm.stack[*slot]);
                break;
            }
            case CLOX_OP_SET_LOCAL: {
                uint8_t *slot = read_bytes(1);
                vm.stack[*slot] = peek(0);
                break;
            }
            case CLOX_OP_SET_LOCAL_16: {
                uint16_t *slot = read_bytes(2);
                vm.stack[*slot] = peek(0);
                break;
            }
            default: break;
        }
    }
#undef INCREMENT_PC
#undef PEEK_BYTE
#undef READ_CONSTANT
#undef READ_CONSTANT_16
#undef BINARY_OP
}

static void* read_bytes(int num) {
    void *rst = vm.pc;
    vm.pc += num;
    if (vm.pc - vm.chunk->code > vm.chunk->count) {
        vm.pc = vm.chunk->code + vm.chunk->count - 1;
        rst = NULL;
    } 
    return rst;
}

static void runtime_error(const char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    // vm will increse pc after read an instruction
    int offset = vm.pc - vm.chunk->code - 1;
    int line = vm.chunk->line_info[offset];
    int column = vm.chunk->column_info[offset];
    fprintf(stderr, "[line %d, column %d] in script\n", line, column);
    reset_stack();
}

static void push(Value value) {
    *vm.sp = value;
    vm.sp++;
}

static Value pop() {
    vm.sp--;
    return *vm.sp; 
}

static Value peek(int distance) {
    return vm.sp[-1 - distance];
}