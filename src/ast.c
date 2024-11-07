#include "ast.h"
#include "scope.h"
#include "type.h"

#include <stdlib.h>

static struct Expr *new_expr(int kind)
{
    struct Expr *e = calloc(1, sizeof(struct Expr));
    e->kind = kind;
    return e;
}

static struct Stmt *new_stmt(int kind)
{
    struct Stmt *s = calloc(1, sizeof(struct Stmt));
    s->kind = kind;
    return s;
}

/* expr */
struct Expr *parser_new_nillit_expr(void)
{
    struct Expr *e = new_expr(NOD_EXPR_NILLIT);
    e->type = NewNilType();
    return e;
}

struct Expr *parser_new_boollit_expr(bool b)
{
    struct Expr *e = new_expr(NOD_EXPR_BOOLLIT);
    e->type = NewBoolType();
    e->ival = b;
    return e;
}

struct Expr *parser_new_intlit_expr(long l)
{
    struct Expr *e = new_expr(NOD_EXPR_INTLIT);
    e->type = NewIntType();
    e->ival = l;
    return e;
}

struct Expr *parser_new_floatlit_expr(double d)
{
    struct Expr *e = new_expr(NOD_EXPR_FLOATLIT);
    e->type = NewFloatType();
    e->fval = d;
    return e;
}

struct Expr *parser_new_stringlit_expr(const char *s)
{
    struct Expr *e = new_expr(NOD_EXPR_STRINGLIT);
    e->type = NewStringType();
    e->sval = s;
    return e;
}

struct Expr *parser_new_funclit_expr(struct Func *func)
{
    struct Expr *e = new_expr(NOD_EXPR_FUNCLIT);
    e->type = NewFuncType(func->func_type);
    e->func = func;
    return e;
}

struct Expr *parser_new_arraylit_expr(struct Expr *elems, int len)
{
    struct Expr *e = new_expr(NOD_EXPR_ARRAYLIT);
    e->type = NewArrayType(len, elems->type);
    e->l = elems;
    return e;
}

struct Expr *parser_new_structlit_expr(struct Struct *strct, struct Expr *fields)
{
    struct Expr *e = new_expr(NOD_EXPR_STRUCTLIT);
    e->type = NewStructType(strct);
    e->l = fields;
    return e;
}

struct Expr *parser_new_conversion_expr(struct Expr *from, struct Type *to)
{
    struct Expr *e = new_expr(NOD_EXPR_CONV);
    e->type = to;
    e->l = from;
    return e;
}

struct Expr *parser_new_ident_expr(struct Symbol *sym)
{
    struct Expr *e = new_expr(NOD_EXPR_IDENT);
    e->type = sym->type;
    e->var = sym->var;
    e->sym = sym;
    return e;
}

struct Expr *parser_new_field_expr(struct Field *f)
{
    struct Expr *e = new_expr(NOD_EXPR_FIELD);
    e->type = f->type;
    e->field = f;
    return e;
}

struct Expr *parser_new_select_expr(struct Expr *inst, struct Expr *fld)
{
    struct Expr *e = new_expr(NOD_EXPR_SELECT);
    e->type = fld->type;
    e->l = inst;
    e->r = fld;
    return e;
}

struct Expr *parser_new_index_expr(struct Expr *ary, struct Expr *idx)
{
    struct Expr *e = new_expr(NOD_EXPR_INDEX);
    e->type = ary->type->underlying;
    e->l = ary;
    e->r = idx;
    return e;
}

struct Expr *parser_new_call_expr(struct Expr *callee, struct Pos p)
{
    struct Expr *e = new_expr(NOD_EXPR_CALL);
    e->type = callee->type->func_type->return_type;
    e->l = callee;
    e->pos = p;
    return e;
}

struct Expr *parser_new_element_expr(struct Expr *key, struct Expr *val)
{
    struct Expr *e = new_expr(NOD_EXPR_ELEMENT);
    e->type = val->type;
    e->l = key;
    e->r = val;
    return e;
}

static struct Expr *new_unary_expr(struct Expr *l, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    return e;
}

struct Expr *parser_new_posi_expr(struct Expr *l)
{
    return new_unary_expr(l, NOD_EXPR_POS);
}

struct Expr *parser_new_nega_expr(struct Expr *l)
{
    return new_unary_expr(l, NOD_EXPR_NEG);
}

struct Expr *parser_new_lognot_expr(struct Expr *l)
{
    return new_unary_expr(l, NOD_EXPR_LOGNOT);
}

struct Expr *parser_new_not_expr(struct Expr *l)
{
    return new_unary_expr(l, NOD_EXPR_NOT);
}

struct Expr *parser_new_addr_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_ADDRESS);
    e->type = NewPtrType(e->type);
    e->l = l;
    return e;
}

struct Expr *parser_new_deref_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_DEREF);
    e->type = DuplicateType(l->type->underlying);
    e->l = l;
    return e;
}

static struct Expr *new_binary_expr(struct Expr *l, struct Expr *r, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;
    return e;
}

struct Expr *parser_new_add_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_ADD);
}

struct Expr *parser_new_sub_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SUB);
}

struct Expr *parser_new_mul_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_MUL);
}

struct Expr *parser_new_div_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_DIV);
}

struct Expr *parser_new_rem_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_REM);
}

struct Expr *parser_new_shl_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SHL);
}

struct Expr *parser_new_shr_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_SHR);
}

struct Expr *parser_new_and_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_AND);
}

struct Expr *parser_new_or_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_OR);
}

struct Expr *parser_new_xor_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_XOR);
}

static struct Expr *new_rel_expr(struct Expr *l, struct Expr *r, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = NewBoolType();
    e->l = l;
    e->r = r;
    return e;
}

struct Expr *parser_new_eq_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_EQ);
}

struct Expr *parser_new_neq_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_NEQ);
}

struct Expr *parser_new_lt_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_LT);
}

struct Expr *parser_new_lte_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_LTE);
}

struct Expr *parser_new_gt_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_GT);
}

struct Expr *parser_new_gte_expr(struct Expr *l, struct Expr *r)
{
    return new_rel_expr(l, r, NOD_EXPR_GTE);
}

struct Expr *parser_new_logand_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_LOGAND);
}

struct Expr *parser_new_logor_expr(struct Expr *l, struct Expr *r)
{
    return new_binary_expr(l, r, NOD_EXPR_LOGOR);
}

/* stmt */
struct Stmt *parser_new_nop_stmt(void)
{
    return new_stmt(NOD_STMT_NOP);
}

struct Stmt *parser_new_block_stmt(struct Stmt *children)
{
    struct Stmt *s = new_stmt(NOD_STMT_BLOCK);
    s->children = children;
    return s;
}

struct Stmt *parser_new_if_stmt(struct Stmt *or_list)
{
    struct Stmt *s = new_stmt(NOD_STMT_IF);
    s->children = or_list;
    return s;
}

struct Stmt *parser_new_else_stmt(struct Expr *cond, struct Stmt *body)
{
    struct Stmt *s = new_stmt(NOD_STMT_ELSE);
    s->cond = cond;
    s->body = body;
    return s;
}

struct Stmt *parser_new_for_stmt(struct Stmt *init, struct Expr *cond,
        struct Stmt *post, struct Stmt *body)
{
    struct Stmt *s = new_stmt(NOD_STMT_FOR);
    s->init = init;
    s->cond = cond;
    s->post = post;
    s->body = body;
    return s;
}

struct Stmt *parser_new_break_stmt(void)
{
    return new_stmt(NOD_STMT_BREAK);
}

struct Stmt *parser_new_continue_stmt(void)
{
    return new_stmt(NOD_STMT_CONTINUE);
}

struct Stmt *parser_new_switch_stmt(struct Expr *cond, struct Stmt *cases)
{
    struct Stmt *s = new_stmt(NOD_STMT_SWITCH);
    s->cond = cond;
    s->children = cases;
    return s;
}

struct Stmt *parser_new_case_stmt(struct Expr *conds, struct Stmt *body)
{
    struct Stmt *s = new_stmt(NOD_STMT_CASE);
    s->cond = conds;
    s->body = body;
    return s;
}

struct Stmt *parser_new_default_stmt(struct Stmt *body)
{
    struct Stmt *s = new_stmt(NOD_STMT_DEFAULT);
    s->body = body;
    return s;
}

struct Stmt *parser_new_return_stmt(struct Expr *e)
{
    struct Stmt *s = new_stmt(NOD_STMT_RETURN);
    s->expr = e;
    return s;
}

struct Stmt *parser_new_expr_stmt(struct Expr *e)
{
    struct Stmt *s = new_stmt(NOD_STMT_EXPR);
    s->expr = e;
    return s;
}

struct Stmt *parser_new_init_stmt(struct Expr *l, struct Expr *r)
{
    struct Expr *e = new_expr(NOD_EXPR_INIT);
    e->type = l->type;
    e->l = l;
    e->r = r;

    struct Stmt *s = new_stmt(NOD_STMT_INIT);
    s->expr = e;

    return s;
}

static struct Stmt *new_assign_stmt(struct Expr *l, struct Expr *r, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;

    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = e;

    return s;
}

struct Stmt *parser_new_assign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ASSIGN);
}

struct Stmt *parser_new_addassign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_ADDASSIGN);
}

struct Stmt *parser_new_subassign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_SUBASSIGN);
}

struct Stmt *parser_new_mulassign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_MULASSIGN);
}

struct Stmt *parser_new_divassign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_DIVASSIGN);
}

struct Stmt *parser_new_remassign_stmt(struct Expr *l, struct Expr *r)
{
    return new_assign_stmt(l, r, NOD_EXPR_REMASSIGN);
}

static struct Stmt *new_incdec_stmt(struct Expr *l, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;

    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = e;

    return s;
}

struct Stmt *parser_new_inc_stmt(struct Expr *l)
{
    return new_incdec_stmt(l, NOD_EXPR_INC);
}

struct Stmt *parser_new_dec_stmt(struct Expr *l)
{
    return new_incdec_stmt(l, NOD_EXPR_DEC);
}
