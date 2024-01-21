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

void *HashmapInsert(HashMap *map, const char *key, void *data);
void *HashmapLookup(HashMap *map, const char *key);
void HashmapPrint(const HashMap *map);

#endif // _H
