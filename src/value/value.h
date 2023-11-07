#ifndef clox_value_h
#define clox_value_h
#include "common.h"

typedef double Value;

typedef struct {
    Value *values;  // constant pool ? 
    int capacity;   // size of memory allocated
    int count;      // size of memory used
} ValueArray;

void init_value_array(ValueArray *array);
void write_value_array(ValueArray *array, Value value);
void free_value_array(ValueArray *array);

#endif  // clox_value_h