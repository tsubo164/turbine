#include "builtin_module.h"
#include "parser_symbol.h"

#include <string.h>

void builtin_register_modules(struct builtin_module *modules)
{
}

const struct builtin_module *builtin_find_module(const struct builtin_module *modules,
        const char *module_name)
{
    static const struct builtin_module mod = {
        .name = "math",
    };

    if (!strcmp(module_name, mod.name)) {
        return &mod;
    }

    return NULL;
}

void builtin_import_module(const struct builtin_module *modules,
        struct parser_scope *scope)
{
}
