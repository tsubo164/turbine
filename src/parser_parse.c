#include "parser_parse.h"
#include "parser_escseq.h"
#include "parser_symbol.h"
#include "parser_error.h"
#include "parser_token.h"
#include "parser_eval.h"
#include "parser_type.h"
#include "builtin_module.h"
#include "lang_limits.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "data_vec.h"
#include "read_file.h"
#include "project.h"
#include "format.h"
#include "os.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

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

    /* semantics */
    struct parser_stmt *block_tail;
    bool uncond_exe;
    bool uncond_ret;

    /* context */
    struct compile_context *ctx;
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

struct parser_node_pool *node_pool(struct parser *p)
{
    return &p->ctx->node_pool;
}

static const char *type_string(struct parser *p, const struct parser_type *t)
{
    struct data_strbuf sbuf = DATA_STRBUF_INIT;
    parser_type_string(t, &sbuf);

    const char *typestr = data_string_intern(sbuf.data);
    data_strbuf_free(&sbuf);

    return typestr;
}

/* forward decls */
static struct parser_type *type_spec(struct parser *p);
static struct parser_expr *expression(struct parser *p);
static struct parser_stmt *block_stmt(struct parser *p, struct parser_scope *block_scope);
static struct parser_expr *default_value(struct parser_node_pool *pool, const struct parser_type *type);

static bool unify_template_types(
        const struct parser_type *param_type, const struct parser_type *arg_type,
        const struct parser_type **type_mapping, int max_mapping_size)
{
    if (parser_is_template_type(param_type)) {
        int id = param_type->template_id;
        assert(id < max_mapping_size);

        const struct parser_type *mapped_type = type_mapping[id];

        if (!mapped_type) {
            /* new mapping */
            type_mapping[id] = arg_type;
            return true;
        }
        else if (!parser_match_type(mapped_type, arg_type)) {
            /* already mapped but not match */
            return false;
        }
        else {
            /* already mapped and match */
            return true;
        }
    }

    if (parser_is_collection_type(param_type)) {
        if (!parser_match_type(param_type, arg_type)) {
            /* collection types not match */
            return false;
        }

        return unify_template_types(param_type->underlying, arg_type->underlying,
                type_mapping, max_mapping_size);
    }

    return true;
}

static struct parser_expr *discard_expr(struct parser *p)
{
    const char *name = "_discard";
    struct parser_symbol *sym;
    struct parser_var *var;

    expect(p, TOK_DISCARD);

    sym = parser_find_symbol(p->scope, name);

    if (sym) {
        assert(sym->kind == SYM_VAR);
        var = sym->var;
    }
    else {
        bool isglobal = false;
        const struct parser_type *type = parser_new_any_type();
        var = parser_define_var(p->scope, name, type, isglobal);
    }

    assert(var);
    var->is_discard = true;

    return parser_new_var_expr(node_pool(p), var);
}

static struct parser_expr *ident_expr(struct parser *p, bool find_parent);

static struct parser_expr *arg_list(struct parser *p, const struct parser_func_sig *func_sig,
        const struct parser_type **type_mapping, int max_mapping_size)
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
            struct parser_pos arg_pos = peek_pos(p);

            /* arg */
            if (consume(p, TOK_AMPERSAND)) {
                if (!parser_is_outparam_index(func_sig, param_idx)) {
                    error(p, arg_pos, "'&' used for non-out parameter");
                }

                struct parser_expr *ident;
                if (peek(p) == TOK_DISCARD)
                    ident = discard_expr(p);
                else
                    ident = ident_expr(p, true);

                if (ident->kind != NOD_EXPR_VAR)
                    error(p, arg_pos, "non local variable used for out parameter");
                if (parser_ast_is_global(ident))
                    error(p, arg_pos, "global variable used for out parameter");
                if (ident->var->passed_as_out)
                    error(p, arg_pos, "out variable overriten before use");
                if (ident->var->is_param && !ident->var->is_outparam)
                    error(p, arg_pos, "non-out parameter cannet be passed as output");

                ident->var->passed_as_out = true;
                ident->var->out_pos = arg_pos;
                arg = arg->next = parser_new_outarg_expr(node_pool(p), ident);
            }
            else {
                if (parser_is_outparam_index(func_sig, param_idx)) {
                    error(p, arg_pos, "missing '&' for out parameter");
                }

                arg = arg->next = expression(p);
            }

            arg->pos = arg_pos;
            arg_count++;

            /* type */
            param_type = parser_get_param_type(func_sig, param_idx);
            if (!param_type)
                error(p, arg_pos, "too many arguments");

            if (parser_has_template_type(param_type)) {
                bool unified = unify_template_types(param_type, arg->type,
                        type_mapping, max_mapping_size);

                if (!unified) {
                    error(p, arg_pos,
                            "type mismatch: parameter '%s': argument '%s'",
                            type_string(p, param_type),
                            type_string(p, arg->type));
                }
            }
            else if (!parser_match_type(arg->type, param_type)) {
                error(p, arg_pos,
                        "type mismatch: parameter '%s': argument '%s'",
                        type_string(p, param_type),
                        type_string(p, arg->type));
            }
        }
        while (consume(p, TOK_COMMA));
    }

    /* special */
    if (func_sig->has_special_var) {
        int caller_line = caller_pos.y;
        arg = arg->next = parser_new_intlit_expr(node_pool(p), caller_line);
        arg_count++;
    }

    /* count */
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
                type_string(p, from_type),
                type_string(p, to_type));
    }

    return parser_new_conversion_expr(node_pool(p), expr, to_type);
}

static struct parser_expr *vec_lit_expr(struct parser *p)
{
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;
    const struct parser_type *elem_type = NULL;
    int len = 0;

    expect(p, TOK_VEC);
    expect(p, TOK_LBRACE);

    do {
        struct parser_expr *val = expression(p);

        if (!elem_type) {
            elem_type = val->type;
        }
        else if (!parser_match_type(elem_type, val->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first value '%s': this value '%s'",
                    type_string(p, elem_type),
                    type_string(p, val->type));
        }

        elem = elem->next = val;
        len++;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_veclit_expr(node_pool(p), elem_type, elemhead.next, len);
}

static struct parser_expr *map_lit_expr(struct parser *p)
{
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;
    const struct parser_type *elem_type = NULL;
    int len = 0;

    expect(p, TOK_MAP);
    expect(p, TOK_LBRACE);

    do {
        struct parser_expr *key, *val;

        key = expression(p);
        if (!parser_is_string_type(key->type)) {
            error(p, tok_pos(p), "key expression must be string type");
        }

        expect(p, TOK_COLON);
        val = expression(p);

        if (!elem_type) {
            elem_type = val->type;
        }
        else if (!parser_match_type(elem_type, val->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first value '%s': this value '%s'",
                    type_string(p, elem_type),
                    type_string(p, val->type));
        }

        elem = elem->next = parser_new_element_expr(node_pool(p), key, val);
        len++;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_maplit_expr(node_pool(p), elem_type, elemhead.next, len);
}

static struct parser_expr *set_lit_expr(struct parser *p)
{
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;
    const struct parser_type *elem_type = NULL;
    int len = 0;

    expect(p, TOK_SET);
    expect(p, TOK_LBRACE);

    do {
        struct parser_expr *val = expression(p);

        if (!elem_type) {
            elem_type = val->type;
        }
        else if (!parser_match_type(elem_type, val->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first value '%s': this value '%s'",
                    type_string(p, elem_type),
                    type_string(p, val->type));
        }

        elem = elem->next = val;
        len++;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_setlit_expr(node_pool(p), elem_type, elemhead.next, len);
}

static struct parser_expr *stack_lit_expr(struct parser *p)
{
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;
    const struct parser_type *elem_type = NULL;
    int len = 0;

    expect(p, TOK_STACK);
    expect(p, TOK_LBRACE);

    do {
        struct parser_expr *val = expression(p);

        if (!elem_type) {
            elem_type = val->type;
        }
        else if (!parser_match_type(elem_type, val->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first value '%s': this value '%s'",
                    type_string(p, elem_type),
                    type_string(p, val->type));
        }

        elem = elem->next = val;
        len++;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_stacklit_expr(node_pool(p), elem_type, elemhead.next, len);
}

static struct parser_expr *queue_lit_expr(struct parser *p)
{
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;
    const struct parser_type *elem_type = NULL;
    int len = 0;

    expect(p, TOK_QUEUE);
    expect(p, TOK_LBRACE);

    do {
        struct parser_expr *val = expression(p);

        if (!elem_type) {
            elem_type = val->type;
        }
        else if (!parser_match_type(elem_type, val->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first value '%s': this value '%s'",
                    type_string(p, elem_type),
                    type_string(p, val->type));
        }

        elem = elem->next = val;
        len++;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RBRACE);
    return parser_new_queuelit_expr(node_pool(p), elem_type, elemhead.next, len);
}

static struct parser_expr *struct_lit_expr(struct parser *p, struct parser_symbol *sym)
{
    struct parser_struct *strct = sym->strct;
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;

    expect(p, TOK_LBRACE);

    if (peek(p) != TOK_RBRACE) {
        do {
            expect(p, TOK_IDENT);

            struct parser_struct_field *field = parser_find_struct_field(strct, tok_str(p));
            if (!field) {
                error(p, tok_pos(p),
                        "struct '%s' has no field '%s'", strct->name, tok_str(p));
            }

            expect(p, TOK_EQUAL);

            struct parser_expr *fld = parser_new_struct_field_expr(node_pool(p), field);
            struct parser_expr *val = expression(p);

            if (!parser_match_type(fld->type, val->type)) {
                error(p, tok_pos(p), "type mismatch: field %s and expression %s",
                        type_string(p, fld->type), type_string(p, val->type));
            }

            elem = elem->next = parser_new_element_expr(node_pool(p), fld, val);
        }
        while (consume(p, TOK_COMMA));

        /* omitted default values */
        struct parser_expr dflthead = {0};
        struct parser_expr *dflt = &dflthead;

        for (int i = 0; i < strct->fields.len; i++) {
            struct parser_struct_field *field = parser_get_struct_field(strct, i);
            bool already_init = false;

            for (const struct parser_expr *init = elemhead.next; init; init = init->next) {
                struct parser_struct_field *init_field = init->l->struct_field;
                if (field == init_field) {
                    already_init = true;
                    break;
                }
            }

            if (already_init)
                continue;

            if (!parser_is_vec_type(field->type) &&
                !parser_is_struct_type(field->type)) {
                continue;
            }

            struct parser_expr *fld = parser_new_struct_field_expr(node_pool(p), field);
            struct parser_expr *val = default_value(node_pool(p), field->type);
            dflt = dflt->next = parser_new_element_expr(node_pool(p), fld, val);
        }

        elem = elem->next = dflthead.next;
    }

    expect(p, TOK_RBRACE);
    return parser_new_structlit_expr(node_pool(p), sym->type, elemhead.next);
}

static struct parser_expr *enum_lit_expr(struct parser *p, struct parser_symbol *sym)
{
    struct parser_enum *enm = sym->enm;

    expect(p, TOK_PERIOD);
    expect(p, TOK_IDENT);

    int index = parser_find_enum_member(enm, tok_str(p));
    if (index < 0) {
        error(p, tok_pos(p),
                "no member named '%s' in enum '%s'", tok_str(p), enm->name);
    }

    return parser_new_enumlit_expr(node_pool(p), sym->type, index);
}

static struct parser_expr *string_lit_expr(struct parser *p)
{
    struct parser_expr *expr;

    expect(p, TOK_STRINGLIT);
    expr = parser_new_stringlit_expr(node_pool(p), tok_str(p));

    return expr;
}

static struct parser_expr *caller_line_expr(struct parser *p)
{
    struct parser_symbol *sym;

    expect(p, TOK_CALLER_LINE);
    sym = parser_find_symbol(p->scope, tok_str(p));

    if (!sym || sym->kind != SYM_VAR) {
        error(p, tok_pos(p),
                "special variable '%s' not declared in parameters",
                tok_str(p));
    }

    return parser_new_var_expr(node_pool(p), sym->var);
}

static struct parser_expr *ident_expr(struct parser *p, bool find_parent)
{
    struct parser_expr *expr;
    struct parser_symbol *sym;

    expect(p, TOK_IDENT);
    if (find_parent)
        sym = parser_find_symbol(p->scope, tok_str(p));
    else
        sym = parser_find_symbol_local(p->scope, tok_str(p));

    if (!sym) {
        error(p, tok_pos(p), "undefined identifier: '%s'", tok_str(p));
    }

    if (sym->kind == SYM_FUNC) {
        expr = parser_new_funclit_expr(node_pool(p), sym->type, sym->func);
    }
    else if (sym->kind == SYM_STRUCT) {
        expr = struct_lit_expr(p, sym);
    }
    else if (sym->kind == SYM_ENUM) {
        expr = enum_lit_expr(p, sym);
    }
    else if (sym->kind == SYM_MODULE) {
        expr = parser_new_modulelit_expr(node_pool(p), sym->type);
    }
    else if (sym->kind == SYM_VAR) {
        expr = parser_new_var_expr(node_pool(p), sym->var);
    }
    else {
        printf("unknown identifier kind: %d\n", sym->kind);
        assert(!"unreachable");
    }

    return expr;
}

/*
primary_expr ::= "nil" | "true" | "false"
    | int_lit | float_lit | string_lit | vec_lit | struct_lit
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
        return parser_new_nillit_expr(node_pool(p));

    case TOK_TRUE:
        gettok(p);
        return parser_new_boollit_expr(node_pool(p), true);

    case TOK_FALSE:
        gettok(p);
        return parser_new_boollit_expr(node_pool(p), false);

    case TOK_INTLIT:
        gettok(p);
        return parser_new_intlit_expr(node_pool(p), tok_int(p));

    case TOK_FLOATLIT:
        gettok(p);
        return parser_new_floatlit_expr(node_pool(p), tok_float(p));

    case TOK_STRINGLIT:
        return string_lit_expr(p);

    case TOK_VEC:
        return vec_lit_expr(p);

    case TOK_MAP:
        return map_lit_expr(p);

    case TOK_SET:
        return set_lit_expr(p);

    case TOK_STACK:
        return stack_lit_expr(p);

    case TOK_QUEUE:
        return queue_lit_expr(p);

    case TOK_LPAREN:
        {
            expect(p, TOK_LPAREN);
            struct parser_expr *expr = expression(p);
            expect(p, TOK_RPAREN);
            return expr;
        }

    case TOK_IDENT:
        {
            struct parser_expr *ident = ident_expr(p, true);
            if (ident->kind == NOD_EXPR_VAR)
                ident->var->passed_as_out = false;
            return ident;
        }

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

static struct parser_expr *add_packed_type_info(struct parser_node_pool *pool, struct parser_expr *args, int *argc)
{
    struct data_strbuf sbuf = DATA_STRBUF_INIT;
    struct parser_expr *arg;
    struct parser_expr *fmt;

    for (arg = args, *argc = 0; arg; arg = arg->next, (*argc)++) {
        parser_typelist_push(&sbuf, arg->type);
    }

    const char *typelist = sbuf.data ? sbuf.data : "";
    fmt = parser_new_stringlit_expr(pool, data_string_intern(typelist));
    data_strbuf_free(&sbuf);

    fmt->next = args;
    return fmt;
}

static void validate_format_string(struct parser *p, struct parser_expr *args)
{
    struct parser_expr *arg = args;

    if (arg->kind != NOD_EXPR_STRINGLIT)
        error(p, arg->pos, "the first argument must be a string literal");

    struct parser_pos fmt_pos = arg->pos;
    const char *fmt_start = arg->sval;
    const char *fmt = fmt_start;

    /* skip the first '"' */
    fmt_pos.x++;

    while (*fmt) {

        if (*fmt == '%') {
            struct format_spec spec = {0};
            bool match = false;

            fmt = format_parse_specifier(fmt, &spec, NULL, 0);

            if (spec.errmsg) {
                struct parser_pos spec_pos = fmt_pos;
                int offset = fmt - fmt_start;
                spec_pos.x += offset;
                error(p, spec_pos, spec.errmsg);
            }

            if (format_is_spec_percent(&spec))
                continue;

            /* advance arg */
            if (!arg->next)
                error(p, arg->pos, "too few arguments for format");
            arg = arg->next;

            /* check type */
            if (format_is_spec_bool(&spec)) {
                match = parser_is_bool_type(arg->type);
            }
            else if (format_is_spec_int(&spec)) {
                match = parser_is_int_type(arg->type);
            }
            else if(format_is_spec_float(&spec)) {
                match = parser_is_float_type(arg->type);
            }
            else if(format_is_spec_string(&spec)) {
                match = parser_is_string_type(arg->type);
            }
            if (!match)
                error(p, arg->pos, "type mismatch: format specifier and argument");
        }
        else {
            fmt++;
        }
    }

    if (arg->next)
        error(p, arg->pos, "too many arguments for format");
}

static struct parser_expr *call_expr(struct parser *p, struct parser_expr *base)
{
    if (!base || !parser_is_func_type(base->type))
        error(p, tok_pos(p), "'()' must be used for function type");

    struct parser_pos call_pos = tok_pos(p);

    /* for template unification */
    const struct parser_type *type_mapping[8] = {NULL};
    int max_mapping_size = sizeof(type_mapping) / sizeof(type_mapping[0]);

    const struct parser_func_sig *func_sig = base->type->func_sig;
    struct parser_expr *call;
    struct parser_expr *args;
    int argc = 0;

    args = arg_list(p, func_sig, type_mapping, max_mapping_size);

    if (func_sig->has_format_param) {
        validate_format_string(p, args);
    }
    if (parser_require_type_sequence(func_sig)) {
        args = add_packed_type_info(node_pool(p), args, &argc);
    }
    if (func_sig->is_variadic) {
        struct parser_expr *e = parser_new_intlit_expr(node_pool(p), argc);
        e->next = args;
        args = e;
    }

    call = parser_new_call_expr(node_pool(p), base, args);

    if (func_sig->has_template_return_type) {
        int id = func_sig->return_type->template_id;
        assert(id < max_mapping_size);
        const struct parser_type *mapped_type = type_mapping[id];
        if (!mapped_type) {
            error(p, call_pos, "return type not resolved: '%s'",
                    type_string(p, func_sig->return_type));
        }
        call->type = mapped_type;
    }

    return call;
}

static struct parser_expr *select_expr(struct parser *p, struct parser_expr *base)
{
    expect(p, TOK_PERIOD);

    if (parser_is_struct_type(base->type)) {
        expect(p, TOK_IDENT);
        const struct parser_struct *strct = base->type->strct;
        struct parser_struct_field *f = parser_find_struct_field(strct, tok_str(p));
        if (!f) {
            error(p, tok_pos(p), "no field named '%s' in struct '%s'",
                    tok_str(p), strct->name);
        }
        struct parser_expr *field_expr = parser_new_struct_field_expr(node_pool(p), f);
        return parser_new_struct_access_expr(node_pool(p), base, field_expr);
    }

    if (parser_is_enum_type(base->type)) {
        expect(p, TOK_IDENT);
        const struct parser_enum *enm = base->type->enm;
        struct parser_enum_field *f = parser_find_enum_field(enm, tok_str(p));
        if (!f) {
            error(p, tok_pos(p),
                    "no member named '%s' in enum '%s'", tok_str(p), enm->name);
        }
        struct parser_expr *field_expr = parser_new_enum_field_expr(node_pool(p), f);
        return parser_new_enum_access_expr(node_pool(p), base, field_expr);
    }

    if (parser_is_module_type(base->type)) {
        struct parser_scope *cur = p->scope;
        struct parser_expr *expr;
        p->scope = base->type->module->scope;
        expr = parser_new_module_access_expr(node_pool(p), base, ident_expr(p, false));
        p->scope = cur;
        return expr;
    }

    error(p, tok_pos(p), "'.' must be used for struct, enum or module type");
    return NULL;
}

static struct parser_expr *indexing_expr(struct parser *p, struct parser_expr *base)
{
    expect(p, TOK_LBRACK);

    if (!parser_is_vec_type(base->type) &&
        !parser_is_map_type(base->type)) {
        error(p, tok_pos(p), "`[]` must be used for vec or map type");
    }

    struct parser_expr *idx = expression(p);

    if (parser_is_vec_type(base->type) &&
        !parser_is_int_type(idx->type)) {
        error(p, tok_pos(p), "index expression for vec must be integer type");
    }

    if (parser_is_map_type(base->type) &&
        !parser_is_string_type(idx->type)) {
        error(p, tok_pos(p), "index expression for map must be string type");
    }

    expect(p, TOK_RBRACK);

    struct parser_expr *expr = NULL;

    if (parser_is_vec_type(base->type))
        expr = parser_new_index_expr(node_pool(p), base, idx);
    else if (parser_is_map_type(base->type))
        expr = parser_new_mapindex_expr(node_pool(p), base, idx);

    return expr;
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

    case TOK_PLUS:
        e = unary_expr(p);
        return parser_new_posi_expr(node_pool(p), e);

    case TOK_MINUS:
        e = unary_expr(p);
        return parser_new_nega_expr(node_pool(p), e);

    case TOK_EXCLAM:
        e = unary_expr(p);
        return parser_new_lognot_expr(node_pool(p), e);

    case TOK_TILDE:
        e = unary_expr(p);
        return parser_new_not_expr(node_pool(p), e);

    default:
        ungettok(p);
        return postfix_expr(p);
    }
}

static void validate_binop_type_match(struct parser *p, struct parser_pos pos,
        const struct parser_type *t0, const struct parser_type *t1)
{
    if (!parser_match_type(t0, t1)) {
        error(p, pos, "type mismatch: %s and %s",
                type_string(p, t0), type_string(p, t1));
    }
}

static const int VALID_INT[] = {TYP_INT, -1};
static const int VALID_INT_FLOAT[] = {TYP_INT, TYP_FLOAT, -1};
static const int VALID_INT_FLOAT_STRING[] = {TYP_INT, TYP_FLOAT, TYP_STRING, -1};
static const int VALID_INT_FLOAT_STRING_BOOL_ENUM[] = {
    TYP_INT, TYP_FLOAT, TYP_STRING, TYP_BOOL, TYP_ENUM, -1};

static void validate_binop_types(struct parser *p, struct parser_pos pos,
        const struct parser_type *type, const int *valid_types)
{
    for (const int *t = valid_types; *t != -1; t++) {
        if (type->kind == *t)
            return;
    }

    error(p, pos, "invalid operands to binary expression: %s",
            type_string(p, type));
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
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT);
            expr = parser_new_mul_expr(node_pool(p), expr, r);
            break;

        case TOK_SLASH:
            r = unary_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT);
            expr = parser_new_div_expr(node_pool(p), expr, r);
            break;

        case TOK_PERCENT:
            r = unary_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT);
            expr = parser_new_rem_expr(node_pool(p), expr, r);
            break;

        case TOK_AMPERSAND:
            r = unary_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT);
            expr = parser_new_and_expr(node_pool(p), expr, r);
            break;

        case TOK_LT2:
            r = unary_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT);
            expr = parser_new_shl_expr(node_pool(p), expr, r);
            break;

        case TOK_GT2:
            r = unary_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT);
            expr = parser_new_shr_expr(node_pool(p), expr, r);
            break;

        default:
            ungettok(p);
            return expr;
        }
    }
}

/*
add_expr = mul_expr (add_op mul_expr)*
add_op   = "+" | "-" | "|" | "^"
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
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING);
            expr = parser_new_add_expr(node_pool(p), expr, r);
            break;

        case TOK_MINUS:
            r = mul_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT);
            expr = parser_new_sub_expr(node_pool(p), expr, r);
            break;

        case TOK_VBAR:
            r = mul_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT);
            expr = parser_new_or_expr(node_pool(p), expr, r);
            break;

        case TOK_CARET:
            r = mul_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT);
            expr = parser_new_xor_expr(node_pool(p), expr, r);
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
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING_BOOL_ENUM);
            expr = parser_new_eq_expr(node_pool(p), expr, r);
            break;

        case TOK_EXCLAMEQ:
            r = add_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING_BOOL_ENUM);
            expr = parser_new_neq_expr(node_pool(p), expr, r);
            break;

        case TOK_LT:
            r = add_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING);
            expr = parser_new_lt_expr(node_pool(p), expr, r);
            break;

        case TOK_LTE:
            r = add_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING);
            expr = parser_new_lte_expr(node_pool(p), expr, r);
            break;

        case TOK_GT:
            r = add_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING);
            expr = parser_new_gt_expr(node_pool(p), expr, r);
            break;

        case TOK_GTE:
            r = add_expr(p);
            validate_binop_type_match(p, pos, expr->type, r->type);
            validate_binop_types(p, pos, expr->type, VALID_INT_FLOAT_STRING);
            expr = parser_new_gte_expr(node_pool(p), expr, r);
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
            expr = parser_new_logand_expr(node_pool(p), expr, rel_expr(p));
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
            expr = parser_new_logor_expr(node_pool(p), expr, logand_expr(p));
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

static struct parser_var *find_root_object(const struct parser_expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_VAR:
        return e->var;

    case NOD_EXPR_STRUCTACCESS:
        return find_root_object(e->l);

    default:
        return NULL;
    }
}

static void semantic_check_assign_stmt(struct parser *p, struct parser_pos pos,
        const struct parser_expr *lval, const struct parser_expr *rval)
{
    /* native function check first */
    if (parser_is_func_type(rval->type) && rval->type->func_sig->is_native) {
        assert(rval->kind == NOD_EXPR_FUNCLIT);
        struct parser_func *func = rval->func;
        /* TODO consider removing: natvie func cannot be assigned */
        error(p, pos, "natvie function can not be assigned: '%s'",
                func->name);
    }

    /* function signature check comes before type match */
    if (parser_is_func_type(lval->type) && parser_is_func_type(rval->type)) {
        if (!parser_match_type(lval->type, rval->type)) {
            error(p, pos,
                    "type mismatch: function signature does not match the expected type");
        }
    }

    /* type check */
    if (!parser_match_type(lval->type, rval->type)) {
        error(p, pos, "type mismatch: l-value '%s': r-value '%s'",
                type_string(p, lval->type), type_string(p, rval->type));
    }

    /* nil check */
    if (parser_is_nil_type(rval->type)) {
        error(p, pos, "invalid type: r-value '%s'", type_string(p, rval->type));
    }

    /* mutable check */
    if (!parser_ast_is_mutable(lval)) {
        const struct parser_var *var = find_root_object(lval);
        assert(var);
        if (!var->is_outparam) {
            error(p, pos, "parameter value can not be modified: '%s'",
                    var->name);
        }
    }

    /* assigned flag */
    if (parser_ast_is_outparam(lval)) {
        struct parser_var *var = find_root_object(lval);
        assert(var);
        var->is_assigned = true;
    }
}

/*
assign_stmt ::= logand_expr assign_op expression
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
        return parser_new_assign_stmt(node_pool(p), lval, rval);

    case TOK_PLUSEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_addassign_stmt(node_pool(p), lval, rval);

    case TOK_MINUSEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_subassign_stmt(node_pool(p), lval, rval);

    case TOK_ASTEREQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_mulassign_stmt(node_pool(p), lval, rval);

    case TOK_SLASHEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_divassign_stmt(node_pool(p), lval, rval);

    case TOK_PERCENTEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_remassign_stmt(node_pool(p), lval, rval);

    case TOK_LT2EQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_shlassign_stmt(node_pool(p), lval, rval);

    case TOK_GT2EQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_shrassign_stmt(node_pool(p), lval, rval);

    case TOK_VBAREQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_orassign_stmt(node_pool(p), lval, rval);

    case TOK_CARETEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_xorassign_stmt(node_pool(p), lval, rval);

    case TOK_AMPERSANDEQ:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return parser_new_andassign_stmt(node_pool(p), lval, rval);

    default:
        ungettok(p);
        return parser_new_expr_stmt(node_pool(p), lval);
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

static struct parser_stmt *cond_clause(struct parser *p, struct parser_expr *cond)
{
    struct parser_stmt *body = NULL;

    expect(p, TOK_NEWLINE);
    body = block_stmt(p, new_child_scope(p));

    return parser_new_else_stmt(node_pool(p), cond, body);
}

static struct parser_stmt *if_stmt(struct parser *p)
{
    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;
    bool uncond_ret = true;
    bool uncond_exe = p->uncond_exe;
    p->uncond_exe = false;

    expect(p, TOK_IF);
    tail = tail->next = cond_clause(p, expression(p));
    uncond_ret &= p->uncond_ret;

    while (true) {
        p->uncond_ret = false;

        if (consume(p, TOK_ELIF)) {
            tail = tail->next = cond_clause(p, expression(p));
            uncond_ret &= p->uncond_ret;
        }
        else if (consume(p, TOK_ELSE)) {
            tail = tail->next = cond_clause(p, NULL);
            uncond_ret &= p->uncond_ret;
            break;
        }
        else {
            break;
        }
    }

    p->uncond_exe = uncond_exe;
    p->uncond_ret = uncond_ret;
    return parser_new_if_stmt(node_pool(p), head.next);
}

struct loop_var {
    const char *name;
    const struct parser_type *type;
};

static struct parser_var *define_loop_vars(struct parser_scope *scope,
        const struct loop_var *loopvars)
{
    struct parser_var *firstvar = NULL;
    const struct loop_var *loopvar;

    for (loopvar = loopvars; loopvar->name; loopvar++) {
        bool isglobal = false;
        struct parser_var *var;

        var = parser_define_var(scope, loopvar->name, loopvar->type, isglobal);
        assert(var);

        if (!firstvar)
            firstvar = var;
    }

    return firstvar;
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

static struct parser_stmt *fornum_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_PERIOD2);

    struct parser_expr *stop, *step;
    struct parser_expr *iter = NULL;

    /* stop and step */
    stop = expression(p);
    if (consume(p, TOK_COMMA))
        step = expression(p);
    else
        step = parser_new_intlit_expr(node_pool(p), 1);

    collection->next = stop;
    stop->next = step;
    expect(p, TOK_NEWLINE);

    if (iter_count > 1) {
        error(p, iters[1]->pos, "too many iterators");
    }

    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { iters[0]->sval, parser_new_int_type() },
        { "_start",   parser_new_int_type() },
        { "_stop",    parser_new_int_type() },
        { "_step",    parser_new_int_type() },
        { NULL }
    };

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_fornum_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *forvec_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_idx", parser_new_int_type() },
        { "_val", collection->type->underlying },
        { "_vec", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[1].name = iters[0]->sval;
    }
    else if (iter_count == 2) {
        loop_vars[0].name = iters[0]->sval;
        loop_vars[1].name = iters[1]->sval;
    }
    else {
        error(p, iters[2]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_forvec_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *formap_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_itr", parser_new_any_type() },
        { "_key", parser_new_string_type() },
        { "_val", collection->type->underlying },
        { "_map", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[2].name = iters[0]->sval;
    }
    else if (iter_count == 2) {
        loop_vars[1].name = iters[0]->sval;
        loop_vars[2].name = iters[1]->sval;
    }
    else {
        error(p, iters[2]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_formap_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *forset_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_itr", parser_new_any_type() },
        { "_val", collection->type->underlying },
        { "_set", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[1].name = iters[0]->sval;
    }
    else {
        error(p, iters[1]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_forset_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *forstack_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_idx", parser_new_int_type() },
        { "_val", collection->type->underlying },
        { "_stack", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[1].name = iters[0]->sval;
    }
    else {
        error(p, iters[1]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_forstack_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *forqueue_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_idx", parser_new_int_type() },
        { "_val", collection->type->underlying },
        { "_queue", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[1].name = iters[0]->sval;
    }
    else {
        error(p, iters[1]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_forqueue_stmt(node_pool(p), iter, collection, body);
}

static struct parser_stmt *forenum_stmt(struct parser *p, struct parser_scope *block_scope,
        struct parser_expr *collection, const struct parser_token **iters, int iter_count)
{
    expect(p, TOK_NEWLINE);

    struct parser_expr *iter = NULL;
    struct parser_var *var = NULL;
    struct loop_var loop_vars[] = {
        { "_idx", collection->type },
        { "_stop", collection->type },
        { NULL }
    };

    if (iter_count == 1) {
        loop_vars[0].name = iters[0]->sval;
    }
    else {
        error(p, iters[1]->pos, "too many iterators");
    }

    var = define_loop_vars(block_scope, loop_vars);
    iter = parser_new_var_expr(node_pool(p), var);

    struct parser_stmt *body = block_stmt(p, block_scope);
    return parser_new_forenum_stmt(node_pool(p), iter, collection, body);
}

/*
for_stmt  ::= "for" iter_list "in" expression "\n" block_stmt
iter_list ::= ident { "," ident }
*/
static struct parser_stmt *for_stmt(struct parser *p)
{
    expect(p, TOK_FOR);

    struct parser_expr *collection = NULL;
    struct parser_stmt *fors = NULL;
    bool uncond_exe = p->uncond_exe;
    p->uncond_exe = false;
    p->uncond_ret = false;

    /* enter new scope */
    struct parser_scope *block_scope = new_child_scope(p);

    /* iterators */
    const struct parser_token *iters[4] = {NULL};
    int iter_count = iter_list(p, iters, sizeof(iters)/sizeof(iters[0]));

    expect(p, TOK_IN);

    /* check if collection is Enum type */
    if (consume(p, TOK_IDENT)) {
        struct parser_symbol *sym;
        sym = parser_find_symbol(p->scope, tok_str(p));
        if (sym->kind == SYM_ENUM) {
            collection = parser_new_enumlit_expr(node_pool(p), sym->type, 0);
        }
        else {
            ungettok(p);
        }
    }
    /* collection */
    if (!collection)
        collection = expression(p);

    if (parser_is_int_type(collection->type)) {
        fors = fornum_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_vec_type(collection->type)) {
        fors = forvec_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_map_type(collection->type)) {
        fors = formap_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_set_type(collection->type)) {
        fors = forset_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_stack_type(collection->type)) {
        fors = forstack_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_queue_type(collection->type)) {
        fors = forqueue_stmt(p, block_scope, collection, iters, iter_count);
    }
    else if (parser_is_enum_type(collection->type)) {
        fors = forenum_stmt(p, block_scope, collection, iters, iter_count);
    }
    else {
        error(p, tok_pos(p), "not an iteratable object");
    }

    p->uncond_exe = uncond_exe;
    return fors;
}

/*
while_stmt ::= "while" expression "\n" block_stmt
*/
static struct parser_stmt *while_stmt(struct parser *p)
{
    expect(p, TOK_WHILE);

    struct parser_expr *cond;
    struct parser_stmt *body;
    bool uncond_exe = p->uncond_exe;
    p->uncond_exe = false;
    p->uncond_ret = false;

    cond = expression(p);

    expect(p, TOK_NEWLINE);

    body = block_stmt(p, new_child_scope(p));

    p->uncond_exe = uncond_exe;
    return parser_new_while_stmt(node_pool(p), cond, body);
}

static struct parser_stmt *break_stmt(struct parser *p)
{
    gettok(p);
    expect(p, TOK_NEWLINE);
    return parser_new_break_stmt(node_pool(p));
}

static struct parser_stmt *continue_stmt(struct parser *p)
{
    gettok(p);
    expect(p, TOK_NEWLINE);
    return parser_new_continue_stmt(node_pool(p));
}

static struct parser_stmt *case_stmt(struct parser *p, const struct parser_type *enum_type,
        int *member_index)
{
    const struct parser_enum *enm = enum_type->enm;
    struct parser_expr *member;
    int index;

    expect(p, TOK_IDENT);

    index = parser_find_enum_member(enm, tok_str(p));
    if (index < 0) {
        error(p, tok_pos(p),
                "no member named '%s' in enum '%s'", tok_str(p), enm->name);
    }

    member = parser_new_enumlit_expr(node_pool(p), enum_type, index);
    *member_index = index;

    expect(p, TOK_NEWLINE);

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    return parser_new_case_stmt(node_pool(p), member, body);
}

static struct parser_stmt *others_stmt(struct parser *p)
{
    expect(p, TOK_NEWLINE);

    struct parser_stmt *body = block_stmt(p, new_child_scope(p));
    return parser_new_others_stmt(node_pool(p), body);
}

static void semantic_check_switch_exhaustiveness(struct parser *p,
        const struct parser_enum *enm, const bool *case_covered)
{
    int member_count = parser_get_enum_member_count(enm);
    bool is_exhaustive = true;

    for (int i = 0; i < member_count; i++) {
        if (!case_covered[i]) {
            is_exhaustive = false;
            break;
        }
    }

    if (is_exhaustive)
        return;

#define MAX_MISSING_CASES_LEN 63
    const char *others = ", and others";
    int others_len = strlen(others);
    char missing_cases[MAX_MISSING_CASES_LEN + 1] = {'\0'};
    char *pos = missing_cases;
    int total_len = 0;

    for (int i = 0; i < member_count; i++) {
        if (!case_covered[i]) {
            struct parser_enum_value val = parser_get_enum_value(enm, 0, i);
            size_t len = strlen(val.sval);

            if (MAX_MISSING_CASES_LEN - others_len - total_len < len) {
                strcpy(pos, others);
                break;
            }

            if (pos != missing_cases) {
                strcpy(pos, ", ");
                pos += 2;
                total_len += 2;
            }

            strcpy(pos, val.sval);
            pos += len;
            total_len += len;
        }
    }

    error(p, tok_pos(p),
            "missing cases in switch on enum '%s': %s", enm->name, missing_cases);
#undef MAX_MISSING_CASES_LEN
}

static struct parser_stmt *switch_stmt(struct parser *p)
{
    expect(p, TOK_SWITCH);

    struct parser_expr *expr;
    const struct parser_enum *enm;
    bool uncond_ret = true;
    bool uncond_exe = p->uncond_exe;
    p->uncond_exe = false;
    p->uncond_ret = false;

    /* enum var */
    expr = expression(p);
    if (!parser_is_enum_type(expr->type)) {
        error(p, tok_pos(p), "switch expression must be an enum");
    }
    enm = expr->type->enm;

    expect(p, TOK_NEWLINE);

    struct parser_stmt head = {0};
    struct parser_stmt *tail = &head;
    bool case_covered[LANG_MAX_ENUM_MEMBERS] = {false};
    int others_count = 0;

    while (true) {
        p->uncond_ret = false;

        if (!consume(p, TOK_ASTER)) {
            break;
        }

        if (consume(p, TOK_OTHERS)) {
            tail = tail->next = others_stmt(p);
            uncond_ret &= p->uncond_ret;
            others_count++;
        }
        else {
            int member_index = -1;
            if (others_count > 0) {
                error(p, tok_pos(p),
                        "no 'case' labels are allowed after 'default' label");
            }
            tail = tail->next = case_stmt(p, expr->type, &member_index);
            uncond_ret &= p->uncond_ret;

            case_covered[member_index] = true;
        }
    }

    /* exhaustiveness check */
    if (others_count == 0) {
        semantic_check_switch_exhaustiveness(p, enm, case_covered);
    }

    p->uncond_exe = uncond_exe;
    p->uncond_ret = uncond_ret;
    return parser_new_switch_stmt(node_pool(p), expr, head.next);
}

static struct parser_stmt *return_stmt(struct parser *p)
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

    if (expr && p->func->sig->return_type->kind != expr->type->kind) {
        error(p, exprpos,
                "type mismatch: function type '%s': expression type '%s'",
                type_string(p, p->func->sig->return_type),
                type_string(p, expr->type), "");
    }

    p->uncond_ret = true;
    return parser_new_return_stmt(node_pool(p), expr);
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

    struct parser_stmt *s = parser_new_nop_stmt(node_pool(p));
    expect(p, TOK_NEWLINE);

    return s;
}

static struct parser_expr *default_struct_lit(struct parser_node_pool *pool, const struct parser_type *type)
{
    const struct parser_struct *strct = type->strct;
    struct parser_expr elemhead = {0};
    struct parser_expr *elem = &elemhead;

    for (int i = 0; i < strct->fields.len; i++) {

        struct parser_struct_field *field = parser_get_struct_field(strct, i);

        if (parser_is_vec_type(field->type)) {
            struct parser_expr *f = parser_new_struct_field_expr(pool, field);
            struct parser_expr *e = parser_new_veclit_expr(pool, field->type->underlying,
                    NULL, 0);
            elem = elem->next = parser_new_element_expr(pool, f, e);
            continue;
        }

        if (parser_is_struct_type(field->type)) {
            struct parser_expr *f = parser_new_struct_field_expr(pool, field);
            struct parser_expr *e = default_struct_lit(pool, field->type);
            elem = elem->next = parser_new_element_expr(pool, f, e);
            continue;
        }
    }

    return parser_new_structlit_expr(pool, type, elemhead.next);
}

static struct parser_expr *default_value(struct parser_node_pool *pool, const struct parser_type *type)
{
    switch ((enum parser_type_kind) type->kind) {

    case TYP_NIL:
        return parser_new_nillit_expr(pool);

    case TYP_BOOL:
        return parser_new_boollit_expr(pool, false);

    case TYP_INT:
        return parser_new_intlit_expr(pool, 0);

    case TYP_FLOAT:
        return parser_new_floatlit_expr(pool, 0.0);

    case TYP_STRING:
        return parser_new_stringlit_expr(pool, "");

    case TYP_VEC:
        return parser_new_veclit_expr(pool, type->underlying, NULL, 0);

    case TYP_MAP:
        return parser_new_maplit_expr(pool, type->underlying, NULL, 0);

    case TYP_SET:
        return parser_new_setlit_expr(pool, type->underlying, NULL, 0);

    case TYP_STACK:
        return parser_new_stacklit_expr(pool, type->underlying, NULL, 0);

    case TYP_QUEUE:
        return parser_new_queuelit_expr(pool, type->underlying, NULL, 0);

    case TYP_STRUCT:
        return default_struct_lit(pool, type);

    case TYP_ENUM:
        return parser_new_enumlit_expr(pool, type, 0);

    case TYP_FUNC:
    case TYP_MODULE:
        /* TODO */
        return parser_new_nillit_expr(pool);

    case TYP_ANY:
    case TYP_TEMPLATE:
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
        struct parser_pos spec_pos = tok_pos(p);

        if (consume(p, TOK_EQUAL)) {
            /* "- x int = 42" */
            init = expression(p);
        }
        else {
            /* "- x int" */
            init = default_value(node_pool(p), type);
        }

        if (!parser_match_type(type, init->type)) {
            error(p, spec_pos, "type mismatch: variable '%s': initializer '%s'",
                    type_string(p, type), type_string(p, init->type));
        }
    }
    struct parser_pos init_pos = tok_pos(p);

    expect(p, TOK_NEWLINE);

    /* define var */
    struct parser_var *var = parser_define_var(p->scope, name, type, isglobal);
    if (!var)
        error(p, ident_pos, "re-defined identifier: '%s'", name);

    /* var expr */
    struct parser_expr *expr = parser_new_var_expr(node_pool(p), var);
    semantic_check_assign_stmt(p, init_pos, expr, init);

    return parser_new_init_stmt(node_pool(p), expr, init);
}

static void field_list(struct parser *p, struct parser_struct *strct)
{
    expect(p, TOK_MINUS);

    do {
        expect(p, TOK_IDENT);
        const char *name = tok_str(p);

        parser_add_struct_field(strct, name, type_spec(p));
        expect(p, TOK_NEWLINE);
    }
    while (consume(p, TOK_MINUS));
}

static struct parser_struct *struct_decl(struct parser *p, const struct parser_token *ident)
{
    struct parser_struct *strct = parser_define_struct(p->scope, ident->sval);
    if (!strct)
        error(p, ident->pos, "re-defined struct: '%s'", ident->sval);

    expect(p, TOK_STRUCT);
    expect(p, TOK_NEWLINE);

    expect(p, TOK_BLOCKBEGIN);
    field_list(p, strct);
    expect(p, TOK_BLOCKEND);

    return strct;
}

static void add_enum_value(struct parser_enum *enm, const struct parser_expr *expr)
{
    switch (expr->kind) {

    case NOD_EXPR_INTLIT:
        parser_add_enum_value_int(enm, expr->ival);
        break;

    case NOD_EXPR_FLOATLIT:
        parser_add_enum_value_float(enm, expr->fval);
        break;

    case NOD_EXPR_STRINGLIT:
        parser_add_enum_value_string(enm, expr->sval);
        break;

    default:
        assert("unknown enum value type");
        break;
    }
}

static struct parser_enum *enum_def(struct parser *p, const struct parser_token *ident)
{
    struct parser_enum *enm = parser_define_enum(p->scope, ident->sval);
    if (!enm)
        error(p, ident->pos, "re-defined enum: '%s'", ident->sval);

    expect(p, TOK_ENUM);
    expect(p, TOK_NEWLINE);
    expect(p, TOK_BLOCKBEGIN);

    /* header */
    int nfields = 0;
    expect(p, TOK_COLON);

    do {
        expect(p, TOK_IDENT);
        parser_add_enum_field(enm, tok_str(p));
        nfields++;

        if (nfields > LANG_MAX_ENUM_FIELDS) {
            error(p, tok_pos(p), "too many fields in enum '%s' (maximum is %d)",
                    enm->name, LANG_MAX_ENUM_FIELDS);
        }
    } while (consume(p, TOK_COMMA));

    expect(p, TOK_NEWLINE);

    /* members */
    int y = 0;

    do {
        struct parser_pos tag_pos;

        expect(p, TOK_MINUS);

        for (int x = 0; x < nfields; x++) {

            if (x == 0) {
                /* make tag field */
                expect(p, TOK_IDENT);
                const char *name = tok_str(p);
                tag_pos = tok_pos(p);

                /* set string type */
                if (y == 0) {
                    parser_set_enum_field_type(enm, 0, parser_new_string_type());
                }

                /* set value */
                int idx = parser_add_enum_member(enm, name);
                assert(idx == y);
                parser_add_enum_value_string(enm, name);
            }
            else {
                struct parser_expr *expr = expression(p);

                /* check const expr */
                if (!parser_ast_is_const(expr))
                    error(p, tok_pos(p), "enum field value must be a compile-time constant");

                if (y == 0) {
                    /* set type */
                    parser_set_enum_field_type(enm, x, expr->type);
                } else {
                    /* check type */
                    const struct parser_enum_field *field;
                    field = parser_get_enum_field(enm, x);
                    if (!parser_match_type(field->type, expr->type)) {
                        error(p, tok_pos(p), "type mismatch: field '%s': value '%s'",
                                type_string(p, field->type),
                                type_string(p, expr->type));
                    }
                }

                /* set value */
                add_enum_value(enm, expr);
            }

            if (x < nfields - 1)
                expect(p, TOK_COMMA);
        }
        expect(p, TOK_NEWLINE);
        y++;

        if (y > LANG_MAX_ENUM_MEMBERS) {
            error(p, tag_pos, "too many enumerators in enum '%s' (maximum is %d)",
                    enm->name, LANG_MAX_ENUM_MEMBERS);
        }
    }
    while(!consume(p, TOK_BLOCKEND));

    return enm;
}

static void struct_or_enum_def(struct parser *p)
{
    expect(p, TOK_HASH2);
    expect(p, TOK_IDENT);

    const struct parser_token *ident = curtok(p);

    if (peek(p) == TOK_STRUCT)
        struct_decl(p, ident);
    else if (peek(p) == TOK_ENUM)
        enum_def(p, ident);
}

static void semantic_check_outarg(struct parser *p)
{
    const struct parser_scope *sc = p->scope;

    for (int i = 0; i < sc->syms.len; i++) {
        const struct parser_symbol *sym = sc->syms.data[i];

        if (sym->kind == SYM_VAR) {
            const struct parser_var *var = sym->var;

            if (var->passed_as_out &&
                !var->is_outparam &&
                !var->is_discard) {
                error(p, var->out_pos, "out variable not used after call");
            }

            if (var->is_outparam &&
                !var->is_assigned &&
                !var->passed_as_out) {
                error(p, var->out_pos, "out parameter neither assigned nor passed as output");
            }
        }
    }
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
            tail = tail->next = return_stmt(p);
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

    p->block_tail = tail;
    semantic_check_outarg(p);

    /* leave scope */
    p->scope = p->scope->parent;
    expect(p, TOK_BLOCKEND);

    return parser_new_block_stmt(node_pool(p), head.next);
}

static bool is_primitive(const struct parser_type *type)
{
    if (parser_is_bool_type(type) ||
        parser_is_int_type(type) ||
        parser_is_float_type(type) ||
        parser_is_enum_type(type)) {
        return true;
    }
    else {
        return false;
    }
}

static void param_list(struct parser *p, struct parser_func *func)
{
    expect(p, TOK_LPAREN);

    if (consume(p, TOK_RPAREN))
        return;

    bool has_outparam = false;

    do {
        struct parser_var *var = NULL;
        struct parser_pos out_pos;
        const struct parser_type *type = NULL;
        const char *name = NULL;
        bool is_out = false;

        if (consume(p, TOK_CALLER_LINE)) {
            name = tok_str(p);
            type = parser_new_int_type();
        }
        else {
            if (consume(p, TOK_AMPERSAND)) {
                is_out = true;
                has_outparam = true;
                out_pos = tok_pos(p);
            }
            else if (has_outparam) {
                error(p, peek_pos(p), "regular parameter cannot follow an out parameter");
            }

            expect(p, TOK_IDENT);
            name = tok_str(p);
            type = type_spec(p);

            if (is_out && !is_primitive(type)) {
                error(p, tok_pos(p), "non-primitive type cannot be used as an out parameter");
            }
        }

        var = parser_declare_param(func, name, type, is_out);
        if (is_out)
            var->out_pos = out_pos;
    }
    while (consume(p, TOK_COMMA));

    expect(p, TOK_RPAREN);
}

static void ret_type(struct parser *p, struct parser_func *func)
{
    int next = peek(p);

    if (next == TOK_NEWLINE)
        parser_add_return_type(func, parser_new_nil_type());
    else
        parser_add_return_type(func, type_spec(p));
}

/*
type_spec = "bool" | "int" | "float" | "string" | identifier ("." type_spec)*
func_sig = "#" param_list type_spec?
*/
static struct parser_type *type_spec(struct parser *p)
{
    struct parser_type *type = NULL;

    if (consume(p, TOK_VEC)) {
        struct parser_type *underlying;
        expect(p, TOK_LBRACE);
        underlying = type_spec(p);
        expect(p, TOK_RBRACE);
        return parser_new_vec_type(underlying);
    }

    if (consume(p, TOK_MAP)) {
        struct parser_type *underlying;
        expect(p, TOK_LBRACE);
        underlying = type_spec(p);
        expect(p, TOK_RBRACE);
        return parser_new_map_type(underlying);
    }

    if (consume(p, TOK_SET)) {
        struct parser_type *underlying;
        expect(p, TOK_LBRACE);
        underlying = type_spec(p);
        expect(p, TOK_RBRACE);
        return parser_new_set_type(underlying);
    }

    if (consume(p, TOK_STACK)) {
        struct parser_type *underlying;
        expect(p, TOK_LBRACE);
        underlying = type_spec(p);
        expect(p, TOK_RBRACE);
        return parser_new_stack_type(underlying);
    }

    if (consume(p, TOK_QUEUE)) {
        struct parser_type *underlying;
        expect(p, TOK_LBRACE);
        underlying = type_spec(p);
        expect(p, TOK_RBRACE);
        return parser_new_queue_type(underlying);
    }

    if (consume(p, TOK_HASH)) {
        const char *func_name = "_lambda";
        struct parser_func *func;

        func = parser_declare_func(p->scope, p->module->filename, func_name);
        /* TODO check NULL func */
        parser_module_add_func(p->module, func);
        param_list(p, func);
        ret_type(p, func);
        return parser_new_func_type(func->sig);
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
        struct parser_pos ident_pos = tok_pos(p);
        struct parser_symbol *sym = parser_find_symbol(p->scope, tok_str(p));
        if (!sym) {
            error(p, ident_pos, "not a type name: '%s'", tok_str(p));
        }

        if (parser_is_module_type(sym->type)) {
            /* TODO consider making the type_spec a part of expression */
            expect(p, TOK_PERIOD);
            struct parser_scope *cur = p->scope;
            p->scope = sym->type->module->scope;
            type = type_spec(p);
            p->scope = cur;
        }
        else if (parser_is_struct_type(sym->type)) {
            type = parser_new_struct_type(parser_find_struct(p->scope, tok_str(p)));
        }
        else if (parser_is_enum_type(sym->type)) {
            type = parser_new_enum_type(parser_find_enum(p->scope, tok_str(p)));
        }
        else {
            error(p, ident_pos, "not a type name: '%s'", sym->name);
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

static void validate_return_stmt(struct parser *p, const struct parser_func *func)
{
    if (p->uncond_ret)
        return;

    const struct parser_type *ret_type = func->sig->return_type;
    if (parser_is_nil_type(ret_type)) {
        struct parser_expr *dflt_val = default_value(node_pool(p), ret_type);
        p->block_tail->next = parser_new_return_stmt(node_pool(p), dflt_val);
        return;
    }

    struct parser_pos end_pos = tok_pos(p);
    end_pos.y--;
    error(p, end_pos, "function must return a value of type: '%s'",
            type_string(p, ret_type));
}

static bool is_valid_main_signature(const struct parser_func *func)
{
    const struct parser_func_sig *sig = func->sig;

    if (!parser_is_int_type(sig->return_type))
        return false;

    if (parser_required_param_count(sig) != 1)
        return false;

    const struct parser_type *param_type = parser_get_param_type(sig, 0);
    if (!parser_is_vec_type(param_type))
        return false;

    if (!parser_is_string_type(param_type->underlying))
        return false;

    return true;
}

/*
func_def ::= "#" identifier param_list type_spec? newline block_stmt
*/
static void func_def(struct parser *p)
{
    expect(p, TOK_HASH);
    expect(p, TOK_IDENT);

    /* func */
    const char *name = tok_str(p);
    struct parser_pos ident_pos = tok_pos(p);
    struct parser_func *func = parser_declare_func(p->scope, p->module->filename, name);
    if (!func) {
        error(p, ident_pos, "re-defined identifier: '%s'", name);
    }
    parser_module_add_func(p->module, func);

    /* params */
    param_list(p, func);
    ret_type(p, func);
    expect(p, TOK_NEWLINE);

    /* func body */
    p->func = func;
    p->uncond_exe = true;
    p->uncond_ret = false;
    struct parser_stmt *body = block_stmt(p, func->scope);
    validate_return_stmt(p, func);

    func->body = body;
    p->func = NULL;
    p->block_tail = NULL;

    /* is main func? */
    if (!strcmp(func->name, "main")) {
        if (!is_valid_main_signature(func)) {
            error(p, ident_pos, "invalid main function signature: expected `main(args vec{string}) int`");
        }
        p->module->main_func = func;
    }
}

static void module_import(struct parser *p)
{
    expect(p, TOK_GT);
    expect(p, TOK_IDENT);

    /* module file name */
    const char *modulename = tok_str(p);
    char module_filename[512] = {'\0'};

    if (strlen(modulename) > 500) {
        error(p, tok_pos(p),
                "error: too long module name: '%s'", modulename);
    }
    sprintf(module_filename, "%s.%s", modulename, PROJECT_SRC_EXT);

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
        struct parser_import import = {0};
        bool found = parser_import_file(&import, module_filepath);

        if (!found) {
            error(p, tok_pos(p),
                    "module %s.%s not found", modulename, PROJECT_SRC_EXT);
        }
        parser_importvec_push(&p->ctx->imports, &import);

        /* parse module file */
        struct parser_token *tok = parser_tokenize(import.text, module_filename, &p->ctx->token_pool);
        struct parser_source source = {0};
        struct parser_search_path paths;

        parser_source_init(&source, import.text, module_filename, modulename);
        parser_search_path_init(&paths, p->paths->filedir);
        parser_parse(tok, p->scope, &source, &paths, p->ctx);

        /* clean */
        parser_search_path_free(&paths);
        free(module_filepath);
    } /* file module end */

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
            struct_or_enum_def(p);
            break;

        case TOK_MINUS:
            tail = tail->next = var_decl(p, true);
            break;

        case TOK_GT:
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
        const struct parser_search_path *paths,
        struct compile_context *ctx)
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
    p.ctx = ctx;

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
