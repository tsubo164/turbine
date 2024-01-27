#include "prog.h"
#include "ast.h"
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
