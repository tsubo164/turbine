#include "code_stackmap.h"
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_CAP 128

static void push_entry(struct code_stackmap_entry_vec *v, struct code_stackmap_entry *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

void code_stackmap_mark(struct code_stackmap *stackmap, value_addr_t addr, int slot, bool is_ref)
{
    struct code_stackmap_entry *ent = &stackmap->current;

    ent->addr = addr;
    ent->slots[slot] = is_ref ? '*': '-';
}

void code_stackmap_push(struct code_stackmap *stackmap)
{
    /* TODO use global var for args to mark stack.data[0] */
    /*
    if (stackmap->records.len == 0) {
        code_stackmap_mark(stackmap, 0, 0, true);
    }
    */

    struct code_stackmap_entry *newent;
    newent = calloc(1, sizeof(*newent));
    *newent = stackmap->current;

    push_entry(&stackmap->records, newent);
}

const struct code_stackmap_entry *code_stackmap_find_entry(const struct code_stackmap *stackmap, value_addr_t addr)
{
    const struct code_stackmap_entry *ent = NULL;

    /* TODO better search */
    for (value_addr_t i = 0; i < stackmap->records.len; i++) {
        ent = stackmap->records.data[i];

        if (ent->addr == addr) {
            break;
        }

        if (ent->addr > addr) {
            ent = stackmap->records.data[i - 1];
            break;
        }
    }
    assert(ent);

    return ent;
}

bool code_stackmap_is_ref(const struct code_stackmap_entry *ent, int slot)
{
    char c = ent->slots[slot];
    return c == '*';
}

void code_stackmap_reset_current(struct code_stackmap *stackmap)
{
    for (int i = 0; i < 64; i++) {
        stackmap->current.slots[i] = 0;
    }
}

static void print_entry(const struct code_stackmap_entry *ent)
{
    printf("[%6" PRIaddr "] ", ent->addr);

    for (int i = 0; i < 64; i++) {
        char c = ent->slots[i];
        printf("%c", c == 0 ? '.' : c);
    }

    printf("\n");
}

void code_print_stackmap(const struct code_stackmap *stackmap)
{
    for (value_addr_t i = 0; i < stackmap->records.len; i++) {
        const struct code_stackmap_entry *ent = stackmap->records.data[i];
        print_entry(ent);
    }
}

void code_stackmap_free(struct code_stackmap *stackmap)
{
    for (value_addr_t i = 0; i < stackmap->records.len; i++) {
        struct code_stackmap_entry *ent = stackmap->records.data[i];
        free(ent);
    }
    free(stackmap->records.data);
}

void code_globalmap_mark(struct code_globalmap *globalmap, int slot, bool is_ref)
{
    assert(slot >= 0 && slot < sizeof(globalmap->slots) / sizeof(globalmap->slots[0]));
    globalmap->slots[slot] = is_ref ? '*': '-';
}

bool code_globalmap_is_ref(const struct code_globalmap *globalmap, int slot)
{
    assert(slot >= 0 && slot < sizeof(globalmap->slots) / sizeof(globalmap->slots[0]));
    char c = globalmap->slots[slot];
    return c == '*';
}

void code_globalmap_print(const struct code_globalmap *globalmap)
{
    for (int i = 0; i < 64; i++) {
        char c = globalmap->slots[i];
        printf("%c", c == 0 ? '.' : c);
    }
    printf("\n");
}
