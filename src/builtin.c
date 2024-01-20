#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        const char *name = intern("print");
        Func *func = DeclareFunc(builtin);

        DeclareParam(func, "...", NewAnyType());
        func->return_type = NewNilType();

        DefineVar(builtin, name, NewFuncType(func));
    }
    {
        const char *name = intern("exit");
        Func *func = DeclareFunc(builtin);

        DeclareParam(func, "code", NewIntType());
        func->return_type = NewNilType();

        DefineVar(builtin, name, NewFuncType(func));
    }
}
