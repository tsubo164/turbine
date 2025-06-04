#ifndef CODE_STACKMAP_H
#define CODE_STACKMAP_H

#include "value_types.h"

struct code_stackmap_entry {
    value_addr_t addr;
    char slots[64];
};

struct code_stackmap {
    struct code_stackmap_entry current;
};

void code_stackmap_mark(struct code_stackmap *code, value_addr_t addr, int slot, bool is_ref);

#endif /* _H */
