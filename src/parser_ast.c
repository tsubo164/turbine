#include "parser_ast.h"
#include "scope.h"
#include "type.h"

#include <stdlib.h>

const struct parser_node_info *parser_get_node_info(int kind)
{
    static const struct parser_node_info table[] = {
    /* stmt */
    [NOD_STMT_NOP]            = {"nop"},
    [NOD_STMT_IF]             = {"if"},
    [NOD_STMT_FOR]            = {"for"},
    [NOD_STMT_ELSE]           = {"else"},
    [NOD_STMT_BREAK]          = {"break"},
    [NOD_STMT_CONTINUE]       = {"continue"},
    [NOD_STMT_SWITCH]         = {"switch"},
    [NOD_STMT_CASE]           = {"case"},
    [NOD_STMT_DEFAULT]        = {"default"},
    [NOD_STMT_RETURN]         = {"return"},
    [NOD_STMT_EXPR]           = {"expr"},
    [NOD_STMT_ASSIGN]         = {"assign"},
    [NOD_STMT_INIT]           = {"init"},
    [NOD_STMT_BLOCK]          = {"block"},
    /* identifier */
    [NOD_EXPR_FIELD]          = {"field", 'g'},
    [NOD_EXPR_IDENT]          = {"ident", 'y'},
    /* literal */
    [NOD_EXPR_NILLIT]         = {"nillit"},
    [NOD_EXPR_BOOLLIT]        = {"boollit",   'i'},
    [NOD_EXPR_INTLIT]         = {"intlit",    'i'},
    [NOD_EXPR_FLOATLIT]       = {"floatlit",  'f'},
    [NOD_EXPR_STRINGLIT]      = {"stringlit", 's'},
    [NOD_EXPR_FUNCLIT]        = {"funclit",   'F'},
    [NOD_EXPR_ARRAYLIT]       = {"arraylit"},
    [NOD_EXPR_STRUCTLIT]      = {"structlit"},
    /* binary */
    [NOD_EXPR_ADD]            = {"+"},
    [NOD_EXPR_SUB]            = {"-"},
    [NOD_EXPR_MUL]            = {"*"},
    [NOD_EXPR_DIV]            = {"/"},
    [NOD_EXPR_REM]            = {"%"},
    /* relational */
    [NOD_EXPR_EQ]             = {"=="},
    [NOD_EXPR_NEQ]            = {"!="},
    [NOD_EXPR_LT]             = {"<"},
    [NOD_EXPR_LTE]            = {"<="},
    [NOD_EXPR_GT]             = {">"},
    [NOD_EXPR_GTE]            = {">="},
    /* bitwise */
    [NOD_EXPR_SHL]            = {"<<"},
    [NOD_EXPR_SHR]            = {">>"},
    [NOD_EXPR_OR]             = {"|"},
    [NOD_EXPR_XOR]            = {"^"},
    [NOD_EXPR_AND]            = {"&"},
    /* logical */
    [NOD_EXPR_LOGOR]          = {"||"},
    [NOD_EXPR_LOGAND]         = {"&&"},
    [NOD_EXPR_LOGNOT]         = {"!"},
    /* unary */
    [NOD_EXPR_POS]            = {"+(pos)"},
    [NOD_EXPR_NEG]            = {"-(neg)"},
    [NOD_EXPR_ADDRESS]        = {"address"},
    [NOD_EXPR_DEREF]          = {"deref"},
    [NOD_EXPR_NOT]            = {"~"},
    [NOD_EXPR_INC]            = {"++"},
    [NOD_EXPR_DEC]            = {"--"},
    [NOD_EXPR_CONV]           = {"conversion"},
    /* array, struct, func */
    [NOD_EXPR_SELECT]         = {"select"},
    [NOD_EXPR_INDEX]          = {"index"},
    [NOD_EXPR_CALL]           = {"call"},
    [NOD_EXPR_ELEMENT]        = {"element"},
    /* assign */
    [NOD_EXPR_ASSIGN]         = {"="},
    [NOD_EXPR_ADDASSIGN]      = {"+="},
    [NOD_EXPR_SUBASSIGN]      = {"-="},
    [NOD_EXPR_MULASSIGN]      = {"*="},
    [NOD_EXPR_DIVASSIGN]      = {"/="},
    [NOD_EXPR_REMASSIGN]      = {"%="},
    [NOD_EXPR_INIT]           = {"init"},           
    };

    int N = sizeof(table) / sizeof(table[0]);

    if (kind < 0 || kind >= N)
        return NULL;

    return &table[kind];
}

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
    e->type = NewNilType();
    return e;
}

struct parser_expr *parser_new_boollit_expr(bool b)
{
    struct parser_expr *e = new_expr(NOD_EXPR_BOOLLIT);
    e->type = NewBoolType();
    e->ival = b;
    return e;
}

struct parser_expr *parser_new_intlit_expr(long l)
{
    struct parser_expr *e = new_expr(NOD_EXPR_INTLIT);
    e->type = NewIntType();
    e->ival = l;
    return e;
}

struct parser_expr *parser_new_floatlit_expr(double d)
{
    struct parser_expr *e = new_expr(NOD_EXPR_FLOATLIT);
    e->type = NewFloatType();
    e->fval = d;
    return e;
}

struct parser_expr *parser_new_stringlit_expr(const char *s)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRINGLIT);
    e->type = NewStringType();
    e->sval = s;
    return e;
}

struct parser_expr *parser_new_funclit_expr(struct Func *func)
{
    struct parser_expr *e = new_expr(NOD_EXPR_FUNCLIT);
    e->type = NewFuncType(func->func_type);
    e->func = func;
    return e;
}

struct parser_expr *parser_new_arraylit_expr(struct parser_expr *elems, int len)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ARRAYLIT);
    e->type = NewArrayType(len, elems->type);
    e->l = elems;
    return e;
}

struct parser_expr *parser_new_structlit_expr(struct Struct *strct, struct parser_expr *fields)
{
    struct parser_expr *e = new_expr(NOD_EXPR_STRUCTLIT);
    e->type = NewStructType(strct);
    e->l = fields;
    return e;
}

struct parser_expr *parser_new_conversion_expr(struct parser_expr *from, struct Type *to)
{
    struct parser_expr *e = new_expr(NOD_EXPR_CONV);
    e->type = to;
    e->l = from;
    return e;
}

struct parser_expr *parser_new_ident_expr(struct Symbol *sym)
{
    struct parser_expr *e = new_expr(NOD_EXPR_IDENT);
    e->type = sym->type;
    e->var = sym->var;
    e->sym = sym;
    return e;
}

struct parser_expr *parser_new_field_expr(struct Field *f)
{
    struct parser_expr *e = new_expr(NOD_EXPR_FIELD);
    e->type = f->type;
    e->field = f;
    return e;
}

struct parser_expr *parser_new_select_expr(struct parser_expr *inst, struct parser_expr *fld)
{
    struct parser_expr *e = new_expr(NOD_EXPR_SELECT);
    e->type = fld->type;
    e->l = inst;
    e->r = fld;
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

struct parser_expr *parser_new_call_expr(struct parser_expr *callee, struct parser_pos p)
{
    struct parser_expr *e = new_expr(NOD_EXPR_CALL);
    e->type = callee->type->func_type->return_type;
    e->l = callee;
    e->pos = p;
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

struct parser_expr *parser_new_addr_expr(struct parser_expr *l)
{
    struct parser_expr *e = new_expr(NOD_EXPR_ADDRESS);
    e->type = NewPtrType(e->type);
    e->l = l;
    return e;
}

struct parser_expr *parser_new_deref_expr(struct parser_expr *l)
{
    struct parser_expr *e = new_expr(NOD_EXPR_DEREF);
    e->type = DuplicateType(l->type->underlying);
    e->l = l;
    return e;
}

static struct parser_expr *new_binary_expr(struct parser_expr *l, struct parser_expr *r, int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;
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
    e->type = NewBoolType();
    e->l = l;
    e->r = r;
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

struct parser_stmt *parser_new_for_stmt(struct parser_stmt *init, struct parser_expr *cond,
        struct parser_stmt *post, struct parser_stmt *body)
{
    struct parser_stmt *s = new_stmt(NOD_STMT_FOR);
    s->init = init;
    s->cond = cond;
    s->post = post;
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

static struct parser_stmt *new_assign_stmt(struct parser_expr *l, struct parser_expr *r, int kind)
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

static struct parser_stmt *new_incdec_stmt(struct parser_expr *l, int kind)
{
    struct parser_expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;

    struct parser_stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = e;

    return s;
}

struct parser_stmt *parser_new_inc_stmt(struct parser_expr *l)
{
    return new_incdec_stmt(l, NOD_EXPR_INC);
}

struct parser_stmt *parser_new_dec_stmt(struct parser_expr *l)
{
    return new_incdec_stmt(l, NOD_EXPR_DEC);
}
