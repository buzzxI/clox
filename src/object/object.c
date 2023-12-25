#include "object.h"
#include "memory/memory.h"
#include "vm/vm.h"

// added for memcpy
#include <string.h>
// added for print obj
#include <stdio.h>
// added for free
#include <stdlib.h>

static Obj* new_obj(ObjType type, size_t size);
static StringObj* take_string(const char *str, int length);
static uint32_t hash_string(const char *str, int length);

void print_obj(Value value) {
    switch (OBJ_TYPE(value)) {
        case OBJ_STRING: {
            printf("%s", AS_CSTRING(value));
            break;
        }
        case OBJ_FUNCTION: {
            FunctionObj *function = AS_FUNCTION(value);
            if (function->name == NULL) printf("<clox script>");
            else printf("<clox function %s>", function->name->str);
            break;
        }
        case OBJ_NATIVE: {
            printf("<clox native %s>", AS_NATIVE(value)->name->str);
            break;
        }
        case OBJ_CLOSURE: {
            ClosureObj *closure = AS_CLOSURE(value);
            if (closure->function->name == NULL) printf("<clox closure script>");
            else printf("<clox closure %s>", closure->function->name->str);
            break;
        }
        case OBJ_UPVALUE: {
            printf("<upvalue>");
            break;
        }
        case OBJ_CLASS: {
            printf("<clox class %s>", AS_CLASS(value)->name->str);
            break;
        }
        case OBJ_INSTANCE: {
            InstanceObj *instance = AS_INSTANCE(value);
            printf("<clox instance %s>", instance->klass->name->str);
            break;
        }
        case OBJ_METHOD: {
            MethodObj *method = AS_METHOD(value);
            printf("<clox method %s>", method->closure->function->name->str);
            break;
        }
    }
} 

bool objs_equal(Value a, Value b) {
    if (OBJ_TYPE(a) != OBJ_TYPE(b)) return false;
    switch (OBJ_TYPE(a)) {
        case OBJ_STRING:    return AS_STRING(a) == AS_STRING(b);
        case OBJ_FUNCTION:  return AS_FUNCTION(a) == AS_FUNCTION(b);
        case OBJ_NATIVE:    return AS_NATIVE(a) == AS_NATIVE(b);
        case OBJ_CLOSURE:   return AS_CLOSURE(a) == AS_CLOSURE(b);
        case OBJ_UPVALUE:   return AS_CLOSURE(a) == AS_CLOSURE(b);
        case OBJ_CLASS:     return AS_CLASS(a) == AS_CLASS(b);
        case OBJ_INSTANCE:  return AS_INSTANCE(a) == AS_INSTANCE(b);
        case OBJ_METHOD:    return AS_METHOD(a) == AS_METHOD(b);
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

FunctionObj* new_function() {
    FunctionObj *function = (FunctionObj*)new_obj(OBJ_FUNCTION, sizeof(FunctionObj));
    function->arity = 0;
    function->name = NULL;
    init_chunk(&function->chunk);
    function->upvalue_cnt = 0;
    return function;
}

NativeObj* new_native(native_func func, StringObj *name) {
    NativeObj *native = (NativeObj*)new_obj(OBJ_NATIVE, sizeof(NativeObj));
    native->native = func;
    native->name = name;
    return native;
}

ClosureObj* new_closure(FunctionObj *function) {
    // make sure to allocate upvalues first => upvalues will not be liked to objects list => it cannot be reaped by gc
    UpvalueObj **upvalues = ALLOCATE(UpvalueObj*, function->upvalue_cnt);
    for (int i = 0; i < function->upvalue_cnt; i++) upvalues[i] = NULL; 
    ClosureObj *closure = (ClosureObj*)new_obj(OBJ_CLOSURE, sizeof(ClosureObj));
    closure->function = function;
    closure->upvalue_cnt = function->upvalue_cnt;
    closure->upvalues = upvalues;
    return closure;
}

UpvalueObj* new_upvalue(Value *slot) {
    UpvalueObj *head = &vm.upvalues;
    UpvalueObj *cur = head;
    while (cur->next != NULL && cur->next->location > slot) cur = cur->next;
    if (cur->next != NULL && cur->next->location == slot) return cur->next;

    UpvalueObj *upvalue = (UpvalueObj*)new_obj(OBJ_UPVALUE, sizeof(UpvalueObj));
    upvalue->location = slot;
    upvalue->close = NIL_VALUE;
    upvalue->next = cur->next;
    cur->next = upvalue;
    return upvalue;
}

ClassObj* new_class(StringObj *name) {
    ClassObj *class = (ClassObj*)new_obj(OBJ_CLASS, sizeof(ClassObj));
    class->name = name;
    init_table(&class->methods);
    return class;
}

InstanceObj* new_instance(ClassObj *klass) {
    InstanceObj *instance = (InstanceObj*)new_obj(OBJ_INSTANCE, sizeof(InstanceObj));
    instance->klass = klass;
    init_table(&instance->fields);
    return instance;
}

MethodObj* new_method(Value receiver, ClosureObj *method) {
    MethodObj *obj = (MethodObj*)new_obj(OBJ_METHOD, sizeof(MethodObj));
    obj->receiver = receiver;
    obj->closure = method;
    return obj;
}

void free_objs() {
    Obj *cur = &vm.objs;
    while (cur->next != NULL) {
        Obj *next = cur->next;
        cur->next = next->next;
        free_obj(next);
    }
}

static Obj* new_obj(ObjType type, size_t size) {
    Obj *obj = (Obj*)reallocate(NULL, 0, size);
    obj->type = type;
    obj->is_marked = false;
    obj->next = vm.objs.next;
    vm.objs.next = obj;
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p allocate %zu for %d\n", (void*)obj, size, type);
#endif // CLOX_DEBUG_LOG_GC
    return obj;
}

void free_obj(Obj *obj) {
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p free type %d\n", (void*)obj, obj->type);
#endif // CLOX_DEBUG_LOX_GC
    switch (obj->type) {
        case OBJ_STRING: {
            StringObj *string = (StringObj*)obj;
            FREE_ARRAY(char, string->str, string->length + 1);
            FREE(StringObj, obj);
            break;
        }
        case OBJ_FUNCTION: {
            FunctionObj *function = (FunctionObj*)obj;
            free_chunk(&function->chunk);
            FREE(FunctionObj, obj);
            break;
        }
        case OBJ_NATIVE: {
            FREE(NativeObj, obj);
            break;
        }
        case OBJ_CLOSURE: {
            ClosureObj *closure = (ClosureObj*)obj;
            FREE_ARRAY(UpvalueObj*, closure->upvalues, closure->upvalue_cnt);
            FREE(ClosureObj, obj);
            break;
        }
        case OBJ_UPVALUE: {
            FREE(UpvalueObj, obj);
            break;
        }
        case OBJ_CLASS: {
            ClassObj *class = (ClassObj*)obj;
            free_table(&class->methods);
            FREE(ClassObj, obj);
            break;
        }
        case OBJ_INSTANCE: {
            InstanceObj *instance = (InstanceObj*)obj;
            free_table(&instance->fields);
            FREE(InstanceObj, obj);
            break;
        }
        case OBJ_METHOD: {
            FREE(MethodObj, obj);
            break;
        }
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
    // table_put may trigger gc
    push_gc(OBJ_VALUE(string));
    table_put(string, NIL_VALUE, &vm.strings);
    pop_gc();
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