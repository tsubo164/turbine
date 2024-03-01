#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "type.h"

void DefineBuiltinFuncs(struct Scope *builtin)
{
    int func_id = 0;
    {
        const char *name = StrIntern("print");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "...", NewAnyType());
        func->return_type = NewNilType();
        func->id = func_id++;
    }
    {
        const char *name = StrIntern("exit");
        struct Func *func = DeclareBuiltinFunc(builtin, name);

        DeclareParam(func, "code", NewIntType());
        func->return_type = NewNilType();
        func->id = func_id++;
    }
}
