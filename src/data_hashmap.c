#include "data_hashmap.h"
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

static void resize(struct data_hashmap *map)
{
    int cap = map->cap < 16 ? 16 : 2 * map->cap;
    map->buckets = calloc(cap, sizeof(map->buckets[0]));
    map->cap = cap;
    map->used = 0;
}

static void rehash(struct data_hashmap *map)
{
    struct data_hashmap_entry *old_buckets = map->buckets;
    int old_cap = map->cap;
    resize(map);

    for (int i = 0; i < old_cap; i++) {
        struct data_hashmap_entry *ent = &old_buckets[i];
        data_hashmap_insert(map, ent->key, ent->val);
    }
    free(old_buckets);
}

static bool match(const struct data_hashmap_entry *ent, const char *key)
{
    return !strcmp(ent->key, key);
}

struct data_hashmap_entry *data_hashmap_insert(struct data_hashmap *map,
        const char *key, void *data)
{
    if (!key)
        return NULL;

    if (!map->buckets)
        resize(map);
    else if (100 * map->used / map->cap >= MAX_LOAD_FACTOR)
        rehash(map);

    uint64_t hash = fnv_hash(key);

    for (int i = 0; i < map->cap; i++) {
        struct data_hashmap_entry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->key) {
            ent->key = key;
            ent->val = data;
            map->used++;
            return ent;
        }
        else if (match(ent, key)) {
            return NULL;
        }
    }
    return NULL;
}

struct data_hashmap_entry *data_hashmap_lookup(const struct data_hashmap *map,
        const char *key)
{
    if (!map || !key)
        return NULL;

    uint64_t hash = fnv_hash(key);

    for (int i = 0; i < map->cap; i++) {
        struct data_hashmap_entry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->key)
            return NULL;
        else if (match(ent, key))
            return ent;
    }
    return NULL;
}

int data_hashmap_get_count(const struct data_hashmap *map)
{
    return map->used;
}

void data_hashmap_print(const struct data_hashmap *map)
{
    for (int i = 0; i < map->cap; i++) {
        struct data_hashmap_entry *ent = &map->buckets[i];
        if (ent->key)
            printf( "%4d: key => \"%s\", val => %p\n", i, ent->key, ent->val);
    }
}

void data_hashmap_free(struct data_hashmap *map)
{
    /* keys are allocated outside of hashmap */
    free(map->buckets);
    map->buckets = NULL;
    map->cap = 0;
    map->used = 0;
}
