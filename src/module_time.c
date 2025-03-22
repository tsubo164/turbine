#include "module_time.h"
#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_vec.h"
#include "runtime_gc.h"
#include "os.h"

#include <stdio.h>

static int time_init(struct runtime_gc *gc, struct runtime_registers *regs)
{
    return RESULT_SUCCESS;
}

static int time_now(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    ret.fpnum = os_time();
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int time_elapsed(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value start = regs->locals[0];
    struct runtime_value ret = {0};

    ret.fpnum = os_elapsed(start.fpnum);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int time_sleep(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value second = regs->locals[0];

    os_sleep(second.fpnum);

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
            { "_ret", parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "elapsed";
        native_func_t fp = time_elapsed;
        struct native_func_param params[] = {
            { "start", parser_new_float_type() },
            { "_ret",  parser_new_float_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "sleep";
        native_func_t fp = time_sleep;
        struct native_func_param params[] = {
            { "seconds", parser_new_float_type() },
            { "_ret",    parser_new_nil_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
