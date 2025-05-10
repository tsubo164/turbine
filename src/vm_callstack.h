#ifndef VM_CALLSTACK_H
#define VM_CALLSTACK_H

#include "value_types.h"
#include <stdbool.h>

struct vm_call {
    int func_index;
    /* TODO remove argc */
    int argc;
    int return_reg;
    value_addr_t return_ip;
    value_addr_t return_bp;
    value_addr_t return_sp;
};

struct vm_callstack {
    struct vm_call *data;
    int cap;
    int len;
};

void vm_callstack_init(struct vm_callstack *v);
void vm_callstack_free(struct vm_callstack *v);

void vm_callstack_push(struct vm_callstack *v, const struct vm_call *call);
void vm_callstack_pop(struct vm_callstack *v, struct vm_call *call);

bool vm_callstack_is_empty(const struct vm_callstack *v);

#endif /* _H */
