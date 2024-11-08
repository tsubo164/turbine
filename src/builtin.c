#include "builtin.h"
#include "parser_type.h"
#include "data_intern.h"
#include "scope.h"

void DefineBuiltinFuncs(struct Scope *builtin)
{
    int func_id = 0;
    {
        const char *name = data_string_intern("print");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "...", parser_new_any_type());
        func->return_type = parser_new_nil_type();
        func->func_type = MakeFuncType(func);
        func->id = func_id++;
    }
    {
        const char *name = data_string_intern("exit");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "code", parser_new_int_type());
        func->return_type = parser_new_int_type();
        func->func_type = MakeFuncType(func);
        func->id = func_id++;
    }
}
