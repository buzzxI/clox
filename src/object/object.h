#ifndef clox_object_h
#define clox_object_h
#include "common.h"
#include "value/value.h"
#include "chunk/chunk.h"
#include "table/table.h"

#define OBJ_TYPE(value)     (AS_OBJ(value)->type)
#define IS_STRING(value)    (isObjType(value, OBJ_STRING))
#define IS_FUNCTION(value)  (isObjType(value, OBJ_FUNCTION))
#define IS_NATIVE(value)    (isObjType(value, OBJ_NATIVE))
#define IS_CLOSURE(value)   (isObjType(value, OBJ_CLOSURE))
#define IS_UPVALUE(value)   (isObjType(value, OBJ_UPVALUE))
#define IS_CLASS(value)     (isObjType(value, OBJ_CLASS))
#define IS_INSTANCE(value)  (isObjType(value, OBJ_INSTANCE))
#define IS_METHOD(value)    (isObjType(value, OBJ_METHOD))

#define AS_STRING(value)    ((StringObj*)AS_OBJ(value))
#define AS_CSTRING(value)   (((StringObj*)AS_OBJ(value))->str)
#define AS_FUNCTION(value)  ((FunctionObj*)AS_OBJ(value))
#define AS_NATIVE(value)    ((NativeObj*)AS_OBJ(value))
#define AS_CLOSURE(value)   ((ClosureObj*)AS_OBJ(value))
#define AS_UPVALUE(value)   ((UpvalueObj*)AS_OBJ(value))
#define AS_CLASS(value)     ((ClassObj*)AS_OBJ(value))
#define AS_INSTANCE(value)  ((InstanceObj*)AS_OBJ(value))
#define AS_METHOD(value)    ((MethodObj*)AS_OBJ(value))

typedef Value (*native_func)(int argc, Value *args);

typedef enum {
    OBJ_STRING,
    OBJ_FUNCTION,
    OBJ_NATIVE,
    OBJ_CLOSURE,
    OBJ_UPVALUE,
    OBJ_CLASS,
    OBJ_INSTANCE,
    OBJ_METHOD,
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

struct ClassObj {
    Obj obj;
    StringObj *name;
    Table methods;
};

struct InstanceObj {
    Obj obj;
    ClassObj *klass;
    Table fields;
};

struct MethodObj {
    Obj obj;
    // type of receiver must be InstanceObj
    Value receiver;
    ClosureObj *closure;
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
ClassObj *new_class(StringObj *name);
InstanceObj *new_instance(ClassObj *klass);
MethodObj *new_method(Value receiver, ClosureObj *closure);

void free_obj(Obj *obj);
void free_objs();

#endif