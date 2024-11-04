#include "builtin.h"
#include "data_intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(struct Scope *builtin)
{
    int func_id = 0;
    {
        const char *name = data_string_intern("print");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "...", NewAnyType());
        func->return_type = NewNilType();
        func->func_type = MakeFuncType(func);
        func->id = func_id++;
    }
    {
        const char *name = data_string_intern("exit");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "code", NewIntType());
        func->return_type = NewIntType();
        func->func_type = MakeFuncType(func);
        func->id = func_id++;
    }
}
