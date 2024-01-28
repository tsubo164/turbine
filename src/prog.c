#include "prog.h"
#include "scope.h"
#include "mem.h"

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
