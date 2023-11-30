#ifndef clox_object_h
#define clox_object_h
#include "common.h"
#include "value/value.h"
#include "chunk/chunk.h"

#define OBJ_TYPE(value)     ((value).data.obj->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))
#define IS_INSTANCE(value)  (isObjType(value, OBJ_INSTANCE))
#define IS_FUNCTION(value)  (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value)    (isObjType(value, OBJ_NATIVE))

#define AS_STRING(value)    ((StringObj*)(value).data.obj)
#define AS_CSTRING(value)   (((StringObj*)(value).data.obj)->str)
#define AS_FUNCTION(value)  ((FunctionObj*)(value).data.obj)
#define AS_NATIVE(value)    ((NativeObj*)(value).data.obj)

typedef Value (*native_func)(int argc, Value *args);

typedef enum {
    OBJ_STRING,
    OBJ_INSTANCE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
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

struct FunctionObj {
    Obj obj;
    StringObj *name;
    int arity;
    Chunk chunk;
};

struct NativeObj {
    Obj obj;
    native_func native;
    StringObj *name;
};

static inline bool isObjType(Value value, ObjType type) {
    return IS_OBJ(value) && OBJ_TYPE(value) == type;
};

void print_obj(Value value);
bool objs_equal(Value a, Value b);

StringObj *new_string(const char *str, int length);
Value append_string(Value a, Value b);

FunctionObj *new_function();
NativeObj *new_native(native_func func, StringObj *name);

void free_objs();

#endif