#include "builtin.h"
#include "compiler.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        const char *name = intern("print");
        Func *func = builtin->DeclareFunc();

        DeclareParam(func, "...", NewAnyType());
        func->return_type = NewNilType();

        builtin->DefineVar(name, NewFuncType(func));
    }
    {
        const char *name = intern("exit");
        Func *func = builtin->DeclareFunc();

        DeclareParam(func, "code", NewIntType());
        func->return_type = NewNilType();

        builtin->DefineVar(name, NewFuncType(func));
    }
}
