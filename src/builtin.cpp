#include "builtin.h"
#include "compiler.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        const char *name = intern("print");
        Func *func = builtin->DeclareFunc();

        func->DeclareParam("...", new Type(TY::ANY));
        func->return_type = new Type(TY::NIL);

        builtin->DefineVar(name, NewFuncType(func));
    }
    {
        const char *name = intern("exit");
        Func *func = builtin->DeclareFunc();

        func->DeclareParam("code", new Type(TY::INT));
        func->return_type = new Type(TY::NIL);

        builtin->DefineVar(name, NewFuncType(func));
    }
}
