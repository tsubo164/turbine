#include "parser_parse.h"
#include "parser_search_path.h"
#include "parser_ast_eval.h"
#include "parser_escseq.h"
#include "parser_symbol.h"
#include "parser_error.h"
#include "parser_token.h"
#include "parser_type.h"
#include "parser_ast.h"
#include "builtin_module.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "os.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

/* TODO move this somewhere */
#include "data_strbuf.h"
static char *read_file(const char *filename)
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
    const struct parser_source *source;
    /* paths */
    const struct parser_search_path *paths;

    struct parser_module *module;
};

static void error(const struct parser *p, struct parser_pos pos, const char *fmt, ...)
{
    va_list args; 
    va_start(args, fmt);
    parser_error_va(p->source->text, p->source->filename, pos.x, pos.y, fmt, args);
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

static struct parser_pos peek_pos(const struct parser *p)
{
    if (p->curr->kind == TOK_EOF) {
        struct parser_pos pos = {-1, -1};
        return pos;
    }
    else
        return p->curr->next->pos;
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

static struct parser_expr *arg_list(struct parser *p,
        const struct parser_func_sig *func_sig)
{
    struct parser_pos caller_pos = tok_pos(p);
    struct parser_expr arghead = {0};
    struct parser_expr *arg = &arghead;
    int arg_count = 0;

    expect(p, TOK_LPAREN);

    if (peek(p) != TOK_RPAREN) {
        do {
            int param_idx = arg_count;
            const struct parser_type *param_type;
            const struct parser_pos arg_pos = peek_pos(p);

            arg = arg->next = expression(p);
            arg->pos = arg_pos;
            arg_count++;

            param_type = parser_get_param_type(func_sig, param_idx);
            if (!param_type)
                error(p, arg_pos, "too many arguments");

            if (!parser_match_type(arg->type, param_type)) {
                error(p, arg_pos,
                        "type mismatch: parameter '%s': argument '%s'",
                        parser_type_string(param_type),
                        parser_type_string(arg->type));
            }
        }
        while (consume(p, TOK_COMMA));
    }

    if (func_sig->has_special_var) {
        int caller_line = caller_pos.y;
        arg = arg->next = parser_new_intlit_expr(caller_line);
        arg_count++;
    }

    int param_count = parser_required_param_count(func_sig);
    if (arg_count < param_count)
        error(p, tok_pos(p), "too few arguments");

    expect(p, TOK_RPAREN);
    return arghead.next;
}

static struct parser_expr *conv_expr(struct parser *p)
{
    struct parser_type *to_type = type_spec(p);
    struct parser_pos tokpos = tok_pos(p);

    expect(p, TOK_LPAREN);
    struct parser_expr *expr = expression(p);
    const struct parser_type *from_type = expr->type;
    expect(p, TOK_RPAREN);

    if (from_type->kind != TYP_BOOL &&
        from_type->kind != TYP_INT &&
        from_type->kind != TYP_FLOAT) {
        error(p, tokpos, "unable to convert type from '%s' to '%s'",
                parser_type_string(from_type),
                parser_type_string(to_type));
    }

    return parser_new_conversion_expr(expr, to_type);
}

static struct parser_expr *array_lit_expr(struct parser *p)
{
    struct parser_expr *expr, *e;
    const struct parser_type *elem_type;
    int len = 1;

    expect(p, TOK_LBRACK);
    expr = expression(p);
    e = expr;
    elem_type = expr->type;

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
    return parser_new_arraylit_expr(elem_type, expr, len);
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

static struct parser_expr *string_lit_expr(struct parser *p)
{
    struct parser_expr *expr;
    const struct parser_token *tok;

    expect(p, TOK_STRINGLIT);

    expr = parser_new_stringlit_expr(tok_str(p));
    tok = curtok(p);

    if (tok->has_escseq) {
        int errpos = parser_convert_escape_sequence(expr->sval, &expr->converted);

        if (errpos != -1) {
            printf("!! expr->val.s [%s] errpos %d\n", expr->sval, errpos);
            struct parser_pos pos = tok->pos;
            pos.x += errpos + 1;
            error(p, pos, "unknown escape sequence");
        }
    }

    return expr;
}

static struct parser_expr *caller_line_expr(struct parser *p)
{
    struct parser_symbol *sym;

    expect(p, TOK_CALLER_LINE);
    sym = parser_find_symbol(p->scope, tok_str(p));

    if (!sym) {
        error(p, tok_pos(p),
                "special variable '%s' not declared in parameters",
                tok_str(p));
    }

    return parser_new_ident_expr(sym);
}

static struct parser_expr *ident_expr(struct parser *p)
{
    struct parser_expr *expr;
    struct parser_symbol *sym;

    expect(p, TOK_IDENT);
    sym = parser_find_symbol(p->scope, tok_str(p));

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

    return expr;
}

/*
primary_expr ::= "nil" | "true" | "false"
    | int_lit | float_lit | string_lit | array_lit | struct_lit
    | "(" expression ")"
    | indentifier
    | conv_expr
*/
static struct parser_expr *primary_expr(struct parser *p)
{
    int next = peek(p);

    switch (next) {

    case TOK_NIL:
        gettok(p);
        return parser_new_nillit_expr();

    case TOK_TRUE:
        gettok(p);
        return parser_new_boollit_expr(true);

    case TOK_FALSE:
        gettok(p);
        return parser_new_boollit_expr(false);

    case TOK_INTLIT:
        gettok(p);
        return parser_new_intlit_expr(tok_int(p));

    case TOK_FLOATLIT:
        gettok(p);
        return parser_new_floatlit_expr(tok_float(p));

    case TOK_STRINGLIT:
        return string_lit_expr(p);

    case TOK_LBRACK:
        return array_lit_expr(p);

    case TOK_LPAREN:
        {
            expect(p, TOK_LPAREN);
            struct parser_expr *expr = expression(p);
            expect(p, TOK_RPAREN);
            return expr;
        }

    case TOK_IDENT:
        return ident_expr(p);

    case TOK_CALLER_LINE:
        return caller_line_expr(p);

    case TOK_BOOL:
    case TOK_INT:
    case TOK_FLOAT:
        return conv_expr(p);

    default:
        gettok(p);
        error(p, tok_pos(p), "unknown token");
        return NULL;
    }
}

static const struct parser_type *replace_template_type(
        const struct parser_type *target_type,
        const struct parser_type *replacement_type)
{
    if (parser_is_array_type(target_type)) {
        const struct parser_type *replaced;
        replaced = replace_template_type(target_type->underlying, replacement_type);
        if (replaced)
            return parser_new_array_type(replaced);
        else
            return NULL;
    }
    else if (parser_is_template_type(target_type)) {
        return replacement_type;
    }
    else {
        return NULL;
    }
}

static const struct parser_type *find_template_type(
        const struct parser_type *param_type, const struct parser_type *arg_type,
        int target_id)
{
    if (parser_is_template_type(param_type)) {
        if (param_type->template_id == target_id)
            return arg_type;
        else
            return NULL;
    }
    else if (parser_is_array_type(param_type)) {
        return find_template_type(
                param_type->underlying, arg_type->underlying, target_id);
    }
    else {
        return NULL;
    }
}

static const struct parser_type *fill_template_type(const struct parser_func_sig *func_sig,
        const struct parser_expr *args)
{
    const struct parser_typevec *param_types = &func_sig->param_types;
    const struct parser_expr *arg = args;

    assert(parser_has_template_type(func_sig->return_type));
    int target_id = func_sig->return_type->template_id;

    for (int i = 0; i < param_types->len; i++, arg = arg->next) {
        const struct parser_type *param_type = param_types->data[i];
        const struct parser_type *found_type;

        found_type = find_template_type(param_type, arg->type, target_id);
        if (found_type) {
            return replace_template_type(func_sig->return_type, found_type);
        }
    }

    return NULL;
}

static void add_type_info(struct data_strbuf *sb, const struct parser_type *type)
{
    if (parser_is_nil_type(type)) {
        data_strbuf_cat(sb, "n");
    }
    else if (parser_is_bool_type(type)) {
        data_strbuf_cat(sb, "b");
    }
    else if (parser_is_int_type(type)) {
        data_strbuf_cat(sb, "i");
    }
    else if (parser_is_float_type(type)) {
        data_strbuf_cat(sb, "f");
    }
    else if (parser_is_string_type(type)) {
        data_strbuf_cat(sb, "s");
    }
    else if (parser_is_array_type(type)) {
        data_strbuf_cat(sb, "a");
        add_type_info(sb, type->underlying);
    }
}

static struct parser_expr *add_packed_type_info(struct parser_expr *args)
{
    struct data_strbuf sbuf = DATA_STRBUF_INIT;
    struct parser_expr *arg;
    struct parser_expr *fmt;

    for (arg = args; arg; arg = arg->next) {
        add_type_info(&sbuf, arg->type);
    }

    fmt = parser_new_stringlit_expr(data_string_intern(sbuf.data));
    data_strbuf_free(&sbuf);

    fmt->next = args;
    return fmt;
}

static void validate_format_string(struct parser *p, struct parser_expr *args)
{
    struct parser_expr *arg = args;

    if (arg->kind != NOD_EXPR_STRINGLIT)
        error(p, arg->pos, "the first argument must be a string literal");

    //struct parser_pos fmt_pos = arg->pos;
    struct parser_pos arg_pos = arg->pos;
    const char *fmt = arg->sval;
    bool match = true;
    arg = arg->next;

    while (*fmt) {
        int c = *fmt++;

        if (c == '%') {
            c = *fmt++;

            switch (c) {
            case 'd':
                if (!arg)
                    error(p, arg_pos, "too few arguments for format");
                match = parser_is_int_type(arg->type);
                arg_pos = arg->pos;
                arg = arg->next;
                break;

            default:
                error(p, arg_pos, "invalid format specifier '%%%c'", c);
            }
        }

        if (!match) {
            error(p, arg_pos, "type mismatch: format specifier '%%%c' and argument", c);
        }
    }

    if (arg)
        error(p, arg->pos, "too many arguments for format");
}

static struct parser_expr *call_expr(struct parser *p, struct parser_expr *base)
{
    if (!base || !parser_is_func_type(base->type))
        error(p, tok_pos(p), "'()' must be used for function type");

    const struct parser_func_sig *func_sig = base->type->func_sig;
    struct parser_expr *call;
    struct parser_expr *args;

    args = arg_list(p, func_sig);

    if (func_sig->has_format_param) {
        validate_format_string(p, args);
    }
    if (func_sig->is_variadic) {
        args = add_packed_type_info(args);
    }

    call = parser_new_call_expr(base, args);

    if (func_sig->has_template_return_type) {
        const struct parser_type *filled_type;
        filled_type = fill_template_type(func_sig, args);

        if (filled_type)
            call->type = filled_type;
    }

    return call;
}

static struct parser_expr *select_expr(struct parser *p, struct parser_expr *base)
{
    expect(p, TOK_PERIOD);

    if (parser_is_struct_type(base->type)) {
        expect(p, TOK_IDENT);
        struct parser_field *f = parser_find_field(base->type->strct, tok_str(p));
        return parser_new_select_expr(base, parser_new_field_expr(f));
    }

    if (parser_is_ptr_type(base->type) &&
            parser_is_struct_type(base->type->underlying)) {
        expect(p, TOK_IDENT);
        struct parser_field *f;
        f = parser_find_field(base->type->underlying->strct, tok_str(p));
        return parser_new_select_expr(base, parser_new_field_expr(f));
    }

    if (parser_is_table_type(base->type)) {
        expect(p, TOK_IDENT);
        struct data_hashmap_entry *ent =
            data_hashmap_lookup(&base->type->table->rows, tok_str(p));
        struct parser_table_row *r = ent->val;
        struct parser_expr *expr;
        expr = parser_new_intlit_expr(r->ival);
        /* TODO come up with better idea rather than keeping it in intlit */
        expr->l = base;
        return expr;
    }

    if (parser_is_module_type(base->type)) {
        struct parser_scope *cur = p->scope;
        struct parser_expr *expr;
        p->scope = base->type->module->scope;
        expr = parser_new_module_expr(base, ident_expr(p));
        p->scope = cur;
        return expr;
    }

    error(p, tok_pos(p), "'.' must be used for struct, table or modlue type");
    return NULL;
}

static struct parser_expr *indexing_expr(struct parser *p, struct parser_expr *base)
{
    expect(p, TOK_LBRACK);

    if (!parser_is_array_type(base->type))
        error(p, tok_pos(p), "`[]` must be used for array type");

    struct parser_expr *idx = expression(p);

    if (!parser_is_int_type(idx->type))
        error(p, tok_pos(p), "index expression must be integer type");

    expect(p, TOK_RBRACK);
    return parser_new_index_expr(base, idx);
}

/*
postfix_expr ::= primary_expr
    | postfix_expr "." identifier
    | postfix_expr "[" expression "]"
    | postfix_expr "(" arg_list ")"
*/
static struct parser_expr *postfix_expr(struct parser *p)
{
    struct parser_expr *expr = primary_expr(p);

    for (;;) {
        int next = peek(p);

        switch (next) {

        case TOK_LPAREN:
            expr = call_expr(p, expr);
            continue;

        case TOK_PERIOD:
            expr = select_expr(p, expr);
            continue;

        case TOK_LBRACK:
            expr = indexing_expr(p, expr);
            continue;

        default:
            if (!expr) {
                const struct parser_token *tok = gettok(p);
                error(p, tok->pos, "unknown token in postfix expression");
            }
            return expr;
        }
    }

    return NULL;
}

/*
unary_expr ::= primary_expr (unary_op primary_expr)*
unary_op   ::= "+" | "-" | "!" | "~"
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
        return postfix_expr(p);
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
        error(p, pos, "type mismatch: l-value '%s': r-value '%s'",
                parser_type_string(lval->type), parser_type_string(rval->type));
    }
    /* TODO make new_assign_stmt() */
    if (parser_is_func_type(rval->type) && rval->type->func_sig->is_builtin) {
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

/*
assign_stmt ::= logand_expr assing_op expression
assign_op   ::= "=" | "+=" | "-=" | "*=" | "/=" | "%="
            | "<<=" | ">>=" | "|=" | "^=" | "&="
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

    case TOK_LT2EQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_shlassign_stmt(lval, rval);

    case TOK_GT2EQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_shrassign_stmt(lval, rval);

    case TOK_VBAREQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_orassign_stmt(lval, rval);

    case TOK_CARETEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_xorassign_stmt(lval, rval);

    case TOK_AMPERSANDEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_andassign_stmt(lval, rval);

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

static struct parser_symbol *define_loop_var(struct parser_scope *scope,
        const char *name, const struct parser_type *type)
{
    bool isglobal = false;
    struct parser_symbol *sym = parser_define_var(scope, name, type, isglobal);
    assert(sym);

    return sym;
}

static int iter_list(struct parser *p, const struct parser_token **iters, int max_iters)
{
    int index = 0;

    do {
        if (index >= max_iters)
            error(p, tok_pos(p), "too many iterators");

        expect(p, TOK_IDENT);
        iters[index++] = curtok(p);

    } while (consume(p, TOK_COMMA));

    return index;
}

/*
for_stmt  ::= "for" iter_list "in" expression "\n" block_stmt
iter_list ::= ident { "," ident }
*/
static struct parser_stmt *for_stmt(struct parser *p)
{
    expect(p, TOK_FOR);

    struct parser_expr *collection = NULL;
    struct parser_expr *iter = NULL;

    /* enter new scope */
    struct parser_scope *block_scope = new_child_scope(p);

    /* iterators */
    const struct parser_token *iters[3] = {NULL};
    int iter_count = iter_list(p, iters, sizeof(iters)/sizeof(iters[0]));

    expect(p, TOK_IN);

    /* collection */
    collection = expression(p);

    if (parser_is_int_type(collection->type)) {
        struct parser_expr *start, *stop, *step;
        expect(p, TOK_PERIOD2);

        start = collection;
        stop = expression(p);
        if (consume(p, TOK_COMMA))
            step = expression(p);
        else
            step = parser_new_intlit_expr(1);

        collection->next = stop;
        stop->next = step;
        expect(p, TOK_NEWLINE);

        if (iter_count > 1) {
            error(p, iters[1]->pos, "too many iterators");
        }
        struct parser_symbol *sym;

        sym = define_loop_var(block_scope, iters[0]->sval, parser_new_int_type());
        define_loop_var(block_scope, "_start", parser_new_int_type());
        define_loop_var(block_scope, "_stop", parser_new_int_type());
        define_loop_var(block_scope, "_step", parser_new_int_type());

        iter = parser_new_ident_expr(sym);

        struct parser_stmt *body = block_stmt(p, block_scope);
        return parser_new_fornum_stmt(iter, collection, body);
    }
    else if (parser_is_array_type(collection->type)) {

        expect(p, TOK_NEWLINE);

        struct parser_symbol *sym = NULL;

        if (iter_count == 1) {
            sym = define_loop_var(block_scope, "_index", parser_new_int_type());
            define_loop_var(block_scope, iters[0]->sval, collection->type->underlying);
            define_loop_var(block_scope, "_array", collection->type);
        }
        else if (iter_count == 2) {
            sym = define_loop_var(block_scope, iters[0]->sval, parser_new_int_type());
            define_loop_var(block_scope, iters[1]->sval, collection->type->underlying);
            define_loop_var(block_scope, "_array", collection->type);
        }
        else {
            error(p, iters[2]->pos, "too many iterators");
        }

        iter = parser_new_ident_expr(sym);

        struct parser_stmt *body = block_stmt(p, block_scope);
        return parser_new_forarray_stmt(iter, collection, body);
    }
    else {
        error(p, tok_pos(p), "not an iteratable object");
        return NULL;
    }
}

/*
while_stmt ::= "while" expression "\n" block_stmt
*/
static struct parser_stmt *while_stmt(struct parser *p)
{
    struct parser_expr *cond;
    struct parser_stmt *body;

    expect(p, TOK_WHILE);

    cond = expression(p);

    expect(p, TOK_NEWLINE);

    body = block_stmt(p, new_child_scope(p));
    return parser_new_while_stmt(cond, body);
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
        return parser_new_arraylit_expr(type->underlying, NULL, 0);

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

    if (isglobal && name[0] != '_')
        error(p, ident_pos,
                "global variable names must have exactly one leading"
                "and one trailing underscore");

    if (!isglobal && name[0] == '_')
        error(p, ident_pos,
                "global variable name used in local scope");

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
    if (init && parser_is_func_type(init->type) && init->type->func_sig->is_builtin) {
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
        int next = peek(p);

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

        case TOK_WHILE:
            tail = tail->next = while_stmt(p);
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
    int next = peek(p);

    if (next == TOK_NEWLINE)
        func->return_type = parser_new_nil_type();
    else
        func->return_type = type_spec(p);
}

/*
 * type_spec = "bool" | "int" | "float" | "string" | identifier ("." type_spec)*
 * func_sig = "#" param_list type_spec?
 */
static struct parser_type *type_spec(struct parser *p)
{
    struct parser_type *type = NULL;

    if (consume(p, TOK_ASTER)) {
        return parser_new_ptr_type(type_spec(p));
    }

    if (consume(p, TOK_LBRACK)) {
        expect(p, TOK_RBRACK);
        return parser_new_array_type(type_spec(p));
    }

    if (consume(p, TOK_HASH)) {
        struct parser_func *func = parser_declare_func(p->scope, "_", p->module->filename);
        /* TODO check NULL func */
        parser_module_add_func(p->module, func);
        param_list(p, func);
        ret_type(p, func);
        /* func sig */
        func->func_sig = parser_make_func_sig(func);
        return parser_new_func_type(func->func_sig);
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
        struct parser_symbol *sym = parser_find_symbol(p->scope, tok_str(p));
        if (parser_is_module_type(sym->type)) {
            /* TODO consider making the type_spec a part of expression */
            expect(p, TOK_PERIOD);
            struct parser_scope *cur = p->scope;
            p->scope = sym->type->module->scope;
            type = type_spec(p);
            p->scope = cur;
        }
        else {
            type = parser_new_struct_type(parser_find_struct(p->scope, tok_str(p)));
        }
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

    /* func sig */
    func->func_sig = parser_make_func_sig(func);

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

    /* module file name */
    const char *modulename = tok_str(p);
    char module_filename[512] = {'\0'};

    if (strlen(modulename) > 500) {
        error(p, tok_pos(p),
                "error: too long module name: '%s'", modulename);
    }
    sprintf(module_filename, "%s.ro", modulename);

    /* builtin modules */
    const struct builtin_module *found_module;
    found_module = builtin_find_module(p->paths->builtin_modules, modulename);

    if (found_module) {
        builtin_import_module(p->scope, found_module);
    }
    else {
        /* TODO consider making parse_module_file() */
        /* read module file */
        char *module_filepath = parser_search_path_find(p->paths, module_filename);
        char *text = read_file(module_filepath);

        if (!text) {
            error(p, tok_pos(p),
                    "module %s.ro not found", modulename);
        }

        /* parse module file */
        const struct parser_token *tok = parser_tokenize(text, module_filename);
        struct parser_source source = {0};
        struct parser_search_path paths;

        parser_source_init(&source, text, module_filename, modulename);
        parser_search_path_init(&paths, p->paths->filedir);
        parser_parse(tok, p->scope, &source, &paths);

        /* clean */
        parser_search_path_free(&paths);
        free(module_filepath);
        free(text);
    } /* file module end */

    expect(p, TOK_RBRACK);
    expect(p, TOK_NEWLINE);
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

struct parser_module *parser_parse(const struct parser_token *tok,
        struct parser_scope *scope,
        const struct parser_source *source,
        const struct parser_search_path *paths)
{
    struct parser_module *mod;
    mod = parser_define_module(scope, source->filename, source->modulename);

    struct parser p = {0};

    p.curr = tok;
    p.scope = mod->scope;
    p.func = NULL;
    p.source = source;
    p.module = mod;
    p.paths = paths;

    program(&p);

    return mod;
}

void parser_source_init(struct parser_source *source,
        const char *text, const char *filename, const char *modulename)
{
    source->text = text;
    source->filename = filename;
    source->modulename = modulename;
}
