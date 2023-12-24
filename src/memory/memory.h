#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"
#include "value/value.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) << 1)
#define GROW_ARRAY(type, pointer, old_size, new_size)\
        (type*)reallocate((type*)(pointer), sizeof(type) * (old_size), sizeof(type) * (new_size))
#define FREE_ARRAY(type, pointer, old_size)\
        (type*)reallocate((type*)(pointer), old_size, 0)
#define ALLOCATE(type, size)\
        (type*)reallocate(NULL, 0, sizeof(type) * (size))
#define FREE(type, pointer)\
        (type*)reallocate((type*)(pointer), sizeof(type), 0)

void* reallocate(void *pointer, size_t old_size, size_t new_size);
void mark_obj(Obj *obj);

#endif  // clox_memory_h