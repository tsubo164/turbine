#include "data_intern.h"
#include "data_cstr.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <inttypes.h>

#define MAX_LOAD_FACTOR 70
#define INIT_SIZE 256

static char **buckets = NULL;
static int64_t capacity = 0;
static int64_t occupied = 0;

static uint64_t fnv_hash(const char *key)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (const char *p = key; *p; p++) {
        hash ^= *p;
        hash *= 0x100000001b3;
    }
    return hash;
}

static const char *insert(const char *key)
{
    if (!key)
        return NULL;

    uint64_t hash = fnv_hash(key);

    for (int i = 0; i < capacity; i++) {
        int pos = (hash + i) % capacity;
        const char *ent = buckets[pos];

        if (!ent) {
            buckets[pos] = data_strdup(key);
            occupied++;
            return buckets[pos];
        }
        else if (!strcmp(ent, key)) {
            return ent;
        }
    }
    return NULL;
}

static void rehash(void)
{
    char **old_buckets = buckets;
    int old_cap = capacity;

    /* resize buckets */
    capacity = capacity < INIT_SIZE ? INIT_SIZE : 2 * capacity;
    buckets = calloc(capacity, sizeof(buckets[0]));
    occupied = 0;

    /* move keys to new buckets */
    for ( int i = 0; i < old_cap; i++ ) {
        const char *ent = old_buckets[i];
        if (ent)
            insert(ent);
    }

    free(old_buckets);
}

const char *data_string_intern(const char *key)
{
    if (!key)
        return NULL;

    if (100 * occupied >= MAX_LOAD_FACTOR * capacity)
        rehash();

    return insert(key);
}

void data_print_intern_table(void)
{
    for (int i = 0; i < capacity; i++) {
        const char *ent = buckets[i];
        if (ent)
            printf("%4d: \"%s\"\n", i, ent);
    }
    printf("buckets %" PRId64 "/%" PRId64 ": %g%% occupied\n",
            occupied, capacity, ((float) occupied) / capacity);
}

void data_intern_table_init(void)
{
    buckets = NULL;
    capacity = 0;
    occupied = 0;
}

void data_intern_table_free(void)
{
    for (int i = 0; i < capacity; i++) {
        char *ent = buckets[i];
        if (ent)
            free(ent);
    }
    free(buckets);
    data_intern_table_init();
}
