#include "builtin.h"
#include "builtin_format_func.h"
#include "builtin_print_func.h"
#include "native_function.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_value.h"

#include <assert.h>
#include <stdio.h>

static int builtin_print(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;

    assert(regs->local_count == argc + 2);

    /* locals[0] holds arg count */
    /* locals[1] holds type sequence */
    struct runtime_value *arg = &regs->locals[1];
    const char *types = runtime_string_get_cstr(arg->string);

    builtin_print_func(arg + 1, types);

    return RESULT_SUCCESS;
}

static int builtin_input(struct runtime_gc *gc, struct runtime_registers *regs)
{
#define MAX_STR_LEN 1023
    struct runtime_value val = regs->locals[0];
    struct runtime_value ret;

    char buf[MAX_STR_LEN + 1] = {'\0'};
    int ch = 0;
    int i = 0;

    printf("%s", runtime_string_get_cstr(val.string));

    for (i = 0; i < MAX_STR_LEN; i++) {
        ch = getchar();

        if (ch == '\n')
            break;

        buf[i] = ch;
    }

    ret.string = runtime_gc_string_new(gc, buf);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
#undef MAX_STR_LEN
}

static int builtin_exit(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    regs->locals[0] = val;

    return RESULT_NORETURN;
}

static int builtin_format(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;

    assert(regs->local_count == argc + 2);

    /* locals[0] holds arg count */
    /* locals[1] holds type sequence */
    struct runtime_value *arg = &regs->locals[2];
    struct runtime_value ret = {0};
    const char *fmt = runtime_string_get_cstr(arg->string);
    struct data_strbuf sb = DATA_STRBUF_INIT;

    builtin_format_func(arg, fmt, &sb);

    ret.string = runtime_gc_string_new(gc, sb.data);
    regs->locals[0] = ret;

    data_strbuf_free(&sb);
    return RESULT_SUCCESS;
}

/* vec */
static int builtin_veclen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret.inum = runtime_vec_len(obj.vec);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_vecpush(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value val = regs->locals[1];

    runtime_vec_push(obj.vec, val);

    return RESULT_SUCCESS;
}

static int builtin_vecclear(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];

    runtime_vec_clear(obj.vec);

    return RESULT_SUCCESS;
}

/* map */
static int builtin_maplen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    val.inum = runtime_map_len(val.map);
    regs->locals[0] = val;

    return RESULT_SUCCESS;
}

static int builtin_strlen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    val.inum = runtime_string_len(val.string);
    regs->locals[0] = val;

    return RESULT_SUCCESS;
}

static int builtin_setlen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    val.inum = runtime_set_len(val.set);
    regs->locals[0] = val;

    return RESULT_SUCCESS;
}

static int builtin_setadd(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value val = regs->locals[1];
    struct runtime_value ret;

    ret.inum = runtime_set_add(obj.set, val);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_setcontains(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value key = regs->locals[1];
    struct runtime_value ret;

    ret.inum = runtime_set_contains(obj.set, key);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_setremove(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value key = regs->locals[1];
    struct runtime_value ret;

    ret.inum = runtime_set_remove(obj.set, key);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_stacklen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret.inum = runtime_stack_len(obj.stack);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_stackempty(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret.inum = runtime_stack_empty(obj.stack);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_stackpush(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value val = regs->locals[1];

    runtime_stack_push(obj.stack, val);

    return RESULT_SUCCESS;
}

static int builtin_stackpop(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret = runtime_stack_pop(obj.stack);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_stacktop(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret = runtime_stack_top(obj.stack);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_queuelen(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret.inum = runtime_queue_len(obj.queue);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_queueempty(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret.inum = runtime_queue_empty(obj.queue);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_queuepush(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value val = regs->locals[1];

    runtime_queue_push(obj.queue, val);

    return RESULT_SUCCESS;
}

static int builtin_queuepop(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret = runtime_queue_pop(obj.queue);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

static int builtin_queuefront(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value obj = regs->locals[0];
    struct runtime_value ret;

    ret = runtime_queue_front(obj.queue);
    regs->locals[0] = ret;

    return RESULT_SUCCESS;
}

void define_builtin_functions(struct parser_scope *builtin)
{
    /* I/O */
    {
        const char *name = "print";
        struct parser_type *ret_type = parser_new_nil_type();
        struct native_func_param params[] = {
            { "...", parser_new_any_type() },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_print);
    }
    {
        const char *name = "input";
        struct parser_type *ret_type = parser_new_string_type();
        struct native_func_param params[] = {
            { "msg", parser_new_string_type() },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_input);
    }
    {
        const char *name = "exit";
        struct parser_type *ret_type = parser_new_int_type();
        struct native_func_param params[] = {
            { "code", parser_new_int_type() },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_exit);
    }
    {
        const char *name = "format";
        struct native_func_param params[] = {
            { "fmt", parser_new_string_type(), true },
            { "...", parser_new_any_type() },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_string_type();

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_format);
    }
    /* string */
    {
        /* strlen(s string) int */
        const char *name = "strlen";
        struct native_func_param params[] = {
            { "str", parser_new_string_type() },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_int_type();

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_strlen);
    }
    /* vec */
    {
        /* veclen(v vec{T}) int */
        const char *name = "veclen";
        struct parser_type *ret_type = parser_new_int_type();
        struct native_func_param params[] = {
            { "vec", parser_new_vec_type(parser_new_any_type()) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_veclen);
    }
    {
        /* vecpush(v vec{T}, val T) */
        const char *name = "vecpush";
        struct parser_type *ret_type = parser_new_nil_type();
        struct native_func_param params[] = {
            { "vec",   parser_new_vec_type(parser_new_template_type(0)) },
            { "val",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_vecpush);
    }
    {
        /* vecclear(v vec{T}) */
        const char *name = "vecclear";
        struct parser_type *ret_type = parser_new_nil_type();
        struct native_func_param params[] = {
            { "vec",   parser_new_vec_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_vecclear);
    }
    /* map */
    {
        const char *name = "maplen";
        struct native_func_param params[] = {
            { "map", parser_new_map_type(parser_new_any_type()) },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_int_type();

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_maplen);
    }
    {
        const char *name = "setlen";
        struct parser_type *ret_type = parser_new_int_type();
        struct native_func_param params[] = {
            { "set",   parser_new_set_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_setlen);
    }
    {
        const char *name = "setadd";
        struct parser_type *ret_type = parser_new_bool_type();
        struct native_func_param params[] = {
            { "set",   parser_new_set_type(parser_new_template_type(0)) },
            { "val",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_setadd);
    }
    {
        const char *name = "setcontains";
        struct parser_type *ret_type = parser_new_bool_type();
        struct native_func_param params[] = {
            { "set",   parser_new_set_type(parser_new_template_type(0)) },
            { "key",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_setcontains);
    }
    {
        const char *name = "setremove";
        struct parser_type *ret_type = parser_new_bool_type();
        struct native_func_param params[] = {
            { "set",   parser_new_set_type(parser_new_template_type(0)) },
            { "key",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_setremove);
    }
    {
        const char *name = "stacklen";
        struct parser_type *ret_type = parser_new_int_type();
        struct native_func_param params[] = {
            { "stack",   parser_new_stack_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_stacklen);
    }
    {
        const char *name = "stackempty";
        struct parser_type *ret_type = parser_new_bool_type();
        struct native_func_param params[] = {
            { "stack",   parser_new_stack_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_stackempty);
    }
    {
        const char *name = "stackpush";
        struct parser_type *ret_type = parser_new_nil_type();
        struct native_func_param params[] = {
            { "stack", parser_new_stack_type(parser_new_template_type(0)) },
            { "val",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_stackpush);
    }
    {
        const char *name = "stackpop";
        struct parser_type *ret_type = parser_new_template_type(0);
        struct native_func_param params[] = {
            { "stack", parser_new_stack_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_stackpop);
    }
    {
        const char *name = "stacktop";
        struct parser_type *ret_type = parser_new_template_type(0);
        struct native_func_param params[] = {
            { "stack", parser_new_stack_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_stacktop);
    }
    {
        const char *name = "queuelen";
        struct parser_type *ret_type = parser_new_int_type();
        struct native_func_param params[] = {
            { "_queue",   parser_new_queue_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_queuelen);
    }
    {
        const char *name = "queueempty";
        struct parser_type *ret_type = parser_new_bool_type();
        struct native_func_param params[] = {
            { "queue",   parser_new_queue_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_queueempty);
    }
    {
        const char *name = "queuepush";
        struct parser_type *ret_type = parser_new_nil_type();
        struct native_func_param params[] = {
            { "queue", parser_new_queue_type(parser_new_template_type(0)) },
            { "val",   parser_new_template_type(0) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_queuepush);
    }
    {
        const char *name = "queuepop";
        struct parser_type *ret_type = parser_new_template_type(0);
        struct native_func_param params[] = {
            { "queue", parser_new_queue_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_queuepop);
    }
    {
        const char *name = "queuefront";
        struct parser_type *ret_type = parser_new_template_type(0);
        struct native_func_param params[] = {
            { "queue", parser_new_queue_type(parser_new_template_type(0)) },
            { NULL },
        };

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_queuefront);
    }
}
