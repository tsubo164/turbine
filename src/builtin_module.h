#ifndef BUILTIN_MODULE_H
#define BUILTIN_MODULE_H

struct parser_scope;

struct builtin_module {
    const char *name;
};

/*
 * Register builtin modules to be seen from parser.
 * They will not be loaded until imported
 */
void builtin_register_modules(struct builtin_module *modules);

const struct builtin_module *builtin_find_module(const struct builtin_module *modules,
        const char *module_name);

void builtin_import_module(const struct builtin_module *modules,
        struct parser_scope *scope);

#endif /* _H */
