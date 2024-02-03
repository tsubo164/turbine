#include "print.h"
#include "ast.h"
#include <stdio.h>

void PrintToken(const struct Token *token, bool format)
{
    int indent = 0;
    bool bol = true;

    for (const struct Token *tok = token; tok; tok = tok->next) {

        if (tok->kind == T_NUL)
            continue;

        if (!format) {
            printf("(%4d, %3d) %s",
                    tok->pos.y, tok->pos.x,
                    TokenString(tok->kind));

            if (tok->kind == T_IDENT)
                printf(" (%s)", tok->sval);
            if (tok->kind == T_INTLIT)
                printf(" (%ld)", tok->ival);
            if (tok->kind == T_FLTLIT)
                printf(" (%g)", tok->fval);
            if (tok->kind == T_STRLIT)
                printf(" (\"%s\")", tok->sval);

            printf("\n");
        }
        else {
            if (tok->kind == T_BLOCKBEGIN) {
                indent++;
                continue;
            }
            else if (tok->kind == T_BLOCKEND) {
                indent--;
                continue;
            }

            if (bol) {
                bol = false;
                for (int i = 0; i < indent; i++)
                    printf("....");
            }

            if (tok->kind == T_NEWLINE) {
                printf("%s\n", TokenString(tok->kind));
                bol = true;
            }
            else if (tok->kind != T_BLOCKBEGIN && tok->kind != T_BLOCKEND) {
                printf("%s ", TokenString(tok->kind));
            }
        }

        if (tok->kind == T_EOF)
            break;
    }
    printf("\n");
}

static void print_expr(const Expr *e, int depth)
{
    const struct KindInfo *info;
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
    const struct KindInfo *info;
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

static void print_func(const struct Func *func, int depth)
{
    if (!func)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <func> \"%s\"", depth, func->name);
    printf(" %s", TypeString(func->return_type));
    printf("\n");

    // children
    print_stmt(func->body, depth + 1);
}

static void print_module(const struct Module *mod, int depth)
{
    if (!mod)
        return;

    // basic info
    printf("%d. <mod> \"%s\"", depth, mod->name);
    printf("\n");

    // children
    for (const Stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        print_stmt(gvar, depth + 1);

    for (int i = 0; i < mod->funcs.len; i++) {
        struct Func *f = mod->funcs.data[i];
        print_func(f, depth + 1);
    }
    //const struct Func *funcs[128] = {NULL};
    //int nfuncs = 0;
    //for (int i = 0; i < mod->scope->symbols.cap; i++) {
    //    const struct MapEntry *e = &mod->scope->symbols.buckets[i];
    //    if (!e->key)
    //        continue;
    //    const struct Symbol *sym = e->val;

    //    if (sym->kind == SYM_VAR) {
    //        const struct Var *var = sym->var;
    //        if (IsFunc(var->type)) {
    //            funcs[var->type->func->id] = var->type->func;
    //            nfuncs++;
    //        }
    //    }
    //}
    //for (int i = 0; i < nfuncs; i++)
    //    print_func(funcs[i], depth + 1);
}

void PrintProg(const struct Module *mod)
{
    if (!mod)
        return;

    print_module(mod, 0);
}

static void print_header(int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("%d. ", depth);
}

static void print_scope(const struct Scope *sc, int depth)
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
                printf("[func] \"%s\" @%d %s(%d)\n",
                        v->name, v->offset,
                        TypeString(v->type), v->type->func->id);
                print_scope(v->type->func->scope, depth + 1);
            }
            else {
                print_header(depth);
                printf("[var] \"%s\" @%d %s\n",
                        v->name, v->offset, TypeString(v->type));
            }
        }

        if (sym->kind == SYM_STRUCT) {
            const struct Struct *strct = sym->strct;
            print_header(depth);
            printf("[struct] \"%s\"\n", strct->name);

            for (int j = 0; j < strct->fields.len; j++) {
                const struct Field *f = strct->fields.data[j];
                print_header(depth + 1);
                printf("[field] \"%s\" @%d %s\n",
                        f->name, f->offset, TypeString(f->type));
            }
        }

        if (sym->kind == SYM_TABLE) {
            const struct Table *t = sym->table;

            print_header(depth);
            printf("[table] \"%s\"\n", t->name);
            for (int i = 0; i < t->rows.cap; i++) {
                struct MapEntry *e = &t->rows.buckets[i];
                if (!e->key)
                    continue;
                struct Row *r = e->val;
                print_header(depth + 1);
                printf("[row] \"%s\" => %lld\n", r->name, r->ival);
            }
        }

        if (sym->kind == SYM_MODULE) {
            const struct Module *m = sym->module;

            print_header(depth);
            printf("[module] \"%s\"\n", m->name);
            print_scope(m->scope, depth + 1);
        }
    }

    if ( sc->symbols.used == 0) {
        // no symbol
        print_header(depth);
        printf("--\n");
    }

    // children
    for (struct Scope *scope = sc->children_; scope; scope = scope->next) {
        print_scope(scope, depth + 1);
    }
}

void PrintScope(const struct Scope *sc)
{
    print_scope(sc, 0);
}
