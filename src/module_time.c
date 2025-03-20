#include "module_time.h"
#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_vec.h"
#include "runtime_gc.h"

#include <stdio.h>
#include <time.h>

static int time_init(struct runtime_gc *gc, struct runtime_registers *regs)
{
    return RESULT_SUCCESS;
}

static int time_now(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    {
        time_t t = time(NULL);
        ret.inum = (int64_t) t;
    }

    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

int module_define_time(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "time");

    /* struct */
    {
    }
    /* function */
    {
        const char *name = "init";
        native_func_t fp = time_init;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "now";
        native_func_t fp = time_now;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
