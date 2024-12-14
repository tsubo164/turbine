#ifndef VM_CPU_H
#define VM_CPU_H

#include "vm_callstack.h"
#include "code_bytecode.h"
#include "runtime_value.h"
#include "runtime_gc.h"

struct vm_args {
    const char **values;
    int count;
};

struct vm_cpu {
    struct runtime_valuevec stack;
    const struct code_bytecode *code;

    int64_t eoc; /* end of code */
    int64_t ip; /* instruction pointer */
    int64_t sp; /* stack pointer */
    int64_t bp; /* base pointer */

    struct vm_callstack callstack;

    bool print_stack;
    struct runtime_gc gc;
};

void vm_execute_bytecode(struct vm_cpu *vm, const struct code_bytecode *bytecode,
        const struct vm_args *args);

int64_t vm_get_stack_top(const struct vm_cpu *vm);
void vm_print_stack(const struct vm_cpu *vm);
void vm_enable_print_stack(struct vm_cpu *vm, bool enable);
void vm_print_gc_objects(const struct vm_cpu *vm);

#endif /* _H */
