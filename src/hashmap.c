#include "hashmap.h"
#include <stdbool.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <stdio.h>

#define MAX_LOAD_FACTOR 70

static uint64_t fnv_hash(const char *key)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (const char *p = key; *p; p++) {
        hash ^= *p;
        hash *= 0x100000001b3;
    }
    return hash;
}

static void resize(HashMap *map)
{
    int cap = map->cap < 16 ? 16 : 2 * map->cap;
    map->buckets = calloc(cap, sizeof(map->buckets[0]));
    map->cap = cap;
    map->used = 0;
}

static void rehash(HashMap *map)
{
    MapEntry *old_buckets = map->buckets;
    int old_cap = map->cap;
    resize(map);

    for (int i = 0; i < old_cap; i++) {
        MapEntry *ent = &old_buckets[i];
        HashmapInsert(map, ent->key, ent->val);
    }
    free(old_buckets);
}

static bool match(const MapEntry *ent, const char *key)
{
    return !strcmp(ent->key, key);
}

void *HashmapInsert(HashMap *map, const char *key, void *data)
{
    if (!key || !data)
        return NULL;

    if (!map->buckets)
        resize(map);
    else if (100 * map->used / map->cap >= MAX_LOAD_FACTOR)
        rehash(map);

    uint64_t hash = fnv_hash(key);

    for (int i = 0; i < map->cap; i++) {
        MapEntry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->key) {
            ent->key = key;
            ent->val = data;
            map->used++;
            return ent->val;
        }
        else if (match(ent, key)) {
            return NULL;
        }
    }
    return NULL;
}

void *HashmapLookup(HashMap *map, const char *key)
{
    if (!map || !key)
        return NULL;

    int64_t hash = fnv_hash(key);

    for (int i = 0; i < map->cap; i++) {
        MapEntry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->key)
            return NULL;
        else if (match(ent, key))
            return ent->val;
    }
    return NULL;
}

void HashmapPrint(const HashMap *map)
{
    for (int i = 0; i < map->cap; i++) {
        MapEntry *ent = &map->buckets[i];
        if (ent->key)
            printf( "%4d: key => \"%s\", val => %p\n", i, ent->key, ent->val);
    }
}
