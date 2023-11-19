#include "value.h"
#include "memory/memory.h"
#include "object/object.h"
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
            printf(AS_BOOL(value) ? "true" : "false");
            break;
        case VAL_NUMBER:
            printf("%g", AS_NUMBER(value));
            break;
        case VAL_NIL:
            printf("nil");
            break;
        case VAL_OBJ:
            print_obj(value);
            break;
    }
}

bool is_false(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool values_equal(Value a, Value b) {
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_NIL: return true;
        case VAL_OBJ: return objs_equal(a, b);
    }
    return false;
}