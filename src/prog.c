#include "prog.h"
#include "ast.h"
#include "mem.h"
#include <stdlib.h>
#include <stdio.h>

//--------------------------------
// FuncDef
FuncDef *NewFuncDef(struct Symbol *sym, Stmt *body)
{
    struct FuncDef *f = calloc(1, sizeof(struct FuncDef));
    f->sym = sym;
    f->var = sym->var;
    f->body = body;
    f->func = sym->type->func;
    f->funclit_id = 0;
    f->next = NULL;
    return f;
}

struct Func *AddFunc(struct Prog *prog, const char *name, struct Scope *parent)
{
    struct Func *f = CALLOC(struct Func);
    int offset = 0;

    f->name = name;
    f->scope = NewScope(parent, offset);
    f->is_builtin = false;
    f->ellipsis_index = -1;

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

    VecPush(&prog->builtinfuncs, f);
    return f;
}
