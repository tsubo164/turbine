#include "parser_ast.h"
#include "parser_type.h"
#include "data_strbuf.h"
#include "data_intern.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <math.h>

const char *parser_node_string(int kind)
{

    static const char *table[] = {
    /* stmt */
    [NOD_STMT_NOP]            = "nop",
    [NOD_STMT_IF]             = "if",
    [NOD_STMT_ELSE]           = "else",
    [NOD_STMT_WHILE]          = "while",
    [NOD_STMT_FORNUM]         = "for (num)",
    [NOD_STMT_FORARRAY]       = "for (array)",
    [NOD_STMT_FORMAP]         = "for (map)",
    [NOD_STMT_FORSET]         = "for (set)",
    [NOD_STMT_BREAK]          = "break",
    [NOD_STMT_CONTINUE]       = "continue",
    [NOD_STMT_SWITCH]         = "switch",
    [NOD_STMT_CASE]           = "case",
    [NOD_STMT_DEFAULT]        = "default",
    [NOD_STMT_RETURN]         = "return",
    [NOD_STMT_EXPR]           = "expr",
    [NOD_STMT_ASSIGN]         = "assign",
    [NOD_STMT_INIT]           = "init",
    [NOD_STMT_BLOCK]          = "block",
    /* identifier */
    [NOD_EXPR_VAR]            = "var",
    [NOD_EXPR_STRUCTFIELD]    = "structfield",
    [NOD_EXPR_ENUMFIELD]      = "enumfield",
    /* literal */
    [NOD_EXPR_NILLIT]         = "nillit",
    [NOD_EXPR_BOOLLIT]        = "boollit",
    [NOD_EXPR_INTLIT]         = "intlit",
    [NOD_EXPR_FLOATLIT]       = "floatlit",
    [NOD_EXPR_STRINGLIT]      = "stringlit",
    [NOD_EXPR_FUNCLIT]        = "funclit",
    [NOD_EXPR_ARRAYLIT]       = "arraylit",
    [NOD_EXPR_MAPLIT]         = "maplit",
    [NOD_EXPR_SETLIT]         = "setlit",
    [NOD_EXPR_STRUCTLIT]      = "structlit",
    [NOD_EXPR_ENUMLIT]        = "enumlit",
    [NOD_EXPR_MODULELIT]      = "modulelit",
    /* binary */
    [NOD_EXPR_ADD]            = "+",
    [NOD_EXPR_SUB]            = "-",
    [NOD_EXPR_MUL]            = "*",
    [NOD_EXPR_DIV]            = "/",
    [NOD_EXPR_REM]            = "%",
    /* relational */
    [NOD_EXPR_EQ]             = "==",
    [NOD_EXPR_NEQ]            = "!=",
    [NOD_EXPR_LT]             = "<",
    [NOD_EXPR_LTE]            = "<=",
    [NOD_EXPR_GT]             = ">",
    [NOD_EXPR_GTE]            = ">=",
    /* bitwise */
    [NOD_EXPR_SHL]            = "<<",
    [NOD_EXPR_SHR]            = ">>",
    [NOD_EXPR_OR]             = "|",
    [NOD_EXPR_XOR]            = "^",
    [NOD_EXPR_AND]            = "&",
    /* logical */
    [NOD_EXPR_LOGOR]          = "||",
    [NOD_EXPR_LOGAND]         = "&&",
    [NOD_EXPR_LOGNOT]         = "!",
    /* unary */
    [NOD_EXPR_POS]            = "+(pos)",
    [NOD_EXPR_NEG]            = "-(neg)",
    [NOD_EXPR_NOT]            = "~",
    [NOD_EXPR_CONV]           = "conversion",
    /* array, struct, func */
    [NOD_EXPR_INDEX]          = "index",
    [NOD_EXPR_MAPINDEX]       = "mapindex",
    [NOD_EXPR_STRUCTACCESS]   = "structaccess",
    [NOD_EXPR_ENUMACCESS]     = "enumaccess",
    [NOD_EXPR_MODULEACCESS]   = "moduleaccess",
    [NOD_EXPR_CALL]           = "call",
    [NOD_EXPR_ELEMENT]        = "element",
    /* assign */
    [NOD_EXPR_ASSIGN]         = "=",
    [NOD_EXPR_ADDASSIGN]      = "+=",
    [NOD_EXPR_SUBASSIGN]      = "-=",
    [NOD_EXPR_MULASSIGN]      = "*=",
    [NOD_EXPR_DIVASSIGN]      = "/=",
    [NOD_EXPR_REMASSIGN]      = "%=",
    [NOD_EXPR_SHLASSIGN]      = "<<=",
    [NOD_EXPR_SHRASSIGN]      = ">>=",
    [NOD_EXPR_ORASSIGN]       = "|=",
    [NOD_EXPR_XORASSIGN]      = "^=",
    [NOD_EXPR_ANDASSIGN]      = "&=",
    [NOD_EXPR_INIT]           = "init",
    };
    static const int count = sizeof(table) / sizeof(table[0]);
    assert(kind >= 0 && kind < count);

    return table[kind];
}

/* eval */
static int updated_kind(int orig_kind, int new_kind)
{
    switch (orig_kind) {
    case NOD_EXPR_EQ:
    case NOD_EXPR_NEQ:
    case NOD_EXPR_GT:
    case NOD_EXPR_LT:
    case NOD_EXPR_GTE:
    case NOD_EXPR_LTE:
        return NOD_EXPR_BOOLLIT;
    default:
        return new_kind;
    }
}

static void eval_bool(struct parser_expr *e)
{
    e->kind_orig = e->kind;
    e->kind = updated_kind(e->kind, NOD_EXPR_BOOLLIT);

    switch (e->kind_orig) {
        case NOD_EXPR_LOGNOT: e->ival = !e->l->ival; break;
        case NOD_EXPR_LOGOR:  e->ival = e->l->ival || e->r->ival; break;
        case NOD_EXPR_LOGAND: e->ival = e->l->ival && e->r->ival; break;
    }
}

static void eval_int(struct parser_expr *e)
{
    e->kind_orig = e->kind;
    e->kind = updated_kind(e->kind, NOD_EXPR_INTLIT);

    switch (e->kind_orig) {
        case NOD_EXPR_NEG: e->ival = -1 * e->l->ival; break;
        case NOD_EXPR_NOT: e->ival = ~e->l->ival; break;

        case NOD_EXPR_ADD: e->ival = e->l->ival + e->r->ival; break;
        case NOD_EXPR_SUB: e->ival = e->l->ival - e->r->ival; break;
        case NOD_EXPR_MUL: e->ival = e->l->ival * e->r->ival; break;
        case NOD_EXPR_DIV: e->ival = e->l->ival / e->r->ival; break;
        case NOD_EXPR_REM: e->ival = e->l->ival % e->r->ival; break;

        case NOD_EXPR_EQ:  e->ival = e->l->ival == e->r->ival; break;
        case NOD_EXPR_NEQ: e->ival = e->l->ival != e->r->ival; break;
        case NOD_EXPR_GT:  e->ival = e->l->ival > e->r->ival; break;
        case NOD_EXPR_LT:  e->ival = e->l->ival < e->r->ival; break;
        case NOD_EXPR_GTE: e->ival = e->l->ival >= e->r->ival; break;
        case NOD_EXPR_LTE: e->ival = e->l->ival <= e->r->ival; break;

        case NOD_EXPR_SHL: e->ival = e->l->ival << e->r->ival; break;
        case NOD_EXPR_SHR: e->ival = e->l->ival >> e->r->ival; break;
        case NOD_EXPR_OR:  e->ival = e->l->ival | e->r->ival; break;
        case NOD_EXPR_XOR: e->ival = e->l->ival ^ e->r->ival; break;
        case NOD_EXPR_AND: e->ival = e->l->ival & e->r->ival; break;
    }
}

static void eval_float(struct parser_expr *e)
{
    e->kind_orig = e->kind;
    e->kind = updated_kind(e->kind, NOD_EXPR_FLOATLIT);

    switch (e->kind_orig) {
        case NOD_EXPR_NEG: e->fval = -1 * e->l->fval; break;

        case NOD_EXPR_ADD: e->fval = e->l->fval + e->r->fval; break;
        case NOD_EXPR_SUB: e->fval = e->l->fval - e->r->fval; break;
        case NOD_EXPR_MUL: e->fval = e->l->fval * e->r->fval; break;
        case NOD_EXPR_DIV: e->fval = e->l->fval / e->r->fval; break;
        case NOD_EXPR_REM: e->fval = fmod(e->l->fval, e->r->fval); break;

        case NOD_EXPR_EQ:  e->ival = e->l->fval == e->r->fval; break;
        case NOD_EXPR_NEQ: e->ival = e->l->fval != e->r->fval; break;
        case NOD_EXPR_GT:  e->ival = e->l->fval > e->r->fval; break;
        case NOD_EXPR_LT:  e->ival = e->l->fval < e->r->fval; break;
        case NOD_EXPR_GTE: e->ival = e->l->fval >= e->r->fval; break;
        case NOD_EXPR_LTE: e->ival = e->l->fval <= e->r->fval; break;
    }
}

static const char *cats(const char *s1, const char *s2)
{
    struct data_strbuf sb = DATA_STRBUF_INIT;
    const char *s3 = NULL;

    data_strbuf_copy(&sb, s1);
    data_strbuf_cat(&sb, s2);

    s3 = data_string_intern(sb.data);
    data_strbuf_free(&sb);

    return s3;
}

static void eval_string(struct parser_expr *e)
{
    e->kind_orig = e->kind;
    e->kind = updated_kind(e->kind, NOD_EXPR_STRINGLIT);

    switch (e->kind_orig) {
        case NOD_EXPR_ADD: e->sval = cats(e->l->sval, e->r->sval); break;

        case NOD_EXPR_EQ:  e->ival = strcmp(e->l->sval, e->r->sval) == 0; break;
        case NOD_EXPR_NEQ: e->ival = strcmp(e->l->sval, e->r->sval) != 0; break;
        case NOD_EXPR_GT:  e->ival = strcmp(e->l->sval, e->r->sval) > 0; break;
        case NOD_EXPR_LT:  e->ival = strcmp(e->l->sval, e->r->sval) < 0; break;
        case NOD_EXPR_GTE: e->ival = strcmp(e->l->sval, e->r->sval) >= 0; break;
        case NOD_EXPR_LTE: e->ival = strcmp(e->l->sval, e->r->sval) <= 0; break;
    }
}

static void eval(struct parser_expr *e)
{
    /* check operands type as relational ops resutl is always bool */
    const struct parser_type *type = e->l->type;

    if (parser_is_bool_type(type)) {
        eval_bool(e);
    }
    else if (parser_is_int_type(type)) {
        eval_int(e);
    }
    else if (parser_is_float_type(type)) {
        eval_float(e);
    }
    else if (parser_is_string_type(type)) {
        eval_string(e);
    }
}

/* new node */
static struct parser_expr *new_expr(int kind)
{
    struct parser_expr *e = calloc(1, sizeof(struct parser_expr));
    e->kind = kind;
    return e;
}

static struct parser_stmt *new_stmt(int kind)
{
    struct parser_stmt *s = calloc(1, sizeof(struct parser_stmt));
    s->kind = kind;
    return s;
}

/* expr */
struct parser_expr *parser_new_nillit_expr(void)
{
    struct parser_expr *e = new_expr(NOD_EXPR_NILLIT);
    e->type = parser_new_nil_type();
    return e;
}

struct parser_expr *parser_new_boollit_expr(bool b)
{
    struct parser_expr *e = new_expr(NOD_EXPR_BOOLLIT);
    e->type = parser_new_bool_type();
    e->ival = b;
    e->is_const = true;
    return e;
}

struct parser_expr *parser_new_intlit_expr(long l)
{
    struct parser_expr *e = new_expr(NOD_EXPR_INTLIT);
    e->type = parser_new_int_type();
    e->ival = l;
    e->is_const = true;
    return e;
}

struct parser_expr *parser_new_floatlit_expr(double d)
{
    struct parser_expr *e = new_expr(NOD_EXPR_FLOATLIT);
    e->type = parser_new_float_type();
    e->fval = d;
    e->is_const = true;
    return e;
}

struct parser_expr *parser_new_stringlit_expr(const char *s)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRINGLIT);
    e->type = parser_new_string_type();
    e->sval = s;
    e->is_const = true;
    return e;
}

struct parser_expr *parser_new_funclit_expr(const struct parser_type *func_type,
        struct parser_func *func)
{
    struct parser_expr *e = new_expr(NOD_EXPR_FUNCLIT);
    e->type = func_type;
    e->func = func;
    return e;
}

struct parser_expr *parser_new_arraylit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ARRAYLIT);
    e->type = parser_new_array_type(elem_type);
    e->l = parser_new_intlit_expr(len);
    e->r = elems;
    return e;
}

struct parser_expr *parser_new_maplit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len)
{
    struct parser_expr *e = new_expr(NOD_EXPR_MAPLIT);
    e->type = parser_new_map_type(elem_type);
    e->l = parser_new_intlit_expr(len);
    e->r = elems;
    return e;
}

struct parser_expr *parser_new_setlit_expr(const struct parser_type *elem_type,
        struct parser_expr *elems, int len)
{
    struct parser_expr *e = new_expr(NOD_EXPR_SETLIT);
    e->type = parser_new_set_type(elem_type);
    e->l = parser_new_intlit_expr(len);
    e->r = elems;
    return e;
}

struct parser_expr *parser_new_structlit_expr(const struct parser_type *struct_type,
        struct parser_expr *fields)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRUCTLIT);
    e->type = struct_type;
    e->l = fields;
    return e;
}

struct parser_expr *parser_new_enumlit_expr(const struct parser_type *enum_type,
        int member_idx)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ENUMLIT);
    e->type = enum_type;
    e->ival = member_idx;
    return e;
}

struct parser_expr *parser_new_modulelit_expr(const struct parser_type *module_type)
{
    struct parser_expr *e = new_expr(NOD_EXPR_MODULELIT);
    e->type = module_type;
    return e;
}

struct parser_expr *parser_new_conversion_expr(struct parser_expr *from, struct parser_type *to)
{
    struct parser_expr *e = new_expr(NOD_EXPR_CONV);
    e->type = to;
    e->l = from;
    return e;
}

struct parser_expr *parser_new_index_expr(struct parser_expr *ary, struct parser_expr *idx)
{
    struct parser_expr *e = new_expr(NOD_EXPR_INDEX);
    e->type = ary->type->underlying;
    e->l = ary;
    e->r = idx;
    return e;
}

struct parser_expr *parser_new_mapindex_expr(struct parser_expr *map, struct parser_expr *key)
{
    struct parser_expr *e = new_expr(NOD_EXPR_MAPINDEX);
    e->type = map->type->underlying;
    e->l = map;
    e->r = key;
    return e;
}

struct parser_expr *parser_new_struct_access_expr(struct parser_expr *inst, struct parser_expr *fld)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRUCTACCESS);
    e->type = fld->type;
    e->l = inst;
    e->r = fld;
    return e;
}

struct parser_expr *parser_new_enum_access_expr(struct parser_expr *enm,
        struct parser_expr *fld)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ENUMACCESS);
    e->type = fld->type;
    e->l = enm;
    e->r = fld;
    return e;
}

struct parser_expr *parser_new_module_access_expr(struct parser_expr *mod,
        struct parser_expr *member)
{
    struct parser_expr *e = new_expr(NOD_EXPR_MODULEACCESS);
    e->type = member->type;
    e->l = mod;
    e->r = member;
    return e;
}

struct parser_expr *parser_new_var_expr(struct parser_var *v)
{
    struct parser_expr *e = new_expr(NOD_EXPR_VAR);
    e->type = v->type;
    e->var = v;
    return e;
}

struct parser_expr *parser_new_struct_field_expr(struct parser_struct_field *f)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRUCTFIELD);
    e->type = f->type;
    e->struct_field = f;
    return e;
}

struct parser_expr *parser_new_enum_field_expr(struct parser_enum_field *f)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ENUMFIELD);
    e->type = f->type;
    e->enum_field = f;
    return e;
}

struct parser_expr *parser_new_call_expr(struct parser_expr *callee, struct parser_expr *args)
{
    struct parser_expr *e = new_expr(NOD_EXPR_CALL);
    e->type = callee->type->func_sig->return_type;
    e->l = callee;
    e->r = args;
    return e;
}

struct parser_expr *parser_new_element_expr(struct parser_expr *key, struct parser_expr *val)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ELEMENT);
    e->type = val->type;
    e->l = key;
    e->r = val;
    return e;
}

static struct parser_expr *new_unary_expr(struct parser_expr *l, int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->is_const = e->l->is_const;

    if (e->is_const)
        eval(e);

    return e;
}

struct parser_expr *parser_new_posi_expr(struct parser_expr *l)
{
    return new_unary_expr(l, NOD_EXPR_POS);
}

struct parser_expr *parser_new_nega_expr(struct parser_expr *l)
{
    return new_unary_expr(l, NOD_EXPR_NEG);
}

struct parser_expr *parser_new_lognot_expr(struct parser_expr *l)
{
    return new_unary_expr(l, NOD_EXPR_LOGNOT);
}

struct parser_expr *parser_new_not_expr(struct parser_expr *l)
{
    return new_unary_expr(l, NOD_EXPR_NOT);
}

static struct parser_expr *new_binary_expr(struct parser_expr *l, struct parser_expr *r,
        int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;
    e->is_const = e->l->is_const && e->r->is_const;

    if (e->is_const)
        eval(e);

    return e;
}

struct parser_expr *parser_new_add_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_ADD);
}

struct parser_expr *parser_new_sub_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SUB);
}

struct parser_expr *parser_new_mul_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_MUL);
}

struct parser_expr *parser_new_div_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_DIV);
}

struct parser_expr *parser_new_rem_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_REM);
}

struct parser_expr *parser_new_shl_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SHL);
}

struct parser_expr *parser_new_shr_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SHR);
}

struct parser_expr *parser_new_and_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_AND);
}

struct parser_expr *parser_new_or_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_OR);
}

struct parser_expr *parser_new_xor_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_XOR);
}

static struct parser_expr *new_rel_expr(struct parser_expr *l, struct parser_expr *r, int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = parser_new_bool_type();
    e->l = l;
    e->r = r;
    e->is_const = e->l->is_const && e->r->is_const;

    if (e->is_const)
        eval(e);

    return e;
}

struct parser_expr *parser_new_eq_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_EQ);
}

struct parser_expr *parser_new_neq_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_NEQ);
}

struct parser_expr *parser_new_lt_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_LT);
}

struct parser_expr *parser_new_lte_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_LTE);
}

struct parser_expr *parser_new_gt_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_GT);
}

struct parser_expr *parser_new_gte_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_GTE);
}

struct parser_expr *parser_new_logand_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_LOGAND);
}

struct parser_expr *parser_new_logor_expr(struct parser_expr *l, struct parser_expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_LOGOR);
}

/* stmt */
struct parser_stmt *parser_new_nop_stmt(void)
{
    return new_stmt(NOD_STMT_NOP);
}

struct parser_stmt *parser_new_block_stmt(struct parser_stmt *children)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_BLOCK);
    s->children = children;
    return s;
}

struct parser_stmt *parser_new_if_stmt(struct parser_stmt *or_list)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_IF);
    s->children = or_list;
    return s;
}

struct parser_stmt *parser_new_else_stmt(struct parser_expr *cond, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_ELSE);
    s->cond = cond;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_while_stmt(struct parser_expr *cond, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_WHILE);
    s->cond = cond;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_fornum_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_FORNUM);
    s->expr = iter;
    s->cond = collection;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_forarray_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_FORARRAY);
    s->expr = iter;
    s->cond = collection;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_formap_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_FORMAP);
    s->expr = iter;
    s->cond = collection;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_forset_stmt(struct parser_expr *iter,
        struct parser_expr *collection, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_FORSET);
    s->expr = iter;
    s->cond = collection;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_break_stmt(void)
{
    return new_stmt(NOD_STMT_BREAK);
}

struct parser_stmt *parser_new_continue_stmt(void)
{
    return new_stmt(NOD_STMT_CONTINUE);
}

struct parser_stmt *parser_new_switch_stmt(struct parser_expr *cond, struct parser_stmt *cases)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_SWITCH);
    s->cond = cond;
    s->children = cases;
    return s;
}

struct parser_stmt *parser_new_case_stmt(struct parser_expr *conds, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_CASE);
    s->cond = conds;
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_default_stmt(struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_DEFAULT);
    s->body = body;
    return s;
}

struct parser_stmt *parser_new_return_stmt(struct parser_expr *e)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_RETURN);
    s->expr = e;
    return s;
}

struct parser_stmt *parser_new_expr_stmt(struct parser_expr *e)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_EXPR);
    s->expr = e;
    return s;
}

struct parser_stmt *parser_new_init_stmt(struct parser_expr *l, struct parser_expr *r)
{
    struct parser_expr *e = new_expr(NOD_EXPR_INIT);
    e->type = l->type;
    e->l = l;
    e->r = r;

    struct parser_stmt *s = new_stmt(NOD_STMT_INIT);
    s->expr = e;

    return s;
}

static struct parser_stmt *new_assign_stmt(struct parser_expr *l, struct parser_expr *r,
        int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;

    struct parser_stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = e;

    return s;
}

struct parser_stmt *parser_new_assign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ASSIGN);
}

struct parser_stmt *parser_new_addassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ADDASSIGN);
}

struct parser_stmt *parser_new_subassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_SUBASSIGN);
}

struct parser_stmt *parser_new_mulassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_MULASSIGN);
}

struct parser_stmt *parser_new_divassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_DIVASSIGN);
}

struct parser_stmt *parser_new_remassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_REMASSIGN);
}

struct parser_stmt *parser_new_shlassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_SHLASSIGN);
}

struct parser_stmt *parser_new_shrassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_SHRASSIGN);
}

struct parser_stmt *parser_new_andassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ANDASSIGN);
}

struct parser_stmt *parser_new_orassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ORASSIGN);
}

struct parser_stmt *parser_new_xorassign_stmt(struct parser_expr *l, struct parser_expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_XORASSIGN);
}
