#include "memory.h"
#include "vm/vm.h"
#include "complier/compiler.h"
// added for realloc
#include <stdlib.h>
#ifdef CLOX_DEBUG_LOG_GC
// added for printf
#include <stdio.h>
#endif // CLOX_DEBUG_LOG_GC
#define CLOX_GC_HEAP_GROW_FACTOR 2

static void collect_garbage();
static void mark_roots();
static void mark_value(Value *value);
static void mark_table(Table *table);
static void traverse_references();
static void black_object(Obj *obj);
static void mark_array(ValueArray *array);
static void sweep();
static void remove_table_white(Table *table);

void* reallocate(void *ptr, size_t old_size, size_t new_size) {
    vm.allocated_bytes += new_size - old_size;

#ifdef CLOX_DEBUG_STRESS_GC
    if (new_size > old_size) collect_garbage();
#else
    if (vm.allocated_bytes > vm.next_gc) collect_garbage();
#endif // CLOX_DEBUG_STRESS_GC

    if (new_size == 0) {
        free(ptr);
        return NULL;
    } 
    void *rst = realloc(ptr, new_size);
    // returns on run out of memory
    if (rst == NULL) exit(1);
    return rst;
}

/// @brief mark and sweep garbage collector
void collect_garbage() {
#ifdef CLOX_DEBUG_LOG_GC
    size_t before = vm.allocated_bytes;
    printf("== clox gc begin ==\n");
#endif // CLOX_DEBUG_LOG_GC
    // mark all reachable objects
    mark_roots();
    // traverse from gray stack
    traverse_references();
    // string table are interned
    remove_table_white(&vm.strings);
    // sweep unreachable objects
    sweep();
    // update threshold after gc
    vm.next_gc = vm.allocated_bytes * CLOX_GC_HEAP_GROW_FACTOR;
#ifdef CLOX_DEBUG_LOG_GC
    printf("== clox gc end == \n");
    printf("collected %zu bytes (from %zu to %zu) next at %zu\n", before - vm.allocated_bytes, before, vm.allocated_bytes, vm.next_gc); 
#endif // CLOX_DEBUG_LOG_GC
}

void mark_obj(Obj *obj) {
    if (obj == NULL) return;
    // erase circular reference
    if (obj->is_marked) return;
    obj->is_marked = true;
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p mark ", (void*)obj);
    print_obj(OBJ_VALUE(obj));
    printf("\n");
#endif // CLOX_DEBUG_LOG_GC

    // push object into gray stack
    vm.gray_stack[vm.gray_count++] = obj;
}

static void mark_roots() {
    // objects in stack are roots
    for (Value *cur = vm.stack; cur < vm.sp; cur++) mark_value(cur);
    // objects in globals are roots
    mark_table(&vm.globals);
    // pointers to closure are roots
    for (int i = 0; i < vm.frame_cnt; i++) mark_obj((Obj*)vm.frames[i].closure);
    // mark initializer string
    mark_obj((Obj*)vm.init_string);
    // mark roots for compile time
    mark_compiler_roots();
    // objects in gc stack are roots
    for (int i = 0; i < vm.gc_stack_cnt; i++) mark_value(&vm.gc_stack[i]);
}

static void mark_value(Value *value) {
    if (IS_OBJ(*value)) mark_obj(AS_OBJ(*value));
}

static void mark_table(Table *table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;    
        mark_obj((Obj*)entry->key);
        mark_value(&entry->value);
    }
}

// dfs traverse
static void traverse_references() {
    while (vm.gray_count > 0) {
        Obj* obj = vm.gray_stack[--vm.gray_count];
        black_object(obj);
    }
}

static void black_object(Obj *obj) {
#ifdef CLOX_DEBUG_LOG_GC
    printf("%p black ", (void*)obj);
    print_obj(OBJ_VALUE(obj));
    printf("\n");
#endif // CLOX_DEBUG_LOG_GC
    switch (obj->type) {
        // string obj does not has reference to other objects
        case OBJ_STRING: break;
        // native function has name to mark
        case OBJ_NATIVE: {
            NativeObj *native = (NativeObj*)obj;
            mark_obj((Obj*)native->name);
            break;
        }
        case OBJ_UPVALUE: {
            UpvalueObj *upvalue = (UpvalueObj*)obj;
            // closed may not always valid
            mark_value(&upvalue->close);
            break;
        }
        case OBJ_FUNCTION: {
            FunctionObj *function = (FunctionObj*)obj;
            mark_obj((Obj*)function->name);
            mark_array(&function->chunk.constant);
            break;
        }
        case OBJ_CLOSURE: {
            ClosureObj *closure = (ClosureObj*)obj;
            mark_obj((Obj*)closure->function);
            for (int i = 0; i < closure->upvalue_cnt; i++) mark_obj((Obj*)closure->upvalues[i]);
            break;
        }
        case OBJ_CLASS: {
            ClassObj *klass = (ClassObj*)obj;
            mark_obj((Obj*)klass->name);
            mark_table(&klass->methods);
            break;
        }
        case OBJ_INSTANCE: {
            InstanceObj *instance = (InstanceObj*)obj;
            mark_obj((Obj*)instance->klass);
            mark_table(&instance->fields);
            break;
        }
        case OBJ_METHOD: {
            MethodObj *method = (MethodObj*)obj;
            mark_value(&method->receiver);
            mark_obj((Obj*)method->closure);
            break;
        }
    }
}

static void mark_array(ValueArray *array) {
    for (int i = 0; i < array->count; i++) mark_value(&array->values[i]);
}

static void sweep() {
    Obj *cur = &vm.objs;
    while (cur->next != NULL) {
        Obj *next = cur->next;
        if (next->is_marked) {
            next->is_marked = false;
            cur = cur->next;
            continue;
        }
        cur->next = next->next;
        free_obj(next);
    }
}

static void remove_table_white(Table *table) {
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        // erase dangling pointer
        if (entry->key != NULL && !entry->key->obj.is_marked) table_remove(entry->key, NULL, table); 
    }
}