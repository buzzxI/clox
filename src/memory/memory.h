#ifndef clox_memory_h
#define clox_memory_h

#include "common.h"

#define GROW_CAPACITY(capacity) ((capacity) < 8 ? 8 : (capacity) << 1)
#define GROW_ARRAY(type, pointer, old_size, new_size)\
        (type*)reallocate((type*)(pointer), sizeof(type) * (old_size), sizeof(type) * (new_size))

void* reallocate(void *pointer, size_t old_size, size_t new_size);

#endif  // clox_memory_h