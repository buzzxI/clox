#ifndef clox_value_h
#define clox_value_h
#include "common.h"

#define NUMBER_VALUE(value) ((Value){VAL_NUMBER, {.number = (value)}})
#define BOOL_VALUE(value)   ((Value){VAL_BOOL, {.boolean = (value)}})
#define NIL_VALUE           ((Value){VAL_NIL, {.number = 0}})
#define OBJ_VALUE(value)    ((Value){VAL_OBJ, {.obj = (Obj*)(value)}})

#define AS_NUMBER(value)    ((value).data.number)
#define AS_BOOL(value)      ((value).data.boolean)
#define AS_OBJ(value)       ((value).data.obj)

#define IS_NUMBER(value)    ((value).type == VAL_NUMBER)
#define IS_BOOL(value)      ((value).type == VAL_BOOL)
#define IS_NIL(value)       ((value).type == VAL_NIL)
#define IS_OBJ(value)       ((value).type == VAL_OBJ)

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

typedef struct Value {
    ValueType type;
    union {
        bool boolean;
        double number;
        Obj *obj; 
    } data;
} Value;

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

#endif  // clox_value_h