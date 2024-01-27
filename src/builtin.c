#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(Scope *builtin)
{
    {
        const char *name = intern("print");
        Func *func = DeclareFunc(builtin, true);

        DeclareParam(func, "...", NewTypeAny());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
    {
        const char *name = intern("exit");
        Func *func = DeclareFunc(builtin, true);

        DeclareParam(func, "code", NewTypeInt());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
}
