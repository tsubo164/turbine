#include "print.h"
#include "prog.h"
#include "ast.h"
#include <stdio.h>

static void print_expr(const Expr *e, int depth)
{
    const KindInfo *info;
    int i;

    if (!e || e->kind == T_NUL)
        return;

    // indentation
    for (i = 0; i < depth; i++) {
        printf("  ");
    }

    // basic info
    info = LookupKindInfo(e->kind);
    printf("%d. <%s>", depth, info->str);
    printf(" (%s)", TypeString(e->type));

    // extra value
    switch (info->type) {
    case 'i':
        printf(" %ld", e->ival);
        break;
    case 'f':
        printf(" %g", e->fval);
        break;
    case 's':
        printf(" %s", e->sval);
        break;
    case 'y':
        printf(" \"%s\"", e->sym->name);
        break;
    }
    printf("\n");

    // children
    if (e->l)
        print_expr(e->l, depth + 1);
    if (e->r)
        print_expr(e->r, depth + 1);
    if (e->list)
        print_expr(e->list, depth + 1);
    if (e->next)
        print_expr(e->next, depth);
}

static void print_stmt(const Stmt *s, int depth)
{
    const KindInfo *info;
    int i;

    if (!s)
        return;

    // indentation
    for (i = 0; i < depth; i++)
        printf("  ");

    // basic info
    info = LookupKindInfo(s->kind);
    printf("%d. <%s>", depth, info->str);
    printf("\n");

    // children
    for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
        print_stmt(stmt, depth + 1);

    print_expr(s->expr, depth + 1);
    print_expr(s->cond, depth + 1);
    print_expr(s->post, depth + 1);
    print_stmt(s->body, depth + 1);
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
    print_stmt(f->body, depth + 1);
}

void PrintProg(const struct Prog *prog)
{
    if (!prog)
        return;

    int depth = 0;

    // basic info
    printf("%d. <prog>", depth);
    printf("\n");

    // children
    for (const Stmt *gvar = prog->gvars; gvar; gvar = gvar->next)
        print_stmt(gvar, depth + 1);

    for (const FuncDef *func = prog->funcs; func; func = func->next)
        print_funcdef(func, depth + 1);
}
