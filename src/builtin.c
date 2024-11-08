#include "builtin.h"
#include "parser_symbol.h"
#include "parser_type.h"
#include "data_intern.h"

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
    }
}
