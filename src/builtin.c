#include "builtin.h"
#include "intern.h"
#include "scope.h"
#include "prog.h"
#include "type.h"

void DefineBuiltinFuncs(struct Prog *prog, struct Scope *builtin)
{
    {
        const char *name = intern("print");
        struct Func *func = AddBuiltinFunc(prog, name, builtin);

        DeclareParam(func, "...", NewTypeAny());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
    {
        const char *name = intern("exit");
        struct Func *func = AddBuiltinFunc(prog, name, builtin);

        DeclareParam(func, "code", NewTypeInt());
        func->return_type = NewTypeNil();

        DefineVar(builtin, name, NewTypeFunc(func), false);
    }
}
