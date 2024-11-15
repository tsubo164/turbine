#include "builtin.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "data_intern.h"

#include "runtime_function.h"
#include "runtime_string.h"
#include "runtime_value.h"

#include <assert.h>
#include <stdio.h>

static int builtin_print(struct runtime_value *registers, int reg_count)
{
    struct runtime_value arg_count = registers[0];
    int argc = arg_count.inum;
    int arg_reg = 1;

    assert(reg_count == 2 * argc + 1);

    for (int i = 0; i < argc; i ++) {

        struct runtime_value val = registers[arg_reg++];
        struct runtime_value type = registers[arg_reg++];

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
            printf("%s", runtime_string_get_cstr(val.str));
            break;
        }

        // peek next arg
        bool skip_separator = false;
        if (i < argc - 1) {
            struct runtime_value next_type = registers[arg_reg + 1];
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

static int builtin_exit(struct runtime_value *registers, int reg_count)
{
    struct runtime_value val = registers[0];

    registers[0] = val;

    return RESULT_NORETURN;
}

void DefineBuiltinFuncs(struct parser_scope *builtin)
{
    {
        const char *name = data_string_intern("print");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "...", parser_new_any_type());

        func->return_type = parser_new_nil_type();
        func->func_type = parser_make_func_type(func);
        func->native_func_ptr = (void*) builtin_print;
    }
    {
        const char *name = data_string_intern("exit");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "code", parser_new_int_type());

        func->return_type = parser_new_int_type();
        func->func_type = parser_make_func_type(func);
        func->native_func_ptr = (void*) builtin_exit;
    }
}
