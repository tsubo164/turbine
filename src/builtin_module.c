#include "builtin_module.h"
#include "parser_symbol.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

int builtin_define_module_math(struct parser_scope *scope)
{
    parser_define_module(scope, ":builtin", "math");
    return 0;
}

#define MIN_CAP 16

static void push_module(struct builtin_module_list *v, const struct builtin_module *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = *val;
}

void builtin_register_modules(struct builtin_module_list *modules)
{
    static const struct builtin_module modarray[] = {
        { .name = "math", .define_module = builtin_define_module_math },
    };

    int N = sizeof(modarray) / sizeof(modarray[0]);

    for (int i = 0; i < N; i++)
        push_module(modules, &modarray[i]);
}

const struct builtin_module *builtin_find_module(
        const struct builtin_module_list *modules,
        const char *module_name)
{
    for (int i = 0; i < modules->len; i++) {
        const struct builtin_module *module = &modules->data[i];

        if (!strcmp(module_name, module->name))
            return module;
    }

    return NULL;
}

void builtin_import_module(struct parser_scope *scope,
        const struct builtin_module *mod)
{
    assert(mod->define_module);
    mod->define_module(scope);
}
