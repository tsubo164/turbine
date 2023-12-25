#include "builtin.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        Func *func = builtin->DefineFunc("print");

        func->DeclareParam("...", new Type(TY::ANY));
        func->type = new Type(TY::NIL);
    }
    {
        Func *func = builtin->DefineFunc("exit");

        func->DeclareParam("code", new Type(TY::INT));
        func->type = new Type(TY::NIL);
    }
}
