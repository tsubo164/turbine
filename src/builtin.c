#include "builtin.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_function.h"
#include "runtime_string.h"
#include "runtime_array.h"
#include "runtime_value.h"
#include "format.h"

#include <assert.h>
#include <string.h>
#include <stdio.h>
#include <ctype.h>
#include <math.h>

static const char *print_value(struct runtime_value val, const char *fmt)
{
    const char *p = fmt;

    switch (*p++) {

    case 'i':
        printf("%lld", val.inum);
        return p;

    case 'f':
        printf("%g", val.fpnum);
        if (fmod(val.fpnum, 1.0) == 0.0)
            printf(".0");
        return p;

    case 's':
        printf("%s", runtime_string_get_cstr(val.string));
        return p;

    case 'a':
        {
            int len = runtime_array_len(val.array);
            const char *elem = p;

            printf("[");
            for (int i = 0; i < len; i++) {
                p = print_value(runtime_array_get(val.array, i), elem);
                if (i == len - 1)
                    printf("]");
                else
                    printf(", ");
            }
        }
        return p;

    case 'n':
        return p;

    default:
        assert(!"variadic argument error");
        return NULL;
    }
}

static int builtin_print(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;

    assert(regs->local_count == argc + 1);

    /* locals[0] holds arg count */
    struct runtime_value *arg = &regs->locals[1];
    const char *fmt = runtime_string_get_cstr(arg->string);
    arg++;

    while (*fmt) {
        int curr;
        int next;

        curr = *fmt;
        fmt  = print_value(*arg++, fmt);
        next = *fmt;

        if (curr == 'n' || next == 'n')
            continue;

        printf(next ? " " : "\n");
    }

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

#if 0
static void insert_group_separators(const char *start, const char *stop, char *output,
        const struct format_spec *spec, bool is_positive)
{
    const char *dot = strchr(start, '.');
    const char *end = dot ? dot : stop;
    char *dst = output;

    if (spec->show_plus && is_positive)
        *dst++ = '+';
    if (spec->positive_space && is_positive)
        *dst++ = ' ';

    for (const char *src = start; src < end; src++) {
        if ((end - src) % 3 == 0 && isdigit(*src))
            *dst++ = spec->group1k;
        *dst++ = *src;
    }
}
#endif

static void format_width(struct data_strbuf *sb, const char *src,
        const struct format_spec *spec, bool is_positive_number)
{
    if (spec->width > 0) {
        int len = strlen(src);
        int width = spec->width;
        int pads = width > len ? width - len : 0;

        if (format_is_spec_align_left(spec)) {
            int i = 0;

            if (spec->plussign && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                i++;
            }

            data_strbuf_cat(sb, src);

            for (; i < pads; i++)
                data_strbuf_push(sb, spec->pad);
        }
        else {
            int i = 0;

            if (spec->plussign && spec->pad == '0' && is_positive_number) {
                data_strbuf_push(sb, spec->plussign);
                i++;
            }

            for (; i < pads; i++) {
                char pad = spec->pad;

                if (i == pads - 1)
                    if (spec->plussign && spec->pad == ' ' && is_positive_number)
                        pad = spec->plussign;

                data_strbuf_push(sb, pad);
            }

            data_strbuf_cat(sb, src);
        }
    }
    else {
        data_strbuf_cat(sb, src);
    }
}

static void format_int(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, int64_t inum)
{
#define BUFSIZE 64
    char buf[BUFSIZE] = {'\0'};
    const char *outputbuf = buf;
    snprintf(buf, BUFSIZE, c_spec, inum);
#undef BUFSIZE

    if (spec->group1k) {
        int len = strlen(buf);
        const char *dot = strchr(buf, '.');
        const char *end = dot ? dot : buf + len;

        char buf1k[64] = {'\0'};
        char *dst = buf1k;
        outputbuf = buf1k;

        for (const char *src = buf; src < end; src++) {
            if ((end - src) % 3 == 0 && isdigit(*src))
                *dst++ = spec->group1k;
            *dst++ = *src;
        }
    }

    format_width(sb, outputbuf, spec, inum > 0);
}

static void format_float(struct data_strbuf *sb, const struct format_spec *spec,
        const char *c_spec, double fpnum)
{
#define BUFSIZE 64
    char buf[BUFSIZE] = {'\0'};

    snprintf(buf, BUFSIZE, c_spec, fpnum);
    if (fmod(fpnum, 1.0) == 0.0) {
        int len = strlen(buf);
        buf[len]   = '.';
        buf[len+1] = '0';
    }

    if (!spec->group1k) {
    }

    format_width(sb, buf, spec, fpnum > 0.0);
}

static int builtin_format(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;

    assert(regs->local_count == argc + 1);

    /* locals[0] holds arg count */
    /* locals[1] holds type info */
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

            if (format_is_spec_int(&spec)) {
                format_int(&sb, &spec, c_spec, arg->inum);
            }
            else if(format_is_spec_float(&spec)) {
                format_float(&sb, &spec, c_spec, arg->fpnum);
            }
            else if(format_is_spec_string(&spec)) {
                data_strbuf_cat(&sb, runtime_string_get_cstr(arg->string));
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

    val.inum = runtime_array_len(val.array);
    regs->locals[0] = val;

    return RESULT_SUCCESS;
}

static int builtin_resize(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value val = regs->locals[0];
    struct runtime_value len = regs->locals[1];

    runtime_array_resize(val.array, len.inum);

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

        func->return_type = parser_new_nil_type();
        func->func_sig = parser_make_func_sig(func);
        func->native_func_ptr = builtin_print;
    }
    {
        const char *name = data_string_intern("input");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "msg", parser_new_string_type());

        func->return_type = parser_new_string_type();
        func->func_sig = parser_make_func_sig(func);
        func->native_func_ptr = builtin_input;
    }
    {
        const char *name = data_string_intern("exit");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "code", parser_new_int_type());

        func->return_type = parser_new_int_type();
        func->func_sig = parser_make_func_sig(func);
        func->native_func_ptr = builtin_exit;
    }
    {
        const char *name = "format";
        struct native_func_param params[] = {
            { "fmt",  parser_new_string_type(), true },
            { "...",  parser_new_any_type() },
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
        struct parser_func *func;

        func = parser_declare_builtin_func(builtin, "len");
        parser_declare_param(func, "array", parser_new_array_type(parser_new_any_type()));

        func->return_type = parser_new_int_type();
        func->func_sig = parser_make_func_sig(func);
        func->native_func_ptr = builtin_len;
    }
    {
        const char *name = "resize";
        struct native_func_param params[] = {
            { "array",    parser_new_array_type(parser_new_template_type(0)) },
            { "new_len",  parser_new_int_type() },
            { NULL },
        };
        struct parser_type *ret_type = parser_new_array_type(parser_new_template_type(0));

        native_declare_func(builtin,
                name,
                params,
                ret_type,
                builtin_resize);
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
            func->has_format_param = true;
    }

    func->return_type = return_type;
    func->func_sig = parser_make_func_sig(func);
    func->native_func_ptr = native_func;
}
