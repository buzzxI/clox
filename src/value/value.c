#include "value.h"
#include "memory/memory.h"
// include for printf
#include <stdio.h>

void init_value_array(ValueArray *array) {
    array->capacity = 0;
    array->count = 0;
    array->values = NULL;
}

void write_value_array(ValueArray *array, Value value) {
    if (array->count + 1 > array->capacity) {
        int new_capacity = GROW_CAPACITY(array->capacity);
        array->values = GROW_ARRAY(Value, array->values, array->capacity, new_capacity);
        array->capacity = new_capacity;
    }
    array->values[array->count] = value;
    array->count++;
}

void free_value_array(ValueArray *array) {
    FREE_ARRAY(Value, array->values, array->capacity);
    init_value_array(array);
}

void print_value(Value value) {
    switch (value.type) {
        case VAL_BOOL:
            printf(value.data.boolean ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%g", value.data.number);
            break;
        case VAL_NIL:
            printf("nil");
            break;
    }
}

bool is_false(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !value.data.boolean);
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL: return a.data.boolean == b.data.boolean;
        case VAL_NUMBER: return a.data.number == b.data.number;
        case VAL_NIL: return true;
    }
    return false;
}