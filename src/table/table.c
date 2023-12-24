#include "table.h"
#include "memory/memory.h"
#include "object/object.h"
// added for memcmp
#include <string.h>

#define TABLE_LOAD 0.75

static Entry* find_entry_by_hash(StringObj *key, Entry *entries, int size);
static void rehash_table(Table *table);


void init_table(Table *table) {
    table->capacity = 0;
    table->count = 0;
    table->entries = NULL;
}

void free_table(Table *table) {
    FREE_ARRAY(Entry, table->entries, table->capacity);
    init_table(table);
}

bool table_put(StringObj *key, Value value, Table *table) {
    // resize before filling up
    if (table->count + 1 > (double)table->capacity * TABLE_LOAD) rehash_table(table);

    Entry *entry = find_entry_by_hash(key, table->entries, table->capacity);
    if (entry == NULL) return false;
    if (entry->key == NULL) table->count++;
    entry->key = key;
    entry->value = value;
    return true;
}

bool table_get(StringObj *key, Value *value, Table *table) {
    if (table->count == 0) return false;

    Entry *entry = find_entry_by_hash(key, table->entries, table->capacity);
    if (entry == NULL || entry->key == NULL) return false;
    if (value != NULL) *value = entry->value;
    return true;
}

bool table_remove(StringObj *key, Value *value, Table *table) {
    if (table->count == 0) return false;

    Entry *entry = find_entry_by_hash(key, table->entries, table->capacity);
    if (entry == NULL || entry->key == NULL) return false;
    if (value != NULL) *value = entry->value;

    table->count--;
    entry->key = NULL;
    entry->value = BOOL_VALUE(true);
    return true;
}

// add all entries from src to dest
void table_put_all(Table *dest, Table *src) {
    for (int i = 0; i < src->capacity; i++) {
        Entry *entry = &src->entries[i];
        if (entry->key == NULL) continue;
        table_put(entry->key, entry->value, dest);
    }
}

StringObj* table_find_string(const char *str, int length, uint32_t hash, Table *table) {
    if (table->count == 0) return NULL;

    uint32_t idx = hash & (table->capacity - 1);
    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[(idx + i) & (table->capacity - 1)];
        if (entry->key != NULL) {
            if (entry->key->length == length && entry->key->hash == hash && memcmp(entry->key->str, str, length) == 0) return entry->key;
        } else if (entry->value.type == VAL_NIL) return NULL;
    }
    return NULL;
}

static Entry* find_entry_by_hash(StringObj *key, Entry *entries, int size) {
    // optimize modulo operation by bitwise AND
    uint32_t idx = key->hash & (size - 1);
    Entry *tombstone = NULL;
    for (int i = 0; i < size; i++) {
        Entry *entry = &entries[(idx + i) & (size - 1)];
        if (entry->key == key) return entry;
        else if (entry->key == NULL) {
            if (entry->value.type == VAL_NIL) return tombstone == NULL ? entry : tombstone;
            else if (tombstone == NULL) tombstone = entry;
        }
    }
    return tombstone;
}

static void rehash_table(Table *table) {
    int new_capacity = GROW_CAPACITY(table->capacity);
    Entry *new_entries = ALLOCATE(Entry, new_capacity);
    for (int i = 0; i < new_capacity; i++) {
        new_entries[i].key = NULL;
        new_entries[i].value = NIL_VALUE;
    }

    for (int i = 0; i < table->capacity; i++) {
        Entry *entry = &table->entries[i];
        if (entry->key == NULL) continue;
        Entry *dest = find_entry_by_hash(entry->key, new_entries, new_capacity);
        dest->key = entry->key;
        dest->value = entry->value;
    }

    FREE_ARRAY(Entry, table->entries, table->capacity);

    table->capacity = new_capacity;
    table->entries = new_entries; 
}
