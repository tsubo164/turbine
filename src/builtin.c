#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        const char *name = intern("print");
        Func *func = DeclareFunc(builtin);

        DeclareParam(func, "...", NewTypeAny());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func));
    }
    {
        const char *name = intern("exit");
        Func *func = DeclareFunc(builtin);

        DeclareParam(func, "code", NewTypeInt());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func));
    }
}
