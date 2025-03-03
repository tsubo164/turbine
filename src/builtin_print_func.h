#ifndef BUILTIN_PRINT_H
#define BUILTIN_PRINT_H

struct runtime_value;

void builtin_print_func(const struct runtime_value *args, const char *typelist);

#endif /* _H */
