#ifndef CODE_STACKMAP_H
#define CODE_STACKMAP_H

#include "value_types.h"

struct code_stackmap_entry {
    value_addr_t addr;
    char slots[64];
};

struct code_stackmap_entry_vec {
    struct code_stackmap_entry **data;
    value_addr_t cap;
    value_addr_t len;
};

struct code_stackmap {
    struct code_stackmap_entry current;
    struct code_stackmap_entry_vec records;
};

void code_stackmap_mark(struct code_stackmap *stackmap, value_addr_t addr, int slot, bool is_ref);
const struct code_stackmap_entry *code_stackmap_find_entry(const struct code_stackmap *stackmap, value_addr_t addr);
bool code_stackmap_is_ref(const struct code_stackmap_entry *ent, int slot);
void code_stackmap_reset_current(struct code_stackmap *stackmap);

void code_stackmap_free(struct code_stackmap *stackmap);
void code_print_stackmap(const struct code_stackmap *stackmap);

/* */
struct code_globalmap {
    char slots[64];
};

void code_globalmap_mark(struct code_globalmap *globalmap, int slots, bool is_ref);
bool code_globalmap_is_ref(const struct code_globalmap *globalmap, int slot);
void code_globalmap_print(const struct code_globalmap *globalmap);

#endif /* _H */
