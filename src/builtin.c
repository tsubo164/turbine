#include "builtin.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_function.h"
#include "runtime_vec.h"
#include "runtime_map.h"
#include "runtime_set.h"
#include "runtime_stack.h"
#include "runtime_queue.h"
#include "runtime_string.h"
#include "runtime_struct.h"
#include "runtime_value.h"
#include "format.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

static void print_value(struct runtime_value val, struct parser_typelist_iterator *it)
{
    switch (it->kind) {

    case TYP_NIL:
        printf("nil");
        return;

    case TYP_BOOL:
        if (val.inum)
            printf("true");
        else
            printf("false");
        return;

    case TYP_INT:
        printf("%lld", val.inum);
        return;

    case TYP_FLOAT:
        printf("%g", val.fpnum);
        if (fmod(val.fpnum, 1.0) == 0.0)
            printf(".0");
        return;

    case TYP_STRING:
        printf("%s", runtime_string_get_cstr(val.string));
        return;

    case TYP_VEC:
        {
            int len = runtime_vec_len(val.vec);
            parser_typelist_next(it);

            printf("vec{");
            for (int i = 0; i < len; i++) {
                print_value(runtime_vec_get(val.vec, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");
        }
        return;

    case TYP_MAP:
        {
            parser_typelist_next(it);

            printf("{");
            struct runtime_map_entry *ent;
            for (ent = runtime_map_entry_begin(val.map);
                    ent; ent = runtime_map_entry_next(ent)) {

                printf("%s:", runtime_string_get_cstr(ent->key.string));
                print_value(ent->val, it);

                if (runtime_map_entry_next(ent))
                    printf(", ");
            }
            printf("}");
        }
        return;

    case TYP_SET:
        {
            parser_typelist_next(it);

            printf("set{");
            struct runtime_set_node *node;
            for (node = runtime_set_node_begin(val.set);
                    node; node = runtime_set_node_next(node)) {

                print_value(node->val, it);

                if (runtime_set_node_next(node))
                    printf(", ");
            }
            printf("}");
        }
        return;

    case TYP_STACK:
        {
            int len = runtime_stack_len(val.stack);
            parser_typelist_next(it);

            printf("stack{");
            for (int i = 0; i < len; i++) {
                print_value(runtime_stack_get(val.stack, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");
        }
        return;

    case TYP_QUEUE:
        {
            int len = runtime_queue_len(val.queue);
            parser_typelist_next(it);

            printf("queue{");
            for (int i = 0; i < len; i++) {
                print_value(runtime_queue_get(val.queue, i), it);
                if (i < len - 1)
                    printf(", ");
            }
            printf("}");
        }
        return;

    case TYP_STRUCT:
        {
            int len = runtime_struct_field_count(val.strct);
            parser_typelist_next(it);

            printf("{");
            for (int i = 0; i < len; i++) {
                print_value(runtime_struct_get(val.strct, i), it);
                if (i < len - 1)
                    printf(", ");
                parser_typelist_next(it);
            }
            printf("}");
            assert(parser_typelist_struct_end(it));
        }
        return;

    case TYP_ENUM:
        printf("%lld", val.inum);
        return;

    default:
        assert(!"variadic argument error");
        return;
    }
}

static int builtin_print(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;

    assert(regs->local_count == argc + 2);

    /* locals[0] holds arg count */
    /* locals[1] holds type sequence */
    struct runtime_value *arg = &regs->locals[1];
    const char *types = runtime_string_get_cstr(arg->string);
    arg++;

    struct parser_typelist_iterator it;
    parser_typelist_begin(&it, types);

    while (!parser_typelist_end(&it)) {
        print_value(*arg++, &it);
        parser_typelist_next(&it);

        if (!parser_typelist_end(&it))
            printf(" ");
    }
    printf("\n");

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

static void format_width(struct data_strbuf *sb, const char *src,
        const struct format_spec *spec, bool is_positive_number)
{
    if (spec->width > 0) {
        int len = strlen(src);
        int width = spec->width;
        int pads = width > len ? width - len : 0;

        if (format_is_spec_align_left(spec)) {
            if (spec->plussign && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                pads--;
            }

            data_strbuf_cat(sb, src);
            data_strbuf_pushn(sb, spec->pad, pads);
        }
        else {
            if (spec->plussign && spec->pad == '0' && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                pads--;
            }

            if (spec->plussign && spec->pad == ' ' && is_positive_number) {
                data_strbuf_pushn(sb, spec->pad, pads - 1);
                data_strbuf_push(sb, spec->plussign);
            }
            else {
                data_strbuf_pushn(sb, spec->pad, pads);
            }

            data_strbuf_cat(sb, src);
        }
    }
    else {
        data_strbuf_cat(sb, src);
    }
}

static const char *insert_group_separators(const char *input, char *output, char separator)
{
    const char *dot = strchr(input, '.');
    const char *end = dot ? dot : input + strlen(input);
    const char *src = input;
    char *dst = output;

    while (src < end) {
        if ((end - src) % 3 == 0 && isdigit(*src))
            *dst++ = separator;
        *dst++ = *src++;
    }

    while (*src)
        *dst++ = *src++;

    *dst = '\0';

    return output;
}

#define BUFSIZE 64
static void format_int(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, int64_t inum)
{
    char buf[BUFSIZE] = {'\0'};
    const char *outputbuf = buf;

    snprintf(buf, BUFSIZE, c_spec, inum);

    if (spec->group1k) {
        char buf1k[BUFSIZE] = {'\0'};
        outputbuf = insert_group_separators(buf, buf1k, spec->group1k);
    }

    format_width(sb, outputbuf, spec, inum > 0);
}

static void format_float(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, double fpnum)
{
    char buf[BUFSIZE] = {'\0'};
    const char *outputbuf = buf;

    snprintf(buf, BUFSIZE, c_spec, fpnum);

    if (spec->pointzero && fmod(fpnum, 1.0) == 0.0) {
        int len = strlen(buf);
        buf[len]   = '.';
        buf[len+1] = '0';
    }

    if (spec->group1k) {
        char buf1k[BUFSIZE] = {'\0'};
        outputbuf = insert_group_separators(buf, buf1k, spec->group1k);
    }

    format_width(sb, outputbuf, spec, fpnum > 0.0);
}
#undef BUFSIZE

static void format_string(struct data_strbuf *sb, const struct format_spec *spec,
        const char *src)
{
    int len = strlen(src);

    if (spec->precision > 0) {
        if (len > spec->precision)
            len = spec->precision;
    }

    if (spec->width > 0) {
        int width = spec->width;
        int pads = width > len ? width - len : 0;

        if (format_is_spec_align_left(spec)) {
            data_strbuf_catn(sb, src, len);
            data_strbuf_pushn(sb, ' ', pads);
        }
        else {
            data_strbuf_pushn(sb, ' ', pads);
            data_strbuf_catn(sb, src, len);
        }
    }
    else {
        data_strbuf_catn(sb, src, len);
    }
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

    while (*fmt) {

        if (*fmt == '%') {

            struct format_spec spec = {0};
            char c_spec[32] = {'\0'};
            int c_spec_size = sizeof(c_spec)/sizeof(c_spec[0]);

            fmt = format_parse_specifier(fmt, &spec, c_spec, c_spec_size);
            assert(!spec.errmsg);
            arg++;

            if (format_is_spec_bool(&spec)) {
                format_string(&sb, &spec, arg->inum ? "true" : "false");
            }
            else if (format_is_spec_int(&spec)) {
                format_int(&sb, &spec, c_spec, arg->inum);
            }
            else if(format_is_spec_float(&spec)) {
                format_float(&sb, &spec, c_spec, arg->fpnum);
            }
            else if(format_is_spec_string(&spec)) {
                format_string(&sb, &spec, runtime_string_get_cstr(arg->string));
            }
            else {
                data_strbuf_push(&sb, '%');
                continue;
            }
        }
        else {
            data_strbuf_push(&sb, *fmt++);
        }
    }

    ret.string = runtime_gc_string_new(gc, sb.data);
    regs->locals[0] = ret;

    data_strbuf_free(&sb);
    return RESULT_SUCCESS;
}

static int builtin_len(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];

    val.inum = runtime_vec_len(val.vec);
    regs->locals[0] = val;

    return RESULT_SUCCESS;
}

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

static int builtin_resize(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];
    struct runtime_value len = regs->locals[1];

    runtime_vec_resize(val.vec, len.inum);

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

struct native_func_param {
    const char *name;
    const struct parser_type *type;
    bool is_format;
};

void native_declare_func(struct parser_scope *scope,
        const char *name,
        const struct native_func_param *params,
        const struct parser_type *return_type,
        runtime_native_function_t native_func);

void define_builtin_functions(struct parser_scope *builtin)
{
    {
        const char *name = data_string_intern("print");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "...", parser_new_any_type());

        parser_add_return_type(func, parser_new_nil_type());
        func->native_func_ptr = builtin_print;
    }
    {
        const char *name = data_string_intern("input");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "msg", parser_new_string_type());

        parser_add_return_type(func, parser_new_string_type());
        func->native_func_ptr = builtin_input;
    }
    {
        const char *name = data_string_intern("exit");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "code", parser_new_int_type());

        parser_add_return_type(func, parser_new_int_type());
        func->native_func_ptr = builtin_exit;
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
    {
        const char *name = "len";
        struct native_func_param params[] = {
            { "vec", parser_new_vec_type(parser_new_any_type()) },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_int_type();

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_len);
    }
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
    {
        const char *name = "resize";
        struct native_func_param params[] = {
            { "vec",   parser_new_vec_type(parser_new_template_type(0)) },
            { "new_len", parser_new_int_type() },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_vec_type(parser_new_template_type(0));

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_resize);
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

void native_declare_func(struct parser_scope *scope,
        const char *name,
        const struct native_func_param *params,
        const struct parser_type *return_type,
        runtime_native_function_t native_func)
{
    const struct native_func_param *param;
    struct parser_func *func;

    func = parser_declare_builtin_func(scope, name);

    for (param = params; param->name; param++) {
        parser_declare_param(func, param->name, param->type);

        if (param->is_format)
            func->sig->has_format_param = true;
    }

    parser_add_return_type(func, return_type);
    func->native_func_ptr = native_func;
}
