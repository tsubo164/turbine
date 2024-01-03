#include "builtin.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        Func *func = builtin->DeclareFunc();

        func->DeclareParam("...", new Type(TY::ANY));
        func->type = new Type(TY::NIL);

        builtin->DefineVar("print", NewFuncType(func));
    }
    {
        Func *func = builtin->DeclareFunc();

        func->DeclareParam("code", new Type(TY::INT));
        func->type = new Type(TY::NIL);

        builtin->DefineVar("exit", NewFuncType(func));
    }
}
