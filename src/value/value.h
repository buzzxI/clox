#ifndef clox_value_h
#define clox_value_h
#include "common.h"

#define NUMBER_VALUE(value) ((Value){VAL_NUMBER, {.number = value}})
#define BOOL_VALUE(value) ((Value){VAL_BOOL, {.boolean = value}})
#define NIL_VALUE ((Value){VAL_NIL, {.number = 0}})

#define IS_NUMBER(value) ((value).type == VAL_NUMBER)
#define IS_BOOL(value) ((value).type == VAL_BOOL)
#define IS_NIL(value) ((value).type == VAL_NIL)

typedef enum {
    VAL_BOOL,
    VAL_NUMBER,
    VAL_NIL,
} ValueType;

typedef struct {
    ValueType type;
    union {
        bool boolean;
        double number;
    } data;
} Value;


typedef struct {
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