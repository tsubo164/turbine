#ifndef HASHMAP_H
#define HASHMAP_H

typedef struct MapEntry {
    const char *key;
    void *val;
} MapEntry;

typedef struct HashMap {
    MapEntry *buckets;
    int cap;
    int used;
} HashMap;

void *HashMapInsert(HashMap *map, const char *key, void *data);
void *HashMapLookup(const HashMap *map, const char *key);
void HashMapPrint(const HashMap *map);

#endif // _H
