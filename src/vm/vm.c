#include "common.h"
#include "vm.h"
#include "disassemble/disassemble.h"
#include "complier/compiler.h"
#include "object/object.h"
// added for print constants
#include <stdio.h>
// added for wrap format print
#include <stdarg.h>
// added for exponentiation operation
#include <math.h>
// added for native function call time
#include <time.h>
// added for strlen
#include <string.h>

static void reset_stack();
static InterpreterResult run();
static bool function_call(Value function, uint8_t arg_cnt);
static bool invoke(FunctionObj *function, uint8_t arg_cnt);
static void* read_bytes(int num);
static void runtime_error(char *format, ...);
static void push(Value value);
static Value pop();
static Value peek(int distance);
static void define_native(const char *name, native_func native);
static Value native_clock(int argc, Value *args);

VM vm;

void init_vm() {
    reset_stack();
    vm.objs = NULL;
    init_table(&vm.strings);
    init_table(&vm.globals);
    define_native("clock", native_clock);
}

void free_vm() {
    free_objs();
    free_table(&vm.strings);
    free_table(&vm.globals);
}

InterpreterResult interpret(const char *source) {
    init_vm();
    FunctionObj *function = compile(source);
    if (function == NULL) return INTERPRET_COMPLIE_ERROR;
    push(OBJ_VALUE(function));
    invoke(function, 0);
    InterpreterResult rst = run();
    free_vm();
    return rst; 
}

static void reset_stack() {
    vm.sp = vm.stack;
    vm.frame_cnt = 0;
}

static InterpreterResult run() {
    CallFrame *frame = &vm.frames[vm.frame_cnt - 1];
#define PEEK_BYTE()         (*frame->pc)
#define READ_CONSTANT()     (frame->function->chunk.constant.values[(*(uint8_t*)(read_bytes(1)))])
#define READ_CONSTANT_16()  (frame->function->chunk.constant.values[(*(uint16_t*)(read_bytes(2)))])
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
        disassemble_instruction(&frame->function->chunk, (int)(frame->pc - frame->function->chunk.code));
#endif // CLOX_DEBUG_TRACE_EXECUTION
        
        uint8_t *instruction = read_bytes(1);
        if (instruction == NULL) {
            runtime_error("running out of file.");
            return INTERPRET_RUNTIME_ERROR;
        }
        switch (*instruction) {
            case CLOX_OP_RETURN: {
                // return value
                Value rst = pop();
                vm.frame_cnt--;
                if (vm.frame_cnt == 0) {
                    // pop the entry function
                    pop();
                    return INTERPRET_OK;
                }
                // reset vm stack
                vm.sp = frame->slots;
                push(rst);
                frame = &vm.frames[vm.frame_cnt - 1];
                break;
            }
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
            case CLOX_OP_MODULO: {
                Value b = pop();
                Value a = pop();
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { 
                    runtime_error("operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                int64_t x = (int64_t)AS_NUMBER(a);
                int64_t y = (int64_t)AS_NUMBER(b);
                push(NUMBER_VALUE(x % y));
                break;
            }
            case CLOX_OP_POWER: {
                Value b = pop();
                Value a = pop();
                if (!IS_NUMBER(a) || !IS_NUMBER(b)) { 
                    runtime_error("operands must be numbers.");
                    return INTERPRET_RUNTIME_ERROR;
                }
                push(NUMBER_VALUE(pow(AS_NUMBER(a), AS_NUMBER(b))));
                break;
            }
            case CLOX_OP_NOT:
                push(BOOL_VALUE(is_false(pop())));
                break;
            case CLOX_OP_EQUAL: {
                Value b = pop();
                Value a = pop();
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    frame->pc++;
                    push(BOOL_VALUE(!values_equal(a, b)));
                } else push(BOOL_VALUE(values_equal(a, b))); 
                break;
            }
            case CLOX_OP_GREATER:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    frame->pc++;
                    BINARY_OP(BOOL_VALUE, <=);
                } else BINARY_OP(BOOL_VALUE, >);
                break;
            case CLOX_OP_LESS:
                if (PEEK_BYTE() == CLOX_OP_NOT) {
                    frame->pc++;
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
                push(frame->slots[*slot]);
                break;
            }
            case CLOX_OP_GET_LOCAL_16: {
                uint16_t *slot = read_bytes(2);
                push(frame->slots[*slot]);
                break;
            }
            case CLOX_OP_SET_LOCAL: {
                uint8_t *slot = read_bytes(1);
                frame->slots[*slot] = peek(0);
                break;
            }
            case CLOX_OP_SET_LOCAL_16: {
                uint16_t *slot = read_bytes(2);
                frame->slots[*slot] = peek(0);
                break;
            }
            case CLOX_OP_JUMP_IF_FALSE: {
                uint16_t *offset = read_bytes(2);
                if (is_false(peek(0))) frame->pc += *offset;
                break;
            }
            case CLOX_OP_JUMP: {
                uint16_t *offset = read_bytes(2);
                frame->pc += *offset;
                break;
            }
            case CLOX_OP_LOOP: {
                uint16_t *offset = read_bytes(2);
                frame->pc -= *offset;
                break;
            }
            case CLOX_OP_CALL: {
                // read arg count
                uint8_t *arg_cnt = read_bytes(1);
                // invoke a function (add a call frame)
                if (!function_call(peek(*arg_cnt), *arg_cnt)) return INTERPRET_RUNTIME_ERROR;
                frame = &vm.frames[vm.frame_cnt - 1];
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

static bool function_call(Value function, uint8_t arg_cnt) {
    if (!IS_OBJ(function)) {
        runtime_error("can only call functions and classes.");
        return false;
    }
    switch(OBJ_TYPE(function)) {
        case OBJ_FUNCTION:
            return invoke(AS_FUNCTION(function), arg_cnt);
        case OBJ_NATIVE: {
            NativeObj *native = AS_NATIVE(function);
            Value rst = native->native(arg_cnt, vm.sp - arg_cnt);
            // discard arguments and native function name
            vm.sp -= arg_cnt + 1;
            push(rst);
            return true;
        }
        default: break;
    }
    return false;
}

static bool invoke(FunctionObj *function, uint8_t arg_cnt) {
    if (vm.frame_cnt == FRAMES_MAX) {
        runtime_error("stack overflow.");
        return false;
    }
    if (function->arity != arg_cnt) {
        runtime_error("expected %d arguments but got %d.", function->arity, arg_cnt);
        return false;
    }
    CallFrame *frame = &vm.frames[vm.frame_cnt++];
    frame->function = function;
    frame->pc = function->chunk.code;
    frame->slots = vm.sp - arg_cnt - 1;
    return true;
}

static void* read_bytes(int num) {
    CallFrame *frame = &vm.frames[vm.frame_cnt - 1];
    void *rst = frame->pc;
    frame->pc += num;
    if (frame->pc - frame->function->chunk.code > frame->function->chunk.count) {
        frame->pc = frame->function->chunk.code + frame->function->chunk.count - 1;
        rst = NULL;
    } 
    return rst;
}

static void runtime_error(char *format, ...) {
    va_list args;
    va_start(args, format);
    vfprintf(stderr, format, args);
    va_end(args);
    fputc('\n', stderr);

    for (int i = vm.frame_cnt - 1; i >= 0; i--) {
        CallFrame *frame = &vm.frames[i];
        FunctionObj *function = frame->function;
        // vm will increse pc after read an instruction
        int offset = frame->pc - function->chunk.code - 1;
        int line = function->chunk.line_info[offset];
        int column = function->chunk.column_info[offset];
        fprintf(stderr, "[line %d, column %d] in ", line, column);
        if (function->name == NULL) fprintf(stderr, "script\n"); 
        else fprintf(stderr, "%s\n", function->name->str);
    }

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

static void define_native(const char *name, native_func native) {
    StringObj *native_name = new_string(name, strlen(name));
    table_put(native_name, OBJ_VALUE(new_native(native, native_name)), &vm.globals);
}


static Value native_clock(int argc, Value *args) {
    return NUMBER_VALUE((double)clock() / CLOCKS_PER_SEC);
}