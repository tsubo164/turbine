#ifndef BUILTIN_PRINT_H
#define BUILTIN_PRINT_H

struct runtime_value;
struct vm_cpu;

void builtin_print_func(const struct vm_cpu *vm, const struct runtime_value *args, const char *typelist);

#endif /* _H */
