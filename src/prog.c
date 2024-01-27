#include "prog.h"
#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

//--------------------------------
// FuncDef
FuncDef *NewFuncDef(struct Symbol *sym, Stmt *body)
{
    //FuncDef *f = CALLOC(FuncDef);
    struct FuncDef *f = calloc(1, sizeof(struct FuncDef));
    f->sym = sym;
    f->var = sym->var;
    f->body = body;
    f->func = sym->type->func;
    f->funclit_id = 0;
    f->next = NULL;
    return f;
}

static void print_funcdef(const FuncDef *f, int depth)
{
    if (!f)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <func_def> \"%s\"", depth, f->var->name);
    printf(" %s", TypeString(f->var->type->func->return_type));
    printf("\n");

    // children
    PrintStmt(f->body, depth + 1);
}

void PrintProg(const struct Prog *prog, int depth)
{
    if (!prog)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <prog>", depth);
    printf("\n");

    // children
    for (const Stmt *gvar = prog->gvars; gvar; gvar = gvar->next)
        PrintStmt(gvar, depth + 1);

    for (const FuncDef *func = prog->funcs; func; func = func->next)
        print_funcdef(func, depth + 1);
}
