#ifndef HASHMAP_H
#define HASHMAP_H

struct MapEntry {
    const char *key;
    void *val;
};

struct HashMap {
    struct MapEntry *buckets;
    int cap;
    int used;
};

struct MapEntry *HashMapInsert(struct HashMap *map, const char *key, void *data);
struct MapEntry *HashMapLookup(const struct HashMap *map, const char *key);
void HashMapPrint(const struct HashMap *map);

#endif // _H
