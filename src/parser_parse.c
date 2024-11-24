#include "parser_parse.h"
#include "parser_search_path.h"
#include "parser_ast_eval.h"
#include "parser_escseq.h"
#include "parser_symbol.h"
#include "parser_error.h"
#include "parser_token.h"
#include "parser_type.h"
#include "parser_ast.h"
#include "os.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

/* TODO move this somewhere */
#include "data_strbuf.h"
static const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    struct data_strbuf sb = DATA_STRBUF_INIT;
    while (fgets(buf, 1024, fp)) {
        data_strbuf_cat(&sb, buf);
    }
    data_strbuf_cat(&sb, "\n");

    fclose(fp);

    return sb.data;
}

struct parser {
    /* current context */
    struct parser_scope *scope;
    struct parser_func *func;
    const struct parser_token *curr;

    /* source */
    const char *src;
    const char *filename;
    const struct parser_search_path *paths;

    struct parser_module *module;
};

static void error(const struct parser *p, struct parser_pos pos, const char *fmt, ...)
{
    va_list args; 
    va_start(args, fmt);
    parser_error_va(p->src, p->filename, pos.x, pos.y, fmt, args);
    va_end(args);
}

static const struct parser_token *curtok(const struct parser *p)
{
    return p->curr;
}

static const struct parser_token *gettok(struct parser *p)
{
    p->curr = p->curr->next;
    return p->curr;
}

static void ungettok(struct parser *p)
{
    p->curr = p->curr->prev;
}

static struct parser_pos tok_pos(const struct parser *p)
{
    return p->curr->pos;
}

static long tok_int(const struct parser *p)
{
    return p->curr->ival;
}

static double tok_float(const struct parser *p)
{
    return p->curr->fval;
}

static const char *tok_str(const struct parser *p)
{
    return p->curr->sval;
}

static int peek(const struct parser *p)
{
    if (p->curr->kind == TOK_EOF)
        return TOK_EOF;
    else
        return p->curr->next->kind;
}

static void expect(struct parser *p, int kind)
{
    const struct parser_token *tok = gettok(p);
    if (tok->kind != kind) {
        error(p, tok->pos, "expected '%s'", parser_get_token_string(kind));
    }
}

static bool consume(struct parser *p, int kind)
{
    if (peek(p) == kind) {
        gettok(p);
        return true;
    }
    else {
        return false;
    }
}

/* forward decls */
static struct parser_type *type_spec(struct parser *p);
static struct parser_expr *expression(struct parser *p);
static struct parser_stmt *block_stmt(struct parser *p, struct parser_scope *block_scope);

static struct parser_expr *arg_list(struct parser *p, struct parser_expr *call)
{
    struct parser_expr head = {0};
    struct parser_expr *tail = &head;
    int count = 0;

    if (peek(p) != TOK_RPAREN) {
        do {
            tail = tail->next = expression(p);
            count++;
        }
        while (consume(p, TOK_COMMA));
    }

    const struct parser_func_type *func_type = call->l->type->func_type;
    if (func_type->has_special_var) {
        tail = tail->next = parser_new_intlit_expr(call->pos.y);
        count++;
    }

    call->r = head.next;

    const int argc = count;
    const int paramc = parser_required_param_count(func_type);
    if (argc < paramc)
        error(p, tok_pos(p), "too few arguments");

    const struct parser_expr *arg = call->r;
    for (int i = 0; i < argc; i++, arg = arg->next) {
        const struct parser_type *param_type = parser_get_param_type(func_type, i);

        if (!param_type)
            error(p, tok_pos(p), "too many arguments");

        /* TODO arg needs to know its pos */
        if (!parser_match_type(arg->type, param_type)) {
            error(p, tok_pos(p),
                    "type mismatch: parameter type '%s': argument type '%s'",
                    parser_type_string(param_type),
                    parser_type_string(arg->type));
        }
    }

    expect(p, TOK_RPAREN);

    return call;
}

static struct parser_expr *conv_expr(struct parser *p, int kind)
{
    struct parser_type *to_type = type_spec(p);
    const struct parser_pos tokpos = tok_pos(p);

    expect(p, TOK_LPAREN);
    struct parser_expr *expr = expression(p);
    expect(p, TOK_RPAREN);

    switch (expr->type->kind) {
    case TYP_BOOL:
    case TYP_INT:
    case TYP_FLOAT:
        break;
    default:
        error(p, tokpos,
                "unable to convert type from '%s' to '%s'",
                parser_type_string(expr->type),
                parser_type_string(to_type));
        break;
    }

    return parser_new_conversion_expr(expr, to_type);
}

static struct parser_expr *array_lit_expr(struct parser *p)
{
    struct parser_expr *expr = expression(p);
    struct parser_expr *e = expr;
    const struct parser_type *elem_type = expr->type;
    int len = 1;

    while (consume(p, TOK_COMMA)) {
        e = e->next = expression(p);
        if (!parser_match_type(elem_type, e->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first element type '%s': this element type '%s'",
                    parser_type_string(elem_type),
                    parser_type_string(e->type));
        }
        len++;
    }
    expect(p, TOK_RBRACK);

    return parser_new_arraylit_expr(expr, len);
}

static struct parser_expr *struct_lit_expr(struct parser *p, struct parser_symbol *sym)
{
    struct parser_struct *strct = sym->strct;
    struct parser_expr *elems = NULL;
    struct parser_expr *e = NULL;

    expect(p, TOK_LBRACE);

    do {
        expect(p, TOK_IDENT);
        struct parser_field *field = parser_find_field(strct, tok_str(p));
        if (!field) {
            error(p, tok_pos(p),
                    "struct '%s' has no field '%s'", strct->name, tok_str(p));
        }
        expect(p, TOK_EQUAL);

        struct parser_expr *f = parser_new_field_expr(field);
        struct parser_expr *elem = parser_new_element_expr(f, expression(p));

        //struct parser_expr *expr = expression(p);
        struct parser_expr *expr = elem;
        if (!e)
            e = elems = expr;
        else
            e = e->next = expr;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_structlit_expr(strct, elems);
}

/*
 * primary_expr =
 *     IntNum |
 *     FpNum |
 *     StringLit |
 *     primary_expr selector
 */
static struct parser_expr *primary_expr(struct parser *p)
{
    if (consume(p, TOK_NIL))
        return parser_new_nillit_expr();

    if (consume(p, TOK_TRUE))
        return parser_new_boollit_expr(true);

    if (consume(p, TOK_FALSE))
        return parser_new_boollit_expr(false);

    if (consume(p, TOK_INTLIT))
        return parser_new_intlit_expr(tok_int(p));

    if (consume(p, TOK_FLOATLIT))
        return parser_new_floatlit_expr(tok_float(p));

    if (consume(p, TOK_STRINGLIT)) {
        struct parser_expr *e = parser_new_stringlit_expr(tok_str(p));
        const struct parser_token *tok = curtok(p);
        if (tok->has_escseq) {
            const int errpos = parser_convert_escape_sequence(e->sval, &e->converted);
            if (errpos != -1) {
                printf("!! e->val.s [%s] errpos %d\n", e->sval, errpos);
                struct parser_pos pos = tok->pos;
                pos.x += errpos + 1;
                error(p, pos, "unknown escape sequence");
            }
        }
        return e;
    }

    if (consume(p, TOK_LBRACK)) {
        return array_lit_expr(p);
    }

    if (consume(p, TOK_LPAREN)) {
        struct parser_expr *e = expression(p);
        expect(p, TOK_RPAREN);
        return e;
    }

    if (consume(p, TOK_CALLER_LINE)) {
        struct parser_symbol *sym = parser_find_symbol(p->scope, tok_str(p));
        if (!sym) {
            error(p, tok_pos(p),
                    "special variable '%s' not declared in parameters",
                    tok_str(p));
        }
        return parser_new_ident_expr(sym);
    }

    const int next = peek(p);
    switch (next) {
    case TOK_BOOL:
    case TOK_INT:
    case TOK_FLOAT:
        return conv_expr(p, next);
    default:
        break;
    }

    struct parser_expr *expr = NULL;

    for (;;) {
        const struct parser_token *tok = gettok(p);

        if (tok->kind == TOK_IDENT) {
            struct parser_symbol *sym = parser_find_symbol(p->scope, tok->sval);
            if (!sym) {
                error(p, tok_pos(p),
                        "undefined identifier: '%s'",
                        tok_str(p));
            }
            if (sym->kind == SYM_FUNC)
                expr = parser_new_funclit_expr(sym->func);
            else if (sym->kind == SYM_STRUCT)
                expr = struct_lit_expr(p, sym);
            else
                expr = parser_new_ident_expr(sym);
            continue;
        }
        else if (tok->kind == TOK_LPAREN) {
            if (!expr || !parser_is_func_type(expr->type)) {
                error(p, tok_pos(p),
                        "call operator must be used for function type");
            }
            /* TODO func signature check */
            struct parser_expr *call = parser_new_call_expr(expr, tok->pos);
            expr = arg_list(p, call);
            continue;
        }
        else if (tok->kind == TOK_PERIOD) {
            if (parser_is_struct_type(expr->type)) {
                expect(p, TOK_IDENT);
                struct parser_field *f = parser_find_field(expr->type->strct, tok_str(p));
                expr = parser_new_select_expr(expr, parser_new_field_expr(f));
            }
            else if (parser_is_ptr_type(expr->type) &&
                    parser_is_struct_type(expr->type->underlying)) {
                expect(p, TOK_IDENT);
                struct parser_field *f;
                f = parser_find_field(expr->type->underlying->strct, tok_str(p));
                expr = parser_new_select_expr(expr, parser_new_field_expr(f));
            }
            else if (parser_is_table_type(expr->type)) {
                expect(p, TOK_IDENT);
                struct data_hashmap_entry *ent =
                    data_hashmap_lookup(&expr->type->table->rows, tok_str(p));
                struct parser_table_row *r = ent->val;
                struct parser_expr *tmp = expr;
                expr = parser_new_intlit_expr(r->ival);
                expr->l = tmp;
            }
            else if (parser_is_module_type(expr->type)) {
                struct parser_scope *cur = p->scope;
                p->scope = expr->type->module->scope;
                struct parser_expr *r = primary_expr(p);
                p->scope = cur;

                /* TODO keep module expr somewhere */
                expr = r;
            }
            else {
                error(p, tok_pos(p),
                        "dot operator must be used for struct or table type");
            }
            continue;
        }
        else if (tok->kind == TOK_LBRACK) {
            if (!parser_is_array_type(expr->type)) {
                error(p, tok_pos(p),
                        "index operator must be used for array type");
            }
            struct parser_expr *idx = expression(p);
            if (!parser_is_int_type(idx->type)) {
                error(p, tok_pos(p),
                        "index expression must be integer type");
            }
            int64_t index = 0;
            if (parser_eval_expr(idx, &index)) {
                const int64_t len = expr->type->len;
                if (index >= len) {
                    error(p, tok_pos(p),
                            "index out of range[%d] with length %d",
                            index, len);
                }
            }
            expect(p, TOK_RBRACK);
            return parser_new_index_expr(expr, idx);
        }
        else {
            if (!expr) {
                error(p, tok->pos,
                        "unknown token for primary expression: \"%s\"",
                        parser_get_token_string(tok->kind));
            }

            ungettok(p);
            return expr;
        }
    }

    return NULL;
}

/*
 * unary_expr = primary_expr (unary_op primary_expr)*
 * unary_op   = "+" | "-" | "!" | "~"
 */
static struct parser_expr *unary_expr(struct parser *p)
{
    const struct parser_token *tok = gettok(p);
    struct parser_expr *e = NULL;

    switch (tok->kind) {

    case TOK_ASTER:
        e = unary_expr(p);
        if (!parser_is_ptr_type(e->type)) {
            error(p, tok->pos,
                    "type mismatch: * must be used for pointer type");
        }
        return parser_new_deref_expr(e);

    case TOK_AMPERSAND:
        e = unary_expr(p);
        return parser_new_addr_expr(e);

    case TOK_PLUS:
        e = unary_expr(p);
        return parser_new_posi_expr(e);

    case TOK_MINUS:
        e = unary_expr(p);
        return parser_new_nega_expr(e);

    case TOK_EXCLAM:
        e = unary_expr(p);
        return parser_new_lognot_expr(e);

    case TOK_TILDE:
        e = unary_expr(p);
        return parser_new_not_expr(e);

    default:
        ungettok(p);
        return primary_expr(p);
    }
}

static void semantic_check_type_match(struct parser *p, struct parser_pos pos,
        const struct parser_type *t0, const struct parser_type *t1)
{
    if (!parser_match_type(t0, t1)) {
        error(p, pos, "type mismatch: %s and %s",
                parser_type_string(t0), parser_type_string(t1));
    }
}

/*
 * mul_expr = unary_expr (mul_op unary_expr)*
 * mul_op   = "*" | "/" | "%" | "&" | "<<" | ">>"
 */
static struct parser_expr *mul_expr(struct parser *p)
{
    struct parser_expr *expr = unary_expr(p);
    struct parser_expr *r = NULL;

    for (;;) {
        const struct parser_token *tok = gettok(p);
        struct parser_pos pos = tok->pos;

        switch (tok->kind) {

        case TOK_ASTER:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_mul_expr(expr, r);
            break;

        case TOK_SLASH:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_div_expr(expr, r);
            break;

        case TOK_PERCENT:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_rem_expr(expr, r);
            break;

        case TOK_AMPERSAND:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_and_expr(expr, r);
            break;

        case TOK_LT2:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_shl_expr(expr, r);
            break;

        case TOK_GT2:
            r = unary_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_shr_expr(expr, r);
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

/*
 * add_expr = mul_expr (add_op mul_expr)*
 * add_op   = "+" | "-" | "|" | "^"
 */
static struct parser_expr *add_expr(struct parser *p)
{
    struct parser_expr *expr = mul_expr(p);
    struct parser_expr *r = NULL;

    for (;;) {
        const struct parser_token *tok = gettok(p);
        struct parser_pos pos = tok->pos;

        switch (tok->kind) {

        case TOK_PLUS:
            r = mul_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_add_expr(expr, r);
            break;

        case TOK_MINUS:
            r = mul_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_sub_expr(expr, r);
            break;

        case TOK_VBAR:
            r = mul_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_or_expr(expr, r);
            break;

        case TOK_CARET:
            r = mul_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_xor_expr(expr, r);
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

/*
 * rel_expr = add_expr (rel_op add_expr)*
 * rel_op   = "==" | "!=" | "<" | ">" | "<=" | ">="
 */
static struct parser_expr *rel_expr(struct parser *p)
{
    struct parser_expr *expr = add_expr(p);
    struct parser_expr *r = NULL;

    for (;;) {
        const struct parser_token *tok = gettok(p);
        struct parser_pos pos = tok->pos;

        switch (tok->kind) {

        case TOK_EQUAL2:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_eq_expr(expr, r);
            break;

        case TOK_EXCLAMEQ:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_neq_expr(expr, r);
            break;

        case TOK_LT:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_lt_expr(expr, r);
            break;

        case TOK_LTE:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_lte_expr(expr, r);
            break;

        case TOK_GT:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_gt_expr(expr, r);
            break;

        case TOK_GTE:
            r = add_expr(p);
            semantic_check_type_match(p, pos, expr->type, r->type);
            expr = parser_new_gte_expr(expr, r);
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

/*
 * logand_expr = rel_expr ("&&" rel_expr)*
 */
static struct parser_expr *logand_expr(struct parser *p)
{
    struct parser_expr *expr = rel_expr(p);

    for (;;) {
        const struct parser_token *tok = gettok(p);

        switch (tok->kind) {

        case TOK_AMPERSAND2:
            expr = parser_new_logand_expr(expr, rel_expr(p));
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

/*
 * logor_expr = logand_expr ("||" logand_expr)*
 */
static struct parser_expr *logor_expr(struct parser *p)
{
    struct parser_expr *expr = logand_expr(p);

    for (;;) {
        const struct parser_token *tok = gettok(p);

        switch (tok->kind) {

        case TOK_VBAR2:
            expr = parser_new_logor_expr(expr, logand_expr(p));
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

static struct parser_expr *expression(struct parser *p)
{
    return logor_expr(p);
}

static const struct parser_var *find_root_object(const struct parser_expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_IDENT:
        return e->var;

    case NOD_EXPR_SELECT:
        return find_root_object(e->l);

    default:
        return NULL;
    }
}

static void semantic_check_assign_stmt(struct parser *p, struct parser_pos pos,
        const struct parser_expr *lval, const struct parser_expr *rval)
{
    if (!parser_match_type(lval->type, rval->type)) {
        error(p, pos, "type mismatch: l-value type '%s': r-value type '%s'",
                parser_type_string(lval->type), parser_type_string(rval->type));
    }
    /* TODO make new_assign_stmt() */
    if (parser_is_func_type(rval->type) && rval->type->func_type->is_builtin) {
        assert(rval->kind == NOD_EXPR_FUNCLIT);
        struct parser_func *func = rval->func;
        error(p, pos, "builtin function can not be assigned: '%s'",
                func->name);
    }
    if (!parser_ast_is_mutable(lval)) {
        const struct parser_var *var = find_root_object(lval);
        assert(var);
        error(p, pos, "parameter object can not be modified: '%s'",
                var->name);
    }
}

static void semantic_check_incdec_stmt(struct parser *p, struct parser_pos pos,
        const struct parser_expr *lval)
{
    if (!parser_is_int_type(lval->type)) {
        error(p, pos, "type mismatch: ++/-- must be used for int");
    }
}

/*
 * assign_stmt = logand_expr assing_op expression
 *             | logand_expr incdec_op
 * assign_op   = "=" | "+=" | "-=" | "*=" | "/=" | "%="
 * incdec_op   = "++" | "--"
 */
static struct parser_stmt *assign_stmt(struct parser *p)
{
    struct parser_expr *lval = expression(p);
    struct parser_expr *rval = NULL;
    const struct parser_token *tok = gettok(p);
    struct parser_pos pos = tok->pos;

    switch (tok->kind) {

    case TOK_EQUAL:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_assign_stmt(lval, rval);

    case TOK_PLUSEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_addassign_stmt(lval, rval);

    case TOK_MINUSEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_subassign_stmt(lval, rval);

    case TOK_ASTEREQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_mulassign_stmt(lval, rval);

    case TOK_SLASHEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_divassign_stmt(lval, rval);

    case TOK_PERCENTEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_remassign_stmt(lval, rval);

    case TOK_PLUS2:
        semantic_check_incdec_stmt(p, pos, lval);
        return parser_new_inc_stmt(lval);

    case TOK_MINUS2:
        semantic_check_incdec_stmt(p, pos, lval);
        return parser_new_dec_stmt(lval);

    default:
        ungettok(p);
        return parser_new_expr_stmt(lval);
    }
}

static struct parser_scope *new_child_scope(struct parser *p)
{
    struct parser_scope *parent = p->scope;
    struct parser_scope *child = parser_new_scope(parent);
    struct parser_symbol *sym = parser_new_symbol(SYM_SCOPE, "_", parser_new_nil_type());

    sym->scope = child;
    parser_scope_add_symbol(parent, sym);

    return child;
}

static struct parser_stmt *or_stmt(struct parser *p)
{
    struct parser_expr *cond = NULL;

    if (consume(p, TOK_NEWLINE)) {
        /* or (else) */
        cond = NULL;
    }
    else {
        /* or if (else if) */
        cond = expression(p);
        expect(p, TOK_NEWLINE);
    }

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));

    return parser_new_else_stmt(cond, body);
}

static struct parser_stmt *if_stmt(struct parser *p)
{
    expect(p, TOK_IF);
    struct parser_expr *cond = expression(p);
    expect(p, TOK_NEWLINE);

    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    tail = tail->next = parser_new_else_stmt(cond, body);

    bool endor = false;

    while (!endor) {
        if (consume(p, TOK_ELSE)) {
            if (peek(p) == TOK_NEWLINE) {
                /* last 'or' (else) */
                endor = true;
            }

            tail = tail->next = or_stmt(p);
        }
        else {
            endor = true;
        }
    }

    return parser_new_if_stmt(head.next);
}

static struct parser_stmt *for_stmt(struct parser *p)
{
    expect(p, TOK_FOR);

    struct parser_stmt *init = NULL;
    struct parser_expr *cond = NULL;
    struct parser_stmt *post = NULL;

    if (consume(p, TOK_NEWLINE)) {
        /* infinite loop */
        init = NULL;
        cond = parser_new_intlit_expr(1);
        post = NULL;
    }
    else {
        struct parser_stmt *stmt = assign_stmt(p);

        if (consume(p, TOK_SEMICOLON)) {
            /* traditional for */
            init = stmt;
            cond = expression(p);
            expect(p, TOK_SEMICOLON);
            post = assign_stmt(p);
            expect(p, TOK_NEWLINE);
        }
        else if (consume(p, TOK_NEWLINE)) {
            /* while style */
            init = NULL;
            cond = stmt->expr;
            post = NULL;
            /* TODO need mem pool? */
            free(stmt);
        }
        else {
            const struct parser_token *tok = gettok(p);
            error(p, tok->pos, "unknown token");
        }
    }

    /* body */
    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    return parser_new_for_stmt(init, cond, post, body);
}

static struct parser_stmt *break_stmt(struct parser *p)
{
    gettok(p);
    expect(p, TOK_NEWLINE);
    return parser_new_break_stmt();
}

static struct parser_stmt *continue_stmt(struct parser *p)
{
    gettok(p);
    expect(p, TOK_NEWLINE);
    return parser_new_continue_stmt();
}

static struct parser_stmt *case_stmt(struct parser *p)
{
    struct parser_expr conds = {0};
    struct parser_expr *cond = &conds;

    do {
        struct parser_expr *expr = expression(p);
        /* TODO const int check */
        cond = cond->next = expr;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_NEWLINE);

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    return parser_new_case_stmt(conds.next, body);
}

static struct parser_stmt *default_stmt(struct parser *p)
{
    expect(p, TOK_NEWLINE);

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    return parser_new_default_stmt(body);
}

static struct parser_stmt *switch_stmt(struct parser *p)
{
    expect(p, TOK_SWITCH);

    struct parser_expr *expr = expression(p);
    /* TODO int check */
    expect(p, TOK_NEWLINE);

    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;
    int default_count = 0;

    for (;;) {
        const struct parser_token *tok = gettok(p);

        switch (tok->kind) {

        case TOK_CASE:
            if (default_count > 0) {
                error(p, tok->pos, "No 'case' should come after 'default'");
            }
            tail = tail->next = case_stmt(p);
            break;

        case TOK_DEFAULT:
            tail = tail->next = default_stmt(p);
            default_count++;
            break;

        default:
            ungettok(p);
            return parser_new_switch_stmt(expr, head.next);
        }
    }
}

static struct parser_stmt *ret_stmt(struct parser *p)
{
    expect(p, TOK_RETURN);

    const struct parser_pos exprpos = tok_pos(p);
    struct parser_expr *expr = NULL;

    if (consume(p, TOK_NEWLINE)) {
        expr = NULL;
    }
    else {
        expr = expression(p);
        expect(p, TOK_NEWLINE);
    }

    assert(p->func);

    if (expr && p->func->return_type->kind != expr->type->kind) {
        error(p, exprpos,
                "type mismatch: function type '%s': expression type '%s'",
                parser_type_string(p->func->return_type),
                parser_type_string(expr->type), "");
    }

    return parser_new_return_stmt(expr);
}

static struct parser_stmt *expr_stmt(struct parser *p)
{
    struct parser_stmt *s = assign_stmt(p);
    expect(p, TOK_NEWLINE);

    return s;
}

static struct parser_stmt *scope_stmt(struct parser *p)
{
    expect(p, TOK_MINUS3);
    expect(p, TOK_NEWLINE);

    return block_stmt(p, new_child_scope(p));
}

static struct parser_stmt *nop_stmt(struct parser *p)
{
    expect(p, TOK_NOP);

    struct parser_stmt *s = parser_new_nop_stmt();
    expect(p, TOK_NEWLINE);

    return s;
}

static struct parser_expr *default_value(const struct parser_type *type)
{
    switch (type->kind) {
    case TYP_BOOL:
        return parser_new_boollit_expr(false);
    case TYP_INT:
        return parser_new_intlit_expr(0);
    case TYP_FLOAT:
        return parser_new_floatlit_expr(0.0);
    case TYP_STRING:
        return parser_new_stringlit_expr("");

    case TYP_PTR:
        return parser_new_nillit_expr();

    case TYP_ARRAY:
        /* TODO fill with zero values */
        /* put len at base addr */
        return parser_new_intlit_expr(type->len);

    case TYP_FUNC:
    case TYP_STRUCT:
    case TYP_TABLE:
    case TYP_MODULE:
        /* TODO */
        return parser_new_nillit_expr();

    case TYP_NIL:
    case TYP_ANY:
        assert(!"unreachable");
        return NULL;
    }

    return NULL;
}

/*
 * var_decl = "-" identifier type newline
 *          | "-" identifier type = expression newline
 */
static struct parser_stmt *var_decl(struct parser *p, bool isglobal)
{
    expect(p, TOK_MINUS);
    expect(p, TOK_IDENT);

    /* var anme */
    const char *name = tok_str(p);
    const struct parser_pos ident_pos = tok_pos(p);
    struct parser_type *type = NULL;
    struct parser_expr *init = NULL;

    /* type and init */
    if (consume(p, TOK_EQUAL)) {
        /* "- x = 42" */
        init = expression(p);
        type = parser_duplicate_type(init->type);
    }
    else {
        type = type_spec(p);

        if (consume(p, TOK_EQUAL)) {
            /* "- x int = 42" */
            init = expression(p);
        }
        else {
            /* "- x int" */
            init = default_value(type);
        }
    }
    const struct parser_pos init_pos = tok_pos(p);

    expect(p, TOK_NEWLINE);

    struct parser_symbol *sym = parser_define_var(p->scope, name, type, isglobal);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }
    struct parser_expr *ident = parser_new_ident_expr(sym);
    /* TODO make new_assign_stmt() */
    if (init && parser_is_func_type(init->type) && init->type->func_type->is_builtin) {
        assert(init->kind == NOD_EXPR_FUNCLIT);
        struct parser_func *func = init->func;
        error(p, init_pos,
                "builtin function can not be assigned: '%s'",
                func->name);
    }
    return parser_new_init_stmt(ident, init);
}

static void field_list(struct parser *p, struct parser_struct *strct)
{
    expect(p, TOK_MINUS);

    do {
        expect(p, TOK_IDENT);
        const char *name = tok_str(p);

        parser_add_field(strct, name, type_spec(p));
        expect(p, TOK_NEWLINE);
    }
    while (consume(p, TOK_MINUS));
}

static struct parser_struct *struct_decl(struct parser *p)
{
    expect(p, TOK_HASH2);
    expect(p, TOK_IDENT);

    /* struct name */
    struct parser_struct *strct = parser_define_struct(p->scope, tok_str(p));
    if (!strct) {
        fprintf(stderr, "error: re-defined struct: '%s'\n", tok_str(p));
        exit(EXIT_FAILURE);
    }

    expect(p, TOK_NEWLINE);
    expect(p, TOK_BLOCKBEGIN);
    field_list(p, strct);
    expect(p, TOK_BLOCKEND);

    return strct;
}

static struct parser_table *table_def(struct parser *p)
{
    expect(p, TOK_COLON2);
    expect(p, TOK_IDENT);

    struct parser_table *tab = parser_define_table(p->scope, tok_str(p));
    if (!tab) {
        error(p, tok_pos(p), "re-defined table: '%s'", tok_str(p));
    }
    expect(p, TOK_NEWLINE);

    expect(p, TOK_BLOCKBEGIN);
    int id = 0;
    for (;;) {
        if (consume(p, TOK_VBAR)) {
            expect(p, TOK_IDENT);
            struct parser_table_row *r;

            r = calloc(1, sizeof(*r));
            r->name = tok_str(p);
            r->ival = id++;

            data_hashmap_insert(&tab->rows, tok_str(p), r);
            expect(p, TOK_NEWLINE);
        }
        else {
            break;
        }
    }
    expect(p, TOK_BLOCKEND);

    return tab;
}

static struct parser_stmt *block_stmt(struct parser *p, struct parser_scope *block_scope)
{
    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;
    bool has_stmts = true;

    /* enter scope */
    p->scope = block_scope;
    expect(p, TOK_BLOCKBEGIN);

    while (has_stmts) {
        const int next = peek(p);

        switch (next) {

        case TOK_MINUS:
            tail = tail->next = var_decl(p, false);
            break;

        case TOK_IF:
            tail = tail->next = if_stmt(p);
            break;

        case TOK_FOR:
            tail = tail->next = for_stmt(p);
            break;

        case TOK_BREAK:
            tail = tail->next = break_stmt(p);
            break;

        case TOK_CONTINUE:
            tail = tail->next = continue_stmt(p);
            break;

        case TOK_SWITCH:
            tail = tail->next = switch_stmt(p);
            break;

        case TOK_RETURN:
            tail = tail->next = ret_stmt(p);
            break;

        case TOK_MINUS3:
            tail = tail->next = scope_stmt(p);
            break;

        case TOK_NOP:
            tail = tail->next = nop_stmt(p);
            break;

        case TOK_NEWLINE:
            gettok(p);
            break;

        case TOK_BLOCKEND:
            has_stmts = false;
            break;

        default:
            tail = tail->next = expr_stmt(p);
            break;
        }
    }

    /* leave scope */
    p->scope = p->scope->parent;
    expect(p, TOK_BLOCKEND);

    return parser_new_block_stmt(head.next);
}

static void param_list(struct parser *p, struct parser_func *func)
{
    expect(p, TOK_LPAREN);

    if (consume(p, TOK_RPAREN))
        return;

    do {
        const struct parser_type *type = NULL;
        const char *name;

        if (consume(p, TOK_CALLER_LINE)) {
            name = tok_str(p);
            type = parser_new_int_type();
        }
        else {
            expect(p, TOK_IDENT);
            name = tok_str(p);
            type = type_spec(p);
        }

        parser_declare_param(func, name, type);
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RPAREN);
}

static void ret_type(struct parser *p, struct parser_func *func)
{
    const int next = peek(p);

    if (next == TOK_NEWLINE)
        func->return_type = parser_new_nil_type();
    else
        func->return_type = type_spec(p);
}

/*
 * type_spec = "bool" | "int" | "float" | "string" | identifier
 * func_type = "#" param_list type_spec?
 */
static struct parser_type *type_spec(struct parser *p)
{
    struct parser_type *type = NULL;

    if (consume(p, TOK_ASTER)) {
        return parser_new_ptr_type(type_spec(p));
    }

    if (consume(p, TOK_LBRACK)) {
        int64_t len = 0;
        if (peek(p) != TOK_RBRACK) {
            struct parser_expr *e = expression(p);
            if (!parser_is_int_type(e->type)) {
                error(p, tok_pos(p),
                        "array length expression must be integer type");
            }
            if (!parser_eval_expr(e, &len)) {
                error(p, tok_pos(p),
                        "array length expression must be compile time constant");
            }
        }
        expect(p, TOK_RBRACK);
        return parser_new_array_type(len, type_spec(p));
    }

    if (consume(p, TOK_HASH)) {
        struct parser_func *func = parser_declare_func(p->scope, "_", p->module->filename);
        /* TODO check NULL func */
        parser_module_add_func(p->module, func);
        param_list(p, func);
        ret_type(p, func);
        /* func type */
        func->func_type = parser_make_func_type(func);
        return parser_new_func_type(func->func_type);
    }

    if (consume(p, TOK_BOOL)) {
        type = parser_new_bool_type();
    }
    else if (consume(p, TOK_INT)) {
        type = parser_new_int_type();
    }
    else if (consume(p, TOK_FLOAT)) {
        type = parser_new_float_type();
    }
    else if (consume(p, TOK_STRING)) {
        type = parser_new_string_type();
    }
    else if (consume(p, TOK_IDENT)) {
        type = parser_new_struct_type(parser_find_struct(p->scope, tok_str(p)));
    }
    else {
        const struct parser_token *tok = gettok(p);
        error(p, tok->pos,
                "not a type name: '%s'",
                parser_get_token_string(tok->kind));
    }

    return type;
}

/*
 * func_def = "#" identifier param_list type_spec? newline block_stmt
 */
static void func_def(struct parser *p)
{
    expect(p, TOK_HASH);
    expect(p, TOK_IDENT);

    /* func */
    const char *name = tok_str(p);
    struct parser_pos ident_pos = tok_pos(p);
    struct parser_func *func = parser_declare_func(p->scope, name, p->module->filename);
    if (!func) {
        error(p, ident_pos, "re-defined identifier: '%s'", name);
    }
    parser_module_add_func(p->module, func);

    /* params */
    param_list(p, func);
    ret_type(p, func);
    expect(p, TOK_NEWLINE);

    /* func type */
    func->func_type = parser_make_func_type(func);

    /* func body */
    p->func = func;
    struct parser_stmt *body = block_stmt(p, func->scope);
    /* TODO control flow check to allow implicit return */
    for (struct parser_stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = parser_new_return_stmt(NULL);
            break;
        }
    }
    func->body = body;
    p->func = NULL;

    /* TODO remove this */
    if (!strcmp(func->name, "main"))
        p->module->main_func = func;
}

static void module_import(struct parser *p)
{
    expect(p, TOK_LBRACK);
    expect(p, TOK_IDENT);

    const char *modulename = tok_str(p);
    char module_filename[512] = {'\0'};

    if (strlen(modulename) > 500) {
        error(p, tok_pos(p),
                "error: too long module name: '%s'", modulename);
    }

    /* TODO have search paths */
    struct parser_search_path paths;

    sprintf(module_filename, "%s.ro", modulename);
    parser_search_path_init(&paths, p->paths->filedir);
    /* TODO can we remove os_? parser_search_path_search_file() */
    char *module_filepath = os_path_join(paths.filedir, module_filename);

    const char *src = read_file(module_filepath);
    if (!src) {
        error(p, tok_pos(p),
                "module %s.ro not found", modulename);
    }

    const struct parser_token *tok = parser_tokenize(src);
    /* TODO use this style => enter_scope(p, mod->scope); */
    parser_parse(src, module_filename, modulename, tok, p->scope, &paths);

    expect(p, TOK_RBRACK);
    expect(p, TOK_NEWLINE);

    /* clean */
    parser_search_path_free(&paths);
    free(module_filepath);
}

static void program(struct parser *p)
{
    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;
    bool eof = false;

    while (!eof) {
        int next = peek(p);

        switch (next) {

        case TOK_HASH:
            func_def(p);
            break;

        case TOK_HASH2:
            struct_decl(p);
            break;

        case TOK_COLON2:
            table_def(p);
            break;

        case TOK_MINUS:
            tail = tail->next = var_decl(p, true);
            break;

        case TOK_LBRACK:
            module_import(p);
            break;

        case TOK_NEWLINE:
            gettok(p);
            break;

        case TOK_EOF:
            eof = true;
            break;

        default:
            {
                const struct parser_token *tok = gettok(p);
                error(p, tok->pos,
                        "error: unexpected token for global object: '%s'",
                        parser_get_token_string(next));
            }
            break;
        }
    }

    p->module->gvars = head.next;
}

struct parser_module *parser_parse(const char *src,
        const char *filename, const char *modulename,
        const struct parser_token *tok, struct parser_scope *scope,
        const struct parser_search_path *paths)
{
    struct parser_module *mod;
    mod = parser_define_module(scope, filename, modulename);

    struct parser p = {0};

    p.src = src;
    p.curr = tok;
    p.scope = mod->scope;
    p.func = NULL;
    p.filename = filename;
    p.module = mod;

    p.paths = paths;

    program(&p);

    return mod;
}
