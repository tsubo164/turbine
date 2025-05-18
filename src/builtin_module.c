#include "builtin_module.h"
#include "module_file.h"
#include "module_math.h"
#include "module_time.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    static const struct builtin_module table[] = {
/*
        { .name = "path",  .define_module = builtin_define_module_path },
        { .name = "regex", .define_module = builtin_define_module_path },
*/
        { .name = "file",  .define_module = module_define_file },
        { .name = "math",  .define_module = module_define_math },
        { .name = "time",  .define_module = module_define_time },
    };

    int N = sizeof(table) / sizeof(table[0]);

    for (int i = 0; i < N; i++)
        push_module(modules, &table[i]);
}

void builtin_free_modules(struct builtin_module_list *modules)
{
    free(modules->data);
}

const struct builtin_module *builtin_find_module(
        const struct builtin_module_list *modules,
        const char *key_name)
{
    for (int i = 0; i < modules->len; i++) {
        const struct builtin_module *module = &modules->data[i];

        if (!strcmp(key_name, module->name))
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
