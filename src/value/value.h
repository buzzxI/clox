#ifndef clox_value_h
#define clox_value_h
#include "common.h"

#ifdef NAN_BOXING
#include <string.h>

#define QNAN      ((uint64_t)0x7ffc000000000000)
#define SIGN_BIT  ((uint64_t)0x8000000000000000)
#define TAG_NIL   0x1
#define TAG_FALSE 0x2
#define TAG_TRUE  0x3

#define NUMBER_VALUE(value) number_to_value(value) 
#define AS_NUMBER(value)    value_to_number(value)
#define IS_NUMBER(value)    (((value) & QNAN) != QNAN)

#define NIL_VALUE           ((Value)(uint64_t)(QNAN | TAG_NIL))
#define IS_NIL(value)       ((uint64_t)(value) == NIL_VALUE)

#define FALSE_VALUE         ((Value)(uint64_t)(QNAN | TAG_FALSE))
#define TRUE_VALUE          ((Value)(uint64_t)(QNAN | TAG_TRUE))
#define BOOL_VALUE(value)   ((value) ? TRUE_VALUE : FALSE_VALUE)
#define AS_BOOL(value)      ((value) == TRUE_VALUE)
#define IS_BOOL(value)      (((value) | 1) == TRUE_VALUE)

#define OBJ_VALUE(value)    (Value)(SIGN_BIT | QNAN | (uint64_t)(uintptr_t)(value))
#define AS_OBJ(value)       ((Obj*)(uintptr_t)((value) & ~(SIGN_BIT | QNAN)))
#define IS_OBJ(value)       (((value) & (QNAN | SIGN_BIT)) == (QNAN | SIGN_BIT))
#else
#define NUMBER_VALUE(value) ((Value){VAL_NUMBER, {.number = (value)}})
#define AS_NUMBER(value)    ((value).data.number)
#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)

#define BOOL_VALUE(value)   ((Value){VAL_BOOL, {.boolean = (value)}})
#define AS_BOOL(value)      ((value).data.boolean)
#define IS_BOOL(value)      ((value).type == VAL_BOOL)

#define NIL_VALUE           ((Value){VAL_NIL, {.number = 0}})
#define IS_NIL(value)       ((value).type == VAL_NIL)

#define OBJ_VALUE(value)    ((Value){VAL_OBJ, {.obj = (Obj*)(value)}})
#define AS_OBJ(value)       ((value).data.obj)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)
#endif // NAN_BOXING

typedef struct Obj Obj;
typedef struct StringObj StringObj;
typedef struct FunctionObj FunctionObj;
typedef struct NativeObj NativeObj;
typedef struct ClosureObj ClosureObj;
typedef struct UpvalueObj UpvalueObj;
typedef struct ClassObj ClassObj;
typedef struct InstanceObj InstanceObj;
typedef struct MethodObj MethodObj;

typedef enum ValueType {
    VAL_BOOL,
    VAL_NUMBER,
    VAL_NIL,
    VAL_OBJ,
} ValueType;

#ifdef NAN_BOXING
typedef uint64_t Value;
#else
typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj; 
    } data;
} Value;
#endif // NAN_BOXING

typedef struct ValueArray {
    Value *values;  // constant pool ? 
    int capacity;   // size of memory allocated
    int count;      // size of memory used
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);
void print_value(Value value);
bool is_false(Value value);
bool values_equal(Value a, Value b);

#ifdef NAN_BOXING
static inline Value number_to_value(double value) {
    // Value rst = 0;
    // memcpy(&rst, &value, sizeof(double));
    // return rst;
    union {
      uint64_t bits;
      double num;
    } data;
    data.num = value;
    return data.bits;
} 

static inline double value_to_number(Value value) {
    union {
      uint64_t bits;
      double num;
    } data;
    data.bits = value;
    return data.num;
}
#endif // NAN_BOXING

#endif  // clox_value_h