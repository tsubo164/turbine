#include "module_gc.h"
#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_vec.h"
#include "runtime_gc.h"
#include "os.h"

#include <stdio.h>

static int gc_init(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};
    regs->locals[0] = ret;
    return RESULT_SUCCESS;
}

static int gc_print(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_print_objects(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_collect(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_collect_objects(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

int module_define_gc(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "gc");

    /* struct */
    {
    }
    /* function */
    {
        const char *name = "init";
        native_func_t fp = gc_init;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "print";
        native_func_t fp = gc_print;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "collect";
        native_func_t fp = gc_collect;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
