#include "builtin.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        Func *func = builtin->DefineFunc("print");

        func->DeclareParam("...", new Type(TY::Nil));
        func->type = new Type(TY::Nil);
    }
    {
        Func *func = builtin->DefineFunc("exit");

        func->DeclareParam("code", new Type(TY::Integer));
        func->type = new Type(TY::Nil);
    }
}
