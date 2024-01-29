#include "prog.h"
#include "scope.h"
#include "mem.h"
#include <string.h>

// Func
struct Func *AddFunc(struct Prog *prog, const char *name, struct Scope *parent)
{
    struct Func *f = CALLOC(struct Func);
    int offset = 0;

    f->name = name;
    f->scope = NewScope(parent, offset);
    f->is_builtin = false;
    f->ellipsis_index = -1;

    f->id = prog->funcs.len;
    VecPush(&prog->funcs, f);
    return f;
}

struct Func *AddBuiltinFunc(struct Prog *prog, const char *name, struct Scope *parent)
{
    struct Func *f = CALLOC(struct Func);
    int offset = 0;

    f->name = name;
    f->scope = NewScope(parent, offset);
    f->is_builtin = true;
    f->ellipsis_index = -1;

    f->id = prog->builtinfuncs.len;
    VecPush(&prog->builtinfuncs, f);
    return f;
}

void DeclareParam(struct Func *f, const char *name, const Type *type)
{
    struct Symbol *sym = DefineVar(f->scope, name, type, false);
    VecPush(&f->params, sym->var);

    if (!strcmp(name, "..."))
        f->ellipsis_index = ParamCount(f) - 1;

    if (name[0] == '$')
        f->has_special_var = true;
}

const struct Var *GetParam(const struct Func *f, int index)
{
    int idx = 0;

    if (IsVariadic(f) && index >= ParamCount(f))
        idx = ParamCount(f) - 1;
    else
        idx = index;

    if (idx < 0 || idx >= ParamCount(f))
        return NULL;

    return f->params.data[idx];
}

int RequiredParamCount(const struct Func *f)
{
    if (IsVariadic(f))
        return ParamCount(f) - 1;
    else
        return ParamCount(f);
}

int ParamCount(const struct Func *f)
{
    return f->params.len;
}

bool IsVariadic(const struct Func *f)
{
    return f->ellipsis_index >= 0;
}
