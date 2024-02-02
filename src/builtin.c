#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(struct Scope *builtin)
{
    int func_id = 0;
    {
        const char *name = StrIntern("print");
        struct Func *func = AddBuiltinFunc(builtin, name);

        DeclareParam(func, "...", NewTypeAny());
        func->return_type = NewTypeNil();
        func->id = func_id++;

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
    {
        const char *name = StrIntern("exit");
        struct Func *func = AddBuiltinFunc(builtin, name);

        DeclareParam(func, "code", NewTypeInt());
        func->return_type = NewTypeNil();
        func->id = func_id++;

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
}
