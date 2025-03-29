#include "parser_print.h"
#include "parser_symbol.h"
#include "parser_token.h"
#include "parser_type.h"
#include "parser_ast.h"

#include <stdio.h>

void parser_print_token(const struct parser_token *token, bool format)
{
    int indent = 0;
    bool begin_of_line = true;
    int index = 0;

    for (const struct parser_token *tok = token; tok; tok = tok->next) {

        if (!format) {
            printf("[%6d] ", index++);
            printf("(%4d, %3d) %s",
                    tok->pos.y, tok->pos.x,
                    parser_get_token_string(tok->kind));

            if (tok->kind == TOK_IDENT)
                printf(" (%s)", tok->sval);
            if (tok->kind == TOK_INTLIT)
                printf(" (%ld)", tok->ival);
            if (tok->kind == TOK_FLOATLIT)
                printf(" (%g)", tok->fval);
            if (tok->kind == TOK_STRINGLIT)
                printf(" (\"%s\")", tok->sval);

            printf("\n");
        }
        else {
            if (tok->kind == TOK_BLOCKBEGIN) {
                indent++;
                continue;
            }
            else if (tok->kind == TOK_BLOCKEND) {
                indent--;
                continue;
            }

            if (begin_of_line) {
                begin_of_line = false;
                for (int i = 0; i < indent; i++)
                    printf("....");
            }

            if (tok->kind == TOK_NEWLINE) {
                printf("%s\n", parser_get_token_string(tok->kind));
                begin_of_line = true;
            }
            else if (tok->kind != TOK_BLOCKBEGIN && tok->kind != TOK_BLOCKEND) {
                printf("%s ", parser_get_token_string(tok->kind));
            }

            if (tok->kind == TOK_ROOT)
                printf("\n");
        }

        if (tok->kind == TOK_EOF)
            break;
    }
    printf("\n");
}

static void print_expr(const struct parser_expr *e, int depth)
{
    if (!e)
        return;

    /* indentation */
    for (int i = 0; i < depth; i++)
        printf("  ");

    /* basic info */
    bool is_constexpr = e->kind_orig > 0;
    int printed_kind = is_constexpr ? e->kind_orig : e->kind;

    printf("%d. <%s>", depth, parser_node_string(printed_kind));
    printf(" (%s)", parser_type_string(e->type));

    if (is_constexpr)
        printf("(constexpr)");

    /* extra value */
    switch (e->kind) {

    case NOD_EXPR_BOOLLIT:
    case NOD_EXPR_INTLIT:
    case NOD_EXPR_ENUMLIT:
        printf(" %lld", e->ival);
        break;

    case NOD_EXPR_FLOATLIT:
        printf(" %g", e->fval);
        break;

    case NOD_EXPR_STRINGLIT:
        printf(" \"%s\"", e->sval);
        break;

    case NOD_EXPR_VAR:
        printf(" \"%s\"", e->var->name);
        break;

    case NOD_EXPR_STRUCTFIELD:
        printf(" \"%s\"", e->struct_field->name);
        break;

    case NOD_EXPR_ENUMFIELD:
        printf(" \"%s\"", e->enum_field->name);
        break;

    case NOD_EXPR_FUNCLIT:
        printf(" %s", e->func->fullname);
        break;
    }
    printf("\n");

    /* children */
    print_expr(e->l, depth + 1);
    print_expr(e->r, depth + 1);
    print_expr(e->next, depth);
}

static void print_stmt(const struct parser_stmt *s, int depth)
{
    if (!s)
        return;

    /* indentation */
    for (int i = 0; i < depth; i++)
        printf("  ");

    /* basic info */
    printf("%d. <%s>", depth, parser_node_string(s->kind));
    printf("\n");

    print_expr(s->expr, depth + 1);
    print_stmt(s->init, depth + 1);
    print_expr(s->cond, depth + 1);
    print_stmt(s->post, depth + 1);
    print_stmt(s->body, depth + 1);

    /* children */
    for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
        print_stmt(stmt, depth + 1);
}

static void print_func(const struct parser_func *func, int depth)
{
    if (!func)
        return;

    /* indentation */
    for (int i = 0; i < depth; i++)
        printf("  ");

    /* basic info */
    printf("%d. <func> \"%s\"", depth, func->name);
    printf(" %s", parser_type_string(func->sig->return_type));
    printf("\n");

    /* children */
    print_stmt(func->body, depth + 1);
}

static void print_module(const struct parser_module *mod, int depth)
{
    if (!mod)
        return;

    /* basic info */
    printf("%d. <mod> \"%s\"", depth, mod->name);
    printf("\n");

    /* children */
    for (const struct parser_stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        print_stmt(gvar, depth + 1);

    for (int i = 0; i < mod->funcs.len; i++) {
        const struct parser_func *f = mod->funcs.data[i];
        print_func(f, depth + 1);
    }
}

void parser_print_prog(const struct parser_module *mod)
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

static void print_scope(const struct parser_scope *sc, int depth)
{
    /* symbols */
    for (int i = 0; i < sc->syms.len; i++) {
        const struct parser_symbol *sym = sc->syms.data[i];

        if (sym->kind == SYM_VAR) {
            const struct parser_var *v = sym->var;
            print_header(depth);
            printf("[var] \"%s\" %s @%d\n",
                    v->name, parser_type_string(v->type),
                    v->offset);
        }

        if (sym->kind == SYM_FUNC) {
            const struct parser_func *func = sym->func;
            print_header(depth);
            printf("[fnc] \"%s\" %s (id:%d)\n",
                    func->name, parser_type_string(func->sig->return_type), func->id);
            print_scope(func->scope, depth + 1);
        }

        if (sym->kind == SYM_STRUCT) {
            const struct parser_struct *strct = sym->strct;
            print_header(depth);
            printf("[struct] \"%s\"\n", strct->name);

            for (int j = 0; j < strct->fields.len; j++) {
                const struct parser_struct_field *f = strct->fields.data[j];
                print_header(depth + 1);
                printf("[field] \"%s\" @%d %s\n",
                        f->name, f->offset, parser_type_string(f->type));
            }
        }

        if (sym->kind == SYM_ENUM) {
            const struct parser_enum *enm = sym->enm;
            int nfields = parser_get_enum_field_count(enm);
            int nmembers = parser_get_enum_member_count(enm);

            print_header(depth);
            printf("[enum] \"%s\"\n", enm->name);

            print_header(depth + 1);
            for (int x = 0; x < nfields; x++) {
                const struct parser_enum_field *f = enm->fields.data[x];
                printf("| %s(%s)", f->name, parser_type_string(f->type));
                printf("%c", x < nfields - 1 ? ' ' : '\n');
            }

            print_header(depth + 1);
            for (int x = 0; x < nfields; x++) {
                printf("| ---");
                printf("%c", x < nfields - 1 ? ' ' : '\n');
            }

            for (int y = 0; y < nmembers; y++) {

                print_header(depth + 1);
                for (int x = 0; x < nfields; x++) {
                    const struct parser_enum_field *f = enm->fields.data[x];
                    const struct parser_enum_value val = enm->values.data[x + y * nfields];
                    if (x == 0)
                        printf("| %s", val.sval);
                    else if (parser_is_string_type(f->type))
                        printf("| \"%s\"", val.sval);
                    else if (parser_is_int_type(f->type))
                        printf("| %lld", val.ival);

                    printf("%c", x < nfields - 1 ? ' ' : '\n');
                }
            }
        }

        if (sym->kind == SYM_MODULE) {
            const struct parser_module *m = sym->module;

            print_header(depth);
            printf("[module] \"%s\"\n", m->name);
            print_scope(m->scope, depth + 1);
        }

        if (sym->kind == SYM_SCOPE) {
            const struct parser_scope *child = sym->scope;
            print_scope(child, depth + 1);
        }
    }

    if (sc->symbols.used == 0) {
        /* no symbol */
        print_header(depth);
        printf("--\n");
    }
}

void parser_print_scope(const struct parser_scope *sc)
{
    print_scope(sc, 0);
}

void parser_print_expr(const struct parser_expr *e)
{
    print_expr(e, 0);
}
