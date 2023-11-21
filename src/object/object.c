#include "object.h"
#include "memory/memory.h"
#include "vm/vm.h"

// added for memcpy
#include <string.h>
// added for print obj
#include <stdio.h>

static Obj* new_obj(ObjType type, size_t size);
static void free_obj(Obj *obj);
static StringObj* take_string(const char *str, int length);
static uint32_t hash_string(const char *str, int length);

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
        case OBJ_STRING: return AS_STRING(a) == AS_STRING(b);
        case OBJ_INSTANCE: return false;
    }
    return false;
}

StringObj* new_string(const char *str, int length) {
    char *heap_str = ALLOCATE(char, length + 1);
    memcpy(heap_str, str, length);
    heap_str[length] = '\0';
    return take_string(heap_str, length);  
}

Value append_string(Value a, Value b) {
    StringObj *a_str = AS_STRING(a);
    StringObj *b_str = AS_STRING(b);
    int length = a_str->length + b_str->length;
    char *heap_str = ALLOCATE(char, length + 1);
    memcpy(heap_str, a_str->str, a_str->length);
    memcpy(heap_str + a_str->length, b_str->str, b_str->length);
    heap_str[length] = '\0';
    return OBJ_VALUE(take_string(heap_str, length));
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

static StringObj* take_string(const char *str, int length) {
    uint32_t hash = hash_string(str, length);
    StringObj *interned = table_find_string(str, length, hash, &vm.strings);
    if (interned != NULL) {
        FREE_ARRAY(char, str, length + 1);
        return interned;
    }
    StringObj *string = (StringObj*)new_obj(OBJ_STRING, sizeof(StringObj));
    string->length = length;
    string->str = str;
    string->hash = hash;
    table_put(string, NIL_VALUE, &vm.strings);
    return string;
}

static uint32_t hash_string(const char *str, int length) {
#define FNV_OFFSET_BASIS 0x811c9dc5
#define FNV_PRIME 0x1000193
    uint32_t hash = FNV_OFFSET_BASIS;
    for (int i = 0; i < length; i++) {
        hash ^= str[i];
        hash *= FNV_PRIME;
    }
#undef FNV_OFFSET_BASIS
#undef FNV_PRIME
    return hash;
}