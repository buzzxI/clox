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
#ifdef NAN_BOXING
    if (IS_BOOL(value)) printf(AS_BOOL(value) ? "true" : "false");
    else if (IS_NIL(value)) printf("nil");
    else if (IS_NUMBER(value)) printf("%g", AS_NUMBER(value));
    else if (IS_OBJ(value)) print_obj(value);
#else
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
#endif // NAN_BOXING
}

bool is_false(Value value) {
    return IS_NIL(value) || (IS_BOOL(value) && !AS_BOOL(value));
}

bool values_equal(Value a, Value b) {
#ifdef NAN_BOXING
    // NaN is not equal to NaN
    if (IS_NUMBER(a) && IS_NUMBER(b)) return AS_NUMBER(a) == AS_NUMBER(b);
    return a == b;
#else
    if (a.type != b.type) return false;
    switch (a.type) {
        case VAL_BOOL: return AS_BOOL(a) == AS_BOOL(b);
        case VAL_NUMBER: return AS_NUMBER(a) == AS_NUMBER(b);
        case VAL_NIL: return true;
        case VAL_OBJ: return objs_equal(a, b);
    }
#endif // NAN_BOXING
    
    return false;
}