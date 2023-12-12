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
#define IS_CLOSURE(value)   (isObjType(value, OBJ_CLOSURE))
#define IS_UPVALUE(value)   (isObjType(value, OBJ_UPVALUE))

#define AS_STRING(value)    ((StringObj*)(value).data.obj)
#define AS_CSTRING(value)   (((StringObj*)(value).data.obj)->str)
#define AS_FUNCTION(value)  ((FunctionObj*)(value).data.obj)
#define AS_NATIVE(value)    ((NativeObj*)(value).data.obj)
#define AS_CLOSURE(value)   ((ClosureObj*)(value).data.obj)
#define AS_UPVALUE(value)   ((UpvalueObj*)(value).data.obj)

typedef Value (*native_func)(int argc, Value *args);

typedef enum {
    OBJ_STRING,
    OBJ_INSTANCE,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
} ObjType;

struct Obj {
    ObjType type;
    bool is_marked;
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
    int upvalue_cnt;
};

struct NativeObj {
    Obj obj;
    native_func native;
    StringObj *name;
};

struct ClosureObj {
    Obj obj;
    FunctionObj *function;
    UpvalueObj **upvalues;
    int upvalue_cnt;
};

struct UpvalueObj {
    Obj obj;
    Value *location;
    Value close;
    UpvalueObj *next;
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
ClosureObj *new_closure(FunctionObj *function);
UpvalueObj *new_upvalue(Value *slot);

void free_obj(Obj *obj);
void free_objs();

#endif