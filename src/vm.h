#ifndef VM_H
#define VM_H

#include "vm_callstack.h"
#include "code_bytecode.h"
#include "runtime_gc.h"
#include "runtime_array.h"
#include "runtime_struct.h"

struct vm_cpu {
    struct runtime_valuevec stack_;
    const struct code_bytecode *code_;

    /* XXX TEST */
    Int eoc; /* end of code */
    Int eoc_; /* end of code */
    Int ip_; /* instruction pointer */
    Int sp_; /* stack pointer */
    Int bp_; /* base pointer */

    struct vm_callstack callstack;

    bool print_stack_;
    struct runtime_gc gc_;
};

void bm_execute_bytecode(struct vm_cpu *vm, const struct code_bytecode *code);

int64_t vm_get_stack_top(const struct vm_cpu *vm);
void vm_print_stack(const struct vm_cpu *vm);
void vm_enable_print_stack(struct vm_cpu *vm, bool enable);
void vm_print_gc_objects(const struct vm_cpu *vm);

#endif /* _H */
