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
    /* TODO move outside of vm_cpu so multiple vm_cpus can share */
    struct runtime_valuevec globals__;
    struct runtime_valuevec *globals;

    value_addr_t eoc; /* end of code */
    value_addr_t ip; /* instruction pointer */
    value_addr_t sp; /* stack pointer */
    value_addr_t bp; /* base pointer */

    struct vm_callstack callstack;

    bool print_stack;
    struct runtime_gc gc;
};

void vm_execute_bytecode(struct vm_cpu *vm, const struct code_bytecode *bytecode,
        const struct vm_args *args);

/* stack */
value_int_t vm_get_stack_top(const struct vm_cpu *vm);
void vm_print_stack(const struct vm_cpu *vm);
void vm_enable_print_stack(struct vm_cpu *vm, bool enable);
void vm_print_gc_objects(const struct vm_cpu *vm);

int vm_get_callstack_count(const struct vm_cpu *vm);
const struct vm_call *vm_get_call(const struct vm_cpu *vm, int index);
struct runtime_value vm_lookup_stack(const struct vm_cpu *vm, value_addr_t bp, int offset);

/* globals */
int vm_get_global_count(const struct vm_cpu *vm);
struct runtime_value vm_get_global(const struct vm_cpu *vm, int id);

/* enums */
struct runtime_value vm_get_enum_field(const struct vm_cpu *vm, int index);

void vm_cpu_clear(struct vm_cpu *vm);

#endif /* _H */
