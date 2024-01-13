#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>

#include "compiler.h"


#if 0
#define HASH_SIZE 1237 /* a prime number */
#else
#define HASH_SIZE 5381 /* a prime number */
#endif
#define MULTIPLIER 31

typedef struct Ent {
    char *str;
    struct Ent *next;
} Ent;

static Ent *entries[HASH_SIZE];

static uint32_t hash_fn(const char *key)
{
    uint32_t h = 0;
    unsigned char *p = NULL;

    for (p = (unsigned char *) key; *p != '\0'; p++)
        /* h = MULTIPLIER * h + *p; */
        /* h * 31 + *p */
        h = (h << 5) - h + *p;

    return h % HASH_SIZE;
}

static Ent *new_entry(const char *src)
{
    Ent *e = CALLOC(Ent);
    size_t alloc = strlen(src) + 1;
    char *dst = NALLOC(alloc, char);

    strncpy(dst, src, alloc);
    e->str = dst;
    e->next = NULL;

    return e;
}

static void free_entry(Ent *e)
{
    if (!e)
        return;

    free(e->str);
    free(e);
}

void free_table(void)
{
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        Ent *e = entries[i], *tmp;

        if (!e)
            continue;

        while (e) {
            tmp = e->next;
            free_entry(e);
            e = tmp;
        }
    }
}

void init_table(void)
{
    int i;

    for (i = 0; i < HASH_SIZE; i++)
        entries[i] = NULL;
}

const char *intern(const char *str)
{
    Ent *e;
    uint32_t h = hash_fn(str);

    for (e = entries[h]; e; e = e->next)
        if (!strcmp(str, e->str))
            return e->str;

    e = new_entry(str);
    e->next = entries[h];
    entries[h] = e;

    return e->str;
}

void print_table(void)
{
    int buckets = 0;
    int i;

    for (i = 0; i < HASH_SIZE; i++) {
        int keys = 0;
        Ent *e = entries[i];

        if (!e)
            continue;

        printf("%d: ", i);

        for (; e; e = e->next) {
            printf("\'%s\' ", e->str);
            keys++;
        }

        printf("=> %d keys\n", keys);
        buckets++;
    }

    printf( "buckets %d/%d: %g%% used\n",
            buckets, HASH_SIZE, ((float) buckets) / HASH_SIZE);
}
