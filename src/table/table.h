#ifndef clox_hash_table_h
#define clox_hash_table_h

#include "common.h"
#include "value/value.h"

typedef struct {
    StringObj* key;
    Value value;
} Entry;

typedef struct {
    int count;
    int capacity;
    Entry* entries;
} Table;


void init_table(Table *table);
void free_table(Table *table);
bool table_put(StringObj *key, Value value, Table *table);
bool table_get(StringObj *key, Value *value, Table *table);
bool table_remove(StringObj *key, Value *value, Table *table);
void table_put_all(Table *dest, Table *src);
StringObj* table_find_string(const char *str, int length, uint32_t hash, Table *table);

#endif // clox_hash_table_h