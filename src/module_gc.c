#include "module_gc.h"
#include "native_module.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_struct.h"
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

static int gc_print_objects(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_print_objects(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_request(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_request_collect(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_collect(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_force_collect(gc);
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

#define SET_FIELD(strct, idx, dst, src) \
do {\
    struct runtime_value val; \
    val.dst  = (src); \
    runtime_struct_set((strct), idx, val); \
} while(0)

static const struct parser_enum *enum_gc_reason = NULL;
static const struct parser_struct *struct_gc_stat = NULL;
static const struct parser_struct *struct_gc_log_entry = NULL;

static int gc_get_stats(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret;
    struct runtime_struct *stat = runtime_struct_new(gc, struct_gc_stat->id, 5);

    SET_FIELD(stat, 0, inum,  gc->total_collections);
    SET_FIELD(stat, 1, inum,  gc->used_bytes);
    SET_FIELD(stat, 2, inum,  gc->threshold_bytes);
    SET_FIELD(stat, 3, inum,  gc->max_threshold_bytes);
    SET_FIELD(stat, 4, fpnum, gc->threshold_multiplier);

    ret.strct = stat;
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_print_stats(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret = {0};

    runtime_gc_print_stats(gc);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int gc_get_log(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value ret;
    struct runtime_vec *vec = runtime_vec_new(gc, VAL_STRUCT, 0);
    int len = runtime_gc_get_log_entry_count(gc);

    for (int i = 0; i < len; i++) {
        const struct runtime_gc_log_entry *ent = runtime_gc_get_log_entry(gc, i);
        struct runtime_struct *strct = runtime_struct_new(gc, struct_gc_log_entry->id, 1);

        SET_FIELD(strct, 0, inum,  ent->triggered_addr);
        SET_FIELD(strct, 1, inum,  ent->trigger_reason);
        SET_FIELD(strct, 2, inum,  ent->used_bytes_before);
        SET_FIELD(strct, 3, inum,  ent->used_bytes_after);
        SET_FIELD(strct, 4, fpnum, ent->duration_msec);
        SET_FIELD(strct, 5, inum,  ent->total_collections);

        struct runtime_value val = {.strct = strct};
        runtime_vec_push(gc, vec, val);
    }

    ret.vec = vec;
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

int module_define_gc(struct parser_scope *scope)
{
    struct parser_module *mod = parser_define_module(scope, "_builtin", "gc");

    /* enum */
    {
        const char *name = "Reason";
        const struct native_enum_field fields[] = {
            { "sym",    parser_new_string_type() },
            { "str",    parser_new_string_type() },
            { NULL },
        };
        const struct native_enum_value values[] = {
            { .sval = "NONE" },      { .sval = "None" },
            { .sval = "USER" },      { .sval = "Triggered by user" },
            { .sval = "THRESHOLD" }, { .sval = "Reached the threshold" },
            { .sval = NULL },
        };
        enum_gc_reason = native_define_enum(mod->scope, name, fields, values);
    }
    /* struct */
    {
        const char *name = "Stat";
        const struct native_struct_field fields[] = {
            { "total_collections",    parser_new_int_type() },
            { "used_bytes",           parser_new_int_type() },
            { "threshold_bytes",      parser_new_int_type() },
            { "max_threshold_bytes",  parser_new_int_type() },
            { "threshold_multiplier", parser_new_float_type() },
            { NULL },
        };
        struct_gc_stat = native_define_struct(mod->scope, name, fields);
    }
    {
        const char *name = "LogEntry";
        const struct native_struct_field fields[] = {
            { "triggered_addr",    parser_new_int_type() },
            { "trigger_reason",    parser_new_enum_type(enum_gc_reason) },
            { "used_bytes_before", parser_new_int_type() },
            { "used_bytes_after",  parser_new_int_type() },
            { "duration_msec",     parser_new_float_type() },
            { "total_collections", parser_new_int_type() },
            { NULL },
        };
        struct_gc_log_entry = native_define_struct(mod->scope, name, fields);
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
        const char *name = "print_objects";
        native_func_t fp = gc_print_objects;
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
        const char *name = "request";
        native_func_t fp = gc_request;
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
            /* TODO check if any type is the best */
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
    {
        const char *name = "get_stats";
        native_func_t fp = gc_get_stats;
        struct native_func_param params[] = {
            { "_ret", parser_new_struct_type(struct_gc_stat) },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "print_stats";
        native_func_t fp = gc_print_stats;
        struct native_func_param params[] = {
            { "_ret", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }
    {
        const char *name = "get_log";
        native_func_t fp = gc_get_log;
        struct native_func_param params[] = {
            { "_ret", parser_new_vec_type(parser_new_struct_type(struct_gc_log_entry)) },
            { NULL },
        };

        native_declare_func(mod->scope, mod->name, name, params, fp);
    }

    return 0;
}
