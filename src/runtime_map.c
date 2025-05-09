#include "runtime_map.h"
#include "runtime_string.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LOAD_FACTOR 70
#define MIN_PRIME_INDEX 5

const int32_t power_of_two_primes[] = {
    /* 2^0 */  1,
    /* 2^1 */  2,
    /* 2^2 */  3,
    /* 2^3 */  7,
    /* 2^4 */  13,
    /* 2^5 */  31,
    /* 2^6 */  61,
    /* 2^7 */  127,
    /* 2^8 */  251,
    /* 2^9 */  509,
    /* 2^10 */ 1021,
    /* 2^11 */ 2039,
    /* 2^12 */ 4093,
    /* 2^13 */ 8179,
    /* 2^14 */ 16381,
    /* 2^15 */ 32749,
    /* 2^16 */ 65521,
    /* 2^17 */ 131071,
    /* 2^18 */ 262139,
    /* 2^19 */ 524287,
    /* 2^20 */ 1048573,
    /* 2^21 */ 2097143,
    /* 2^22 */ 4194301,
    /* 2^23 */ 8388593,
    /* 2^24 */ 16777199,
    /* 2^25 */ 33554393,
    /* 2^26 */ 67108859,
    /* 2^27 */ 134217689,
    /* 2^28 */ 268435399,
    /* 2^29 */ 536870909,
    /* 2^30 */ 1073741789,
    /* 2^31 */ 2147483647
};

static uint64_t simple_hash(const char *key, int len)
{
    uint64_t h = 0;
    const uint8_t *p = (const uint8_t *) key;

    for (int i = 0; i < len; i++) {
        h = 31 * h + p[i];
    }

    return h;
}

static uint64_t fnv_hash(const char *key, int len)
{
    uint64_t hash = 0xcbf29ce484222325;
    const uint8_t *p = (const uint8_t *) key;

    for (int i = 0; i < len; i++) {
        hash ^= p[i];
        hash *= 0x100000001b3;
    }

    return hash;
}

static int32_t next_prime_index(int index)
{
    return index < MIN_PRIME_INDEX ?  MIN_PRIME_INDEX : index + 1;
}

static int32_t get_prime(int index)
{
    int idx = index < 0 ? 0 : index;
    idx = index > 31 ? 31 : index;

    return power_of_two_primes[idx];
}

static uint64_t get_hash(const struct runtime_map *map, struct runtime_value key)
{
    const char *str = runtime_string_get_cstr(key.string);
    int len = runtime_string_len(key.string);
    uint64_t h;

    if (1)
        h = fnv_hash(str, len);
    else
        h = simple_hash(str, len);

    return h % map->cap;
}

static void resize(struct runtime_map *map, int prime_index)
{
    int cap = get_prime(prime_index);

    map->buckets = calloc(cap, sizeof(map->buckets[0]));
    map->prime_index = prime_index;
    map->cap = cap;
    map->len = 0;
}

static void rehash(struct runtime_map *map)
{
    struct runtime_map_entry **old_buckets = map->buckets;
    int old_cap = map->cap;

    resize(map, next_prime_index(map->prime_index));

    for (int i = 0; i < old_cap; i++) {
        struct runtime_map_entry *ent, *next;
        uint64_t h;

        for (ent = old_buckets[i]; ent; ) {
            next = ent->next_in_chain;

            h = get_hash(map, ent->key);
            ent->next_in_chain = map->buckets[h];
            map->buckets[h] = ent;

            ent = next;
        }
    }

    free(old_buckets);
}

static bool match(const struct runtime_map_entry *ent, struct runtime_value key)
{
    return !runtime_string_compare(ent->key.string, key.string);
}

static struct runtime_map_entry *new_entry(struct runtime_value key, struct runtime_value val)
{
    struct runtime_map_entry *ent;

    ent = calloc(1, sizeof(struct runtime_map_entry));
    ent->key = key;
    ent->val = val;

    return ent;
}

static struct runtime_map_entry *insert(struct runtime_map *map,
        struct runtime_value key, struct runtime_value val)
{
    if (!key.string)
        return NULL;

    if (map->cap == 0)
        resize(map, next_prime_index(0));
    else if (100. * map->len / map->cap >= MAX_LOAD_FACTOR)
        rehash(map);

    uint64_t h = get_hash(map, key);
    struct runtime_map_entry *ent;

    for (ent = map->buckets[h]; ent; ent = ent->next_in_chain) {
        if (match(ent, key)) {
            ent->val = val;
            return ent;
        }
    }

    ent = new_entry(key, val);
    ent->next_in_chain = map->buckets[h];
    map->buckets[h] = ent;
    map->len++;

    map->tail = map->tail->next_in_order = ent;

    return ent;
}

static struct runtime_map_entry *lookup(const struct runtime_map *map,
        struct runtime_value key)
{
    if (!map)
        return NULL;

    uint64_t h = get_hash(map, key);
    struct runtime_map_entry *ent;

    for (ent = map->buckets[h]; ent; ent = ent->next_in_chain)
        if (match(ent, key))
            return ent;

    return NULL;
}

struct runtime_map *runtime_map_new(int64_t len)
{
    struct runtime_map *m;

    m = calloc(1, sizeof(*m));
    m->obj.kind = OBJ_MAP;
    m->tail = &m->head;

    int init_cap = 2 * len;
    int idx = 0;

    if (len > 0) {
        do {
            idx = next_prime_index(idx);
        } while (get_prime(idx) < init_cap);

        resize(m, idx);
    }

    return m;
}

void runtime_map_free(struct runtime_map *m)
{
    if (!m)
        return;

    struct runtime_map_entry *ent, *next;
    ent = runtime_map_entry_begin(m);

    while (ent) {
        next = runtime_map_entry_next(ent);
        free(ent);
        ent = next;
    }

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

void print_map(const struct runtime_map *map);
void runtime_map_set(struct runtime_map *m, struct runtime_value key, struct runtime_value val)
{
    insert(m, key, val);
}

int64_t runtime_map_len(const struct runtime_map *m)
{
    return m->len;
}

struct runtime_map_entry *runtime_map_entry_begin(const struct runtime_map *m)
{
    return m->head.next_in_order;
}

struct runtime_map_entry *runtime_map_entry_next(const struct runtime_map_entry *ent)
{
    return ent->next_in_order;
}

void print_map(const struct runtime_map *map)
{
    for (int i = 0; i < map->cap; i++) {
        struct runtime_map_entry *ent;
        for (ent = map->buckets[i]; ent; ent = ent->next_in_chain)
            printf( "%4d/%d: key => %s, val => %" PRIival "\n", i, map->cap,
                    runtime_string_get_cstr(ent->key.string),
                    ent->val.inum);
    }
}
