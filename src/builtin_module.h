#ifndef BUILTIN_MODULE_H
#define BUILTIN_MODULE_H

struct parser_scope;

typedef int (*define_module_function_t)(struct parser_scope *scope);

struct builtin_module {
    const char *name;
    define_module_function_t define_module;
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

const struct builtin_module *builtin_find_module(
        const struct builtin_module_list *modules,
        const char *module_name);

void builtin_import_module(struct parser_scope *scope,
        const struct builtin_module *mod);

#endif /* _H */
