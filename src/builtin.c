#include "builtin.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "data_intern.h"

#include "runtime_function.h"
#include "runtime_value.h"

#include <stdio.h>

static int builtin_exit(struct runtime_value *registers, int reg_count)
{
    struct runtime_value val = registers[0];

    registers[0] = val;

    return RESULT_NORETURN;
}

void DefineBuiltinFuncs(struct parser_scope *builtin)
{
    int func_id = 0;
    {
        const char *name = data_string_intern("print");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "...", parser_new_any_type());
        func->return_type = parser_new_nil_type();
        func->func_type = parser_make_func_type(func);
        func->id = func_id++;
    }
    {
        const char *name = data_string_intern("exit");
        struct parser_func *func = parser_declare_builtin_func(builtin, name);

        parser_declare_param(func, "code", parser_new_int_type());
        func->return_type = parser_new_int_type();
        func->func_type = parser_make_func_type(func);
        func->id = func_id++;

        func->native_func_ptr = (void*) builtin_exit;
    }
}
