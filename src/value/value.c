#include "memory.h"
#include "value.h"

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