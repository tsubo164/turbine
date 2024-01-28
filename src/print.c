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

    for (int i = 0; i < prog->funcv.len; i++) {
        FuncDef *f = prog->funcv.data[i];
        print_funcdef(f, depth + 1);
    }
}

static void print_header(int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("%d. ", depth);
}

static void print_scope(const Scope *sc, int depth)
{
    // symbols
    for (int i = 0; i < sc->symbols.cap; i++) {
        const struct MapEntry *e = &sc->symbols.buckets[i];
        if (!e->key)
            continue;
        const struct Symbol *sym = e->val;

        if (sym->kind == SYM_VAR) {
            const struct Var *v = sym->var;

            if (IsFunc(v->type)) {
                print_header(depth);
                printf("[func] %s @%d %s\n",
                        v->name, v->id, TypeString(v->type));
                print_scope(v->type->func->scope, depth + 1);
            }
            else {
                print_header(depth);
                printf("[var] %s @%d %s\n",
                        v->name, v->id, TypeString(v->type));
            }
        }

        if (sym->kind == SYM_STRUCT) {
            const struct Struct *strct = sym->strct;
            print_header(depth);
            printf("[struct] %s\n", strct->name);

            for (int j = 0; j < strct->fields.len; j++) {
                const struct Field *f = strct->fields.data[j];
                print_header(depth + 1);
                printf("[field] %s @%d %s\n",
                        f->name, f->id, TypeString(f->type));
            }
        }

        if (sym->kind == SYM_TABLE) {
            const struct Table *t = sym->table;

            print_header(depth);
            printf("[table] %s\n", t->name);
            for (int i = 0; i < t->rows.cap; i++) {
                MapEntry *e = &t->rows.buckets[i];
                if (!e->key)
                    continue;
                Row *r = e->val;
                print_header(depth + 1);
                printf("[row] %s => %lld\n", r->name, r->ival);
            }
        }

        if (sym->kind == SYM_MODULE) {
            const struct Module *m = sym->module;

            print_header(depth);
            printf("[module] %s\n", m->name);
            print_scope(m->scope, depth + 1);
        }
    }

    if ( sc->symbols.used == 0) {
        // no symbol
        print_header(depth);
        printf("--\n");
    }

    // children
    for (Scope *scope = sc->children_; scope; scope = scope->next) {
        print_scope(scope, depth + 1);
    }
}

void PrintScope(const Scope *sc)
{
    print_scope(sc, 0);
}
