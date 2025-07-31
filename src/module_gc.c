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

    runtime_gc_request_collect(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_get_object_id(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    value_int_t id = runtime_gc_get_object_id(val.obj);
    if (id == 0) {
        /* null */
        id = -1;
    }
    regs->locals[0].inum = id;

    return RESULT_SUCCESS;
}

static int gc_is_object_alive(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    value_int_t id = val.inum;
    bool result = runtime_gc_is_object_alive(gc, id);
    regs->locals[0].inum = result;

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
    {
        const char *name = "get_object_id";
        native_func_t fp = gc_get_object_id;
        struct native_func_param params[] = {
            /* TODO check if any time is the best */
            { "obj",  parser_new_any_type() },
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "is_object_alive";
        native_func_t fp = gc_is_object_alive;
        struct native_func_param params[] = {
            { "id",   parser_new_int_type() },
            { "_ret", parser_new_bool_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
