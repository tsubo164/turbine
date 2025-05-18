#ifndef BUILTIN_MODULE_H
#define BUILTIN_MODULE_H

struct parser_scope;
struct runtime_value;

typedef int (*define_module_function_t)(struct parser_scope *scope);
typedef int (*init_module_function_t)(struct runtime_value *values,
        struct parser_scope *scope);

struct builtin_module {
    const char *name;
    define_module_function_t define_module;
    /* TODO remove this */
    init_module_function_t init_module;
};

struct builtin_module_list {
    struct builtin_module *data;
    int cap;
    int len;
};

/*
 * Register builtin modules to be seen from parser.
 * They will not be loaded until imported
 */
void builtin_register_modules(struct builtin_module_list *modules);
void builtin_free_modules(struct builtin_module_list *modules);

const struct builtin_module *builtin_find_module(
        const struct builtin_module_list *modules,
        const char *key_name);

/* TODO consider renaming to builtin_import_module_symbols */
void builtin_import_module(struct parser_scope *scope,
        const struct builtin_module *mod);

#endif /* _H */
