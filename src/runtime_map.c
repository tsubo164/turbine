#include "runtime_map.h"
#include "runtime_string.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* hash map */
struct runtime_map_entry {
    struct runtime_value key;
    struct runtime_value val;
    bool used;
};

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

static void resize(struct runtime_map *map)
{
    int cap = map->cap < 16 ? 16 : 2 * map->cap;
    map->buckets = calloc(cap, sizeof(map->buckets[0]));
    map->cap = cap;
    map->used = 0;
}

static struct runtime_map_entry *insert(struct runtime_map *map,
        struct runtime_value key, struct runtime_value val);

static void rehash(struct runtime_map *map)
{
    struct runtime_map_entry *old_buckets = map->buckets;
    int old_cap = map->cap;
    resize(map);

    for (int i = 0; i < old_cap; i++) {
        struct runtime_map_entry *ent = &old_buckets[i];
        insert(map, ent->key, ent->val);
    }
    free(old_buckets);
}

static bool match(const struct runtime_map_entry *ent, struct runtime_value key)
{
    return !runtime_string_compare(ent->key.string, key.string);
}

static struct runtime_map_entry *insert(struct runtime_map *map,
        struct runtime_value key, struct runtime_value val)
{
    if (!key.string)
        return NULL;

    if (!map->buckets)
        resize(map);
    else if (100 * map->used / map->cap >= MAX_LOAD_FACTOR)
        rehash(map);

    uint64_t hash = fnv_hash(runtime_string_get_cstr(key.string));

    for (int i = 0; i < map->cap; i++) {
        struct runtime_map_entry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->used) {
            ent->key = key;
            ent->val = val;
            ent->used = true;
            map->used++;
            return ent;
        }
        else if (match(ent, key)) {
            /*
            return NULL;
            */
            /* TODO consider not writing a new value here */
            ent->val = val;
        }
    }
    return NULL;
}

static struct runtime_map_entry *lookup(const struct runtime_map *map,
        struct runtime_value key)
{
    if (!map)
        return NULL;

    uint64_t hash = fnv_hash(runtime_string_get_cstr(key.string));

    for (int i = 0; i < map->cap; i++) {
        struct runtime_map_entry *ent = &map->buckets[(hash + i) % map->cap];

        if (!ent->used)
            return NULL;
        else if (match(ent, key))
            return ent;
    }
    return NULL;
}

struct runtime_map *runtime_map_new(int64_t len)
{
    struct runtime_map *m;

    m = calloc(1, sizeof(*m));
    m->obj.kind = OBJ_MAP;

    return m;
}

void runtime_map_free(struct runtime_map *m)
{
    if (!m)
        return;
    free(m->buckets);
    free(m);
}

struct runtime_value runtime_map_get(const struct runtime_map *m, struct runtime_value key)
{
    struct runtime_map_entry *ent = lookup(m, key);
    if (ent)
        return ent->val;
    else
        return (struct runtime_value) {0};
}

void runtime_map_set(struct runtime_map *m, struct runtime_value key, struct runtime_value val)
{
    insert(m, key, val);
}

int64_t runtime_map_len(const struct runtime_map *m)
{
    return m->used;
}

void print_map(const struct runtime_map *map)
{
    for (int i = 0; i < map->cap; i++) {
        struct runtime_map_entry *ent = &map->buckets[i];
        if (ent->used)
            printf( "%4d: key => \"%s\", val => %lld\n", i,
                    runtime_string_get_cstr(ent->key.string),
                    ent->val.inum);
    }
}
