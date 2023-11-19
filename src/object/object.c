#include "object.h"
#include "memory/memory.h"
#include "vm/vm.h"

// added for memcpy
#include <string.h>
// added for print obj
#include <stdio.h>

static Obj* new_obj(ObjType type, size_t size);
static void free_obj(Obj *obj);

void print_obj(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING:
            printf("%s", AS_CSTRING(value));
            break;
        case OBJ_INSTANCE:
            printf("<instance>");
            break;
    }
} 

bool objs_equal(Value a, Value b) {
    if (OBJ_TYPE(a) != OBJ_TYPE(b)) return false;
    switch (OBJ_TYPE(a)) {
        case OBJ_STRING: {
            StringObj *a_str = AS_STRING(a);
            StringObj *b_str = AS_STRING(b);
            return a_str->length == b_str->length && memcmp(a_str->str, b_str->str, a_str->length) == 0;
        }
        case OBJ_INSTANCE:
            return false;
    }
    return false;
}

StringObj* new_string(const char *str, int length) {
    StringObj *string = (StringObj*)new_obj(OBJ_STRING, sizeof(StringObj));
    string->length = length;
    string->str = ALLOCATE(char, length + 1);
    memcpy(string->str, str, length);
    string->str[length] = '\0';
    return string;
}

Value append_string(Value a, Value b) {
    StringObj *string = (StringObj*)new_obj(OBJ_STRING, sizeof(StringObj));
    StringObj *a_str = AS_STRING(a);
    StringObj *b_str = AS_STRING(b);
    string->length = a_str->length + b_str->length;
    string->str = ALLOCATE(char, string->length + 1);
    memcpy(string->str, a_str->str, a_str->length);
    memcpy(string->str + a_str->length, b_str->str, b_str->length);
    string->str[string->length] = '\0';
    return OBJ_VALUE(string);
}

void free_objs() {
    Obj *objs = vm.objs;
    while (objs) {
        Obj *next = objs->next;
        free_obj(objs);
        objs = next;
    }
    vm.objs = NULL;
}

static Obj* new_obj(ObjType type, size_t size) {
    Obj *obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->next = vm.objs;
    vm.objs = obj;
    return obj;
}

static void free_obj(Obj *obj) {
    switch (obj->type) {
        case OBJ_STRING:
            StringObj *string = (StringObj*)obj;
            FREE_ARRAY(char, string->str, string->length + 1);
            FREE(StringObj, obj);
            break;
        case OBJ_INSTANCE:
            // FREE(, obj);
            break;
        default:
            break;
    }
    
}