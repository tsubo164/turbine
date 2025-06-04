#include "code_stackmap.h"
#include <stdio.h>

static void print_gcstat(const struct code_stackmap *stackmap)
{
    /*
    const struct code_stackmap_entry *ent = &stackmap->current;

    printf("[%6" PRIaddr "] ", ent->addr);

    for (int i = 0; i < 64; i++) {
        char c = ent->slots[i];
        printf("%c", c == 0 ? '.' : c);
    }
    printf("\n");
    */
}

void code_stackmap_mark(struct code_stackmap *stackmap, value_addr_t addr, int slot, bool is_ref)
{
    struct code_stackmap_entry *ent = &stackmap->current;

    ent->addr = addr;
    ent->slots[slot] = is_ref ? '*': '-';

    print_gcstat(stackmap);
}
