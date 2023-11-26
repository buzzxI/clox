#ifndef clox_object_h
#define clox_object_h
#include "common.h"
#include "value/value.h"
#include "chunk/chunk.h"

#define OBJ_TYPE(value)     ((value).data.obj->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))
#define IS_INSTANCE(value)  (isObjType(value, OBJ_INSTANCE))

#define AS_STRING(value)    ((StringObj*)(value).data.obj)
#define AS_CSTRING(value)   (((StringObj*)(value).data.obj)->str)

typedef enum {
    OBJ_STRING,
    OBJ_INSTANCE,
    ObJ_FUNCTION,
} ObjType;

struct Obj {
    ObjType type;
    Obj *next;
};

struct StringObj {
    Obj obj;
    int length;
    const char *str;
    uint32_t hash;
};

typedef struct {
    Obj obj;
    StringObj *name;
    int arity;
    Chunk code;
} FunctionObj;

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
};

void print_obj(Value value);
bool objs_equal(Value a, Value b);

StringObj *new_string(const char *str, int length);
Value append_string(Value a, Value b);

Function *new_function();

void free_objs();

#endif