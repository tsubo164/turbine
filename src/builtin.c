#include "builtin.h"
#include "data_intern.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "runtime_function.h"
#include "runtime_string.h"
#include "runtime_array.h"
#include "runtime_value.h"

#include <assert.h>
#include <stdio.h>

static int builtin_print(struct runtime_gc *gc, struct runtime_registers *regs)
{
    struct runtime_value arg_count = regs->locals[0];
    int argc = arg_count.inum;
    int arg_reg = 1;

    assert(regs->local_count == 2 * argc + 1);

    for (int i = 0; i < argc; i ++) {

        struct runtime_value val = regs->locals[arg_reg++];
        struct runtime_value type = regs->locals[arg_reg++];

        switch (type.inum) {

        case VAL_NIL:
            continue;

        case VAL_BOOL:
            if (val.inum == 0)
                printf("false");
            else
                printf("true");
            break;

        case VAL_INT:
            printf("%lld", val.inum);
            break;

        case VAL_FLOAT:
            printf("%g", val.fpnum);
            break;

        case VAL_STRING:
            printf("%s", runtime_string_get_cstr(val.string));
            break;
        }

        /* peek next arg */
        bool skip_separator = false;
        if (i < argc - 1) {
            struct runtime_value next_type = regs->locals[arg_reg + 1];
            if (next_type.inum == VAL_NIL)
                skip_separator = true;
        }

        if (skip_separator)
            continue;

        int separator = (i == argc - 1) ? '\n' : ' ';
        printf("%c", separator);
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
    }

    func->return_type = return_type;
    func->func_sig = parser_make_func_sig(func);
    func->native_func_ptr = native_func;
}
