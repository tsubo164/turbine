#include "data_intern.h"
#include "data_cstr.h"

#include <inttypes.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MAX_LOAD_FACTOR 70
#define INIT_SIZE 256

static uint64_t fnv_hash(const char *key)
{
    uint64_t hash = 0xcbf29ce484222325;
    for (const char *p = key; *p; p++) {
        hash ^= *p;
        hash *= 0x100000001b3;
    }
    return hash;
}

static const char *insert(struct data_intern_table *table, const char *key, bool dup_key)
{
    if (!key)
        return NULL;

    uint64_t hash = fnv_hash(key);

    for (int i = 0; i < table->cap; i++) {
        int pos = (hash + i) % table->cap;
        const char *ent = table->buckets[pos];

        if (!ent) {
            if (dup_key)
                table->buckets[pos] = data_strdup(key);
            else
                table->buckets[pos] = (char*) key;
            table->used++;
            return table->buckets[pos];
        }
        else if (!strcmp(ent, key)) {
            return ent;
        }
    }
    return NULL;
}

static void rehash(struct data_intern_table *table)
{
    char **old_buckets = table->buckets;
    int old_cap = table->cap;

    /* resize buckets */
    table->cap = table->cap < INIT_SIZE ? INIT_SIZE : 2 * table->cap;
    table->buckets = calloc(table->cap, sizeof(table->buckets[0]));
    table->used = 0;

    /* move keys to new buckets */
    for ( int i = 0; i < old_cap; i++ ) {
        const char *ent = old_buckets[i];
        if (ent) {
            bool dup_key = false;
            insert(table, ent, dup_key);
        }
    }

    free(old_buckets);
}

const char *data_string_intern(struct data_intern_table *table, const char *key)
{
    if (!key)
        return NULL;

    if (100 * table->used >= MAX_LOAD_FACTOR * table->cap)
        rehash(table);

    bool dup_key = true;
    return insert(table, key, dup_key);
}

void data_print_intern_table(const struct data_intern_table *table)
{
    for (int i = 0; i < table->cap; i++) {
        const char *ent = table->buckets[i];
        if (ent)
            printf("%4d: \"%s\"\n", i, ent);
    }
    printf("table->buckets %" PRId64 "/%" PRId64 ": %g%% table->used\n",
            table->used, table->cap, ((float) table->used) / table->cap);
}

void data_intern_table_init(struct data_intern_table *table)
{
    table->buckets = NULL;
    table->cap = 0;
    table->used = 0;
}

void data_intern_table_clear(struct data_intern_table *table)
{
    for (int i = 0; i < table->cap; i++) {
        char *ent = table->buckets[i];
        if (ent)
            free(ent);
    }
    free(table->buckets);
    data_intern_table_init(table);
}
