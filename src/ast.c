#include "ast.h"
#include "scope.h"
#include "type.h"
#include "mem.h"

#include <assert.h>

static int token_to_node(int token_kind)
{
    static const int table[] = {
        /* stmt */
        [T_NOP]                     = NOD_STMT_NOP,
        [T_IF]                      = NOD_STMT_IF,
        [T_FOR]                     = NOD_STMT_FOR,
        [T_ELS]                     = NOD_STMT_ELSE,
        [T_BRK]                     = NOD_STMT_BREAK,
        [T_CNT]                     = NOD_STMT_CONTINUE,
        [T_SWT]                     = NOD_STMT_SWITCH,
        [T_CASE]                    = NOD_STMT_CASE,
        [T_DFLT]                    = NOD_STMT_DEFAULT,
        [T_RET]                     = NOD_STMT_RETURN,
        [T_EXPR]                    = NOD_STMT_EXPR,
        [T_BLOCK]                   = NOD_STMT_BLOCK,
        /*
        */
        /* identifier */
        [T_FIELD]                   = NOD_EXPR_FIELD,
        [T_IDENT]                   = NOD_EXPR_IDENT,
        /* literal */
        [T_NILLIT]                  = NOD_EXPR_NILLIT,
        [T_BOLLIT]                  = NOD_EXPR_BOOLLIT,
        [T_INTLIT]                  = NOD_EXPR_INTLIT,
        [T_FLTLIT]                  = NOD_EXPR_FLOATLIT,
        [T_STRLIT]                  = NOD_EXPR_STRINGLIT,
        [T_FUNCLIT]                 = NOD_EXPR_FUNCLIT,
        [T_ARRAYLIT]                = NOD_EXPR_ARRAYLIT,
        [T_STRUCTLIT]               = NOD_EXPR_STRUCTLIT,
        /* binary */
        [T_ADD]                     = NOD_EXPR_ADD,
        [T_SUB]                     = NOD_EXPR_SUB,
        [T_MUL]                     = NOD_EXPR_MUL,
        [T_DIV]                     = NOD_EXPR_DIV,
        [T_REM]                     = NOD_EXPR_REM,
        /* relational */
        [T_EQ]                      = NOD_EXPR_EQ,
        [T_NEQ]                     = NOD_EXPR_NEQ,
        [T_LT]                      = NOD_EXPR_LT,
        [T_LTE]                     = NOD_EXPR_LTE,
        [T_GT]                      = NOD_EXPR_GT,
        [T_GTE]                     = NOD_EXPR_GTE,
        /* bitwise */
        [T_SHL]                     = NOD_EXPR_SHL,
        [T_SHR]                     = NOD_EXPR_SHR,
        [T_OR]                      = NOD_EXPR_OR,
        [T_XOR]                     = NOD_EXPR_XOR,
        [T_AND]                     = NOD_EXPR_AND,
        /* logical */
        [T_LOR]                     = NOD_EXPR_LOGOR,
        [T_LAND]                    = NOD_EXPR_LOGAND,
        [T_LNOT]                    = NOD_EXPR_LOGNOT,
        /* unary */
        [T_POS]                     = NOD_EXPR_POS,
        [T_NEG]                     = NOD_EXPR_NEG,
        [T_ADR]                     = NOD_EXPR_ADDRESS,
        [T_DRF]                     = NOD_EXPR_DEREF,
        [T_NOT]                     = NOD_EXPR_NOT,
        [T_INC]                     = NOD_EXPR_INC,
        [T_DEC]                     = NOD_EXPR_DEC,
        [T_CONV]                    = NOD_EXPR_CONV,
        /* array] struct, func */
        [T_SELECT]                  = NOD_EXPR_SELECT,
        [T_INDEX]                   = NOD_EXPR_INDEX,
        [T_CALL]                    = NOD_EXPR_CALL,
        /* assign */
        [T_ASSN]                    = NOD_EXPR_ASSIGN,
        [T_AADD]                    = NOD_EXPR_ADDASSIGN,
        [T_ASUB]                    = NOD_EXPR_SUBASSIGN,
        [T_AMUL]                    = NOD_EXPR_MULASSIGN,
        [T_ADIV]                    = NOD_EXPR_DIVASSIGN,
        [T_AREM]                    = NOD_EXPR_REMASSIGN,
        [T_INIT]                    = NOD_EXPR_INIT,
        /*
        NOD_EXPR_ASSIGN             = NOD_STMT_ASSIGN,
        NOD_EXPR_INIT               = NOD_STMT_INIT,
        */
        [T_ELEMENT]                 = NOD_EXPR_ELEMENT,
    };

    int TABLE_SIZE = sizeof(table) / sizeof(table[0]);
    if (token_kind >= TABLE_SIZE) {
        assert(!"token_kind >= TABLE_SIZE failed");
    }

    return table[token_kind];
}

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

// Expr
struct Expr *NewNilLitExpr(void)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewNilType();
    e->kind = NOD_EXPR_NILLIT;
    return e;
}

struct Expr *NewBoolLitExpr(bool b)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewBoolType();
    e->kind = NOD_EXPR_BOOLLIT;
    e->ival = b;
    return e;
}

struct Expr *NewIntLitExpr(long l)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewIntType();
    e->kind = NOD_EXPR_INTLIT;
    e->ival = l;
    return e;
}

struct Expr *NewFloatLitExpr(double d)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewFloatType();
    e->kind = NOD_EXPR_FLOATLIT;
    e->fval = d;
    return e;
}

struct Expr *NewStringLitExpr(const char *s)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewStringType();
    e->kind = NOD_EXPR_STRINGLIT;
    e->sval = s;
    return e;
}

struct Expr *NewFuncLitExpr(struct Func *func)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewFuncType(func->func_type);
    e->kind = NOD_EXPR_FUNCLIT;
    e->func = func;
    return e;
}

struct Expr *NewArrayLitExpr(struct Expr *elems, int len)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewArrayType(len, elems->type);
    e->kind = NOD_EXPR_ARRAYLIT;
    e->l = elems;
    return e;
}

struct Expr *NewStructLitExpr(struct Struct *strct, struct Expr *fields)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewStructType(strct);
    e->kind = NOD_EXPR_STRUCTLIT;
    e->l = fields;
    return e;
}

struct Expr *NewConversionExpr(struct Expr *from, struct Type *to)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = to;
    e->kind = NOD_EXPR_CONV;
    e->l = from;
    return e;
}

struct Expr *NewIdentExpr(struct Symbol *sym)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = sym->type;
    e->kind = NOD_EXPR_IDENT;
    e->var = sym->var;
    e->sym = sym;
    return e;
}

struct Expr *NewFieldExpr(struct Field *f)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = f->type;
    e->kind = NOD_EXPR_FIELD;
    e->field = f;
    return e;
}

struct Expr *NewSelectExpr(struct Expr *inst, struct Expr *fld)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = fld->type;
    e->kind = NOD_EXPR_SELECT;
    e->l = inst;
    e->r = fld;
    return e;
}

struct Expr *NewIndexExpr(struct Expr *ary, struct Expr *idx)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = ary->type->underlying;
    e->kind = NOD_EXPR_INDEX;
    e->l = ary;
    e->r = idx;
    return e;
}

struct Expr *NewCallExpr(struct Expr *callee, struct Pos p)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = callee->type->func_type->return_type;
    e->kind = NOD_EXPR_CALL;
    e->l = callee;
    e->pos = p;
    return e;
}

struct Expr *NewBinaryExpr(struct Expr *L, struct Expr *R, int kind)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = L->type;
    /*
    switch (kind) {
    case T_ADD: case T_SUB: case T_MUL: case T_DIV: case T_REM:
    case T_LOR: case T_LAND: case T_LNOT:
    case T_AND: case T_OR: case T_XOR: case T_NOT:
    case T_SHL: case T_SHR:
        e->kind = kind;
        break;
    default:
        // error
        break;
    }
    */
    e->kind = token_to_node(kind);
    e->l = L;
    e->r = R;
    return e;
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

struct Expr *NewElementExpr(struct Expr *key, struct Expr *val)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = val->type;
    e->kind = NOD_EXPR_ELEMENT;
    e->l = key;
    e->r = val;
    return e;
}

struct Expr *parser_new_posi_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_POS);
    e->type = l->type;
    e->l = l;
    return e;
}

struct Expr *parser_new_nega_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_NEG);
    e->type = l->type;
    e->l = l;
    return e;
}

struct Expr *parser_new_lognot_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_LOGNOT);
    e->type = l->type;
    e->l = l;
    return e;
}

struct Expr *parser_new_not_expr(struct Expr *l)
{
    struct Expr *e = new_expr(NOD_EXPR_NOT);
    e->type = l->type;
    e->l = l;
    return e;
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

static struct Expr *new_init_expr(struct Expr *l, struct Expr *r)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = l->type;
    e->kind = NOD_EXPR_INIT;
    e->l = l;
    e->r = r;
    return e;
}

// Stmt
struct Stmt *NewNopStmt(void)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_NOP;
    return s;
}

struct Stmt *NewBlockStmt(struct Stmt *children)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_BLOCK;
    s->children = children;
    return s;
}

struct Stmt *NewOrStmt(struct Expr *cond, struct Stmt *body)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_ELSE;
    s->cond = cond;
    s->body = body;
    return s;
}

struct Stmt *NewIfStmt(struct Stmt *or_list)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_IF;
    s->children = or_list;
    return s;
}

struct Stmt *NewForStmt(struct Stmt *init, struct Expr *cond, struct Stmt *post,
        struct Stmt *body)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_FOR;
    s->init = init;
    s->cond = cond;
    s->post = post;
    s->body = body;
    return s;
}

struct Stmt *NewJumpStmt(int kind)
{
    struct Stmt *s = new_stmt(kind);
    return s;
}

struct Stmt *NewCaseStmt(struct Expr *conds, struct Stmt *body, int kind)
{
    struct Stmt *s = new_stmt(kind);
    s->cond = conds;
    s->body = body;
    return s;
}

struct Stmt *NewSwitchStmt(struct Expr *cond, struct Stmt *cases)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_SWITCH;
    s->cond = cond;
    s->children = cases;
    return s;
}

struct Stmt *NewReturnStmt(struct Expr *e)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_RETURN;
    s->expr = e;
    return s;
}

struct Stmt *NewExprStmt(struct Expr *e)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_EXPR;
    s->expr = e;
    return s;
}

static struct Expr *new_assign_expr(struct Expr *l, struct Expr *r, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    e->r = r;
    return e;
}

struct Stmt *parser_new_assign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_ASSIGN);
    return s;
}

struct Stmt *parser_new_addassign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_ADDASSIGN);
    return s;
}

struct Stmt *parser_new_subassign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_SUBASSIGN);
    return s;
}

struct Stmt *parser_new_mulassign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_MULASSIGN);
    return s;
}

struct Stmt *parser_new_divassign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_DIVASSIGN);
    return s;
}

struct Stmt *parser_new_remassign_stmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_assign_expr(l, r, NOD_EXPR_REMASSIGN);
    return s;
}

struct Stmt *NewInitStmt(struct Expr *l, struct Expr *r)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = NOD_STMT_INIT;
    s->expr = new_init_expr(l, r);
    return s;
}

static struct Expr *new_incdec_expr(struct Expr *l, int kind)
{
    struct Expr *e = new_expr(kind);
    e->type = l->type;
    e->l = l;
    return e;
}

struct Stmt *parser_new_inc_stmt(struct Expr *l)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_incdec_expr(l, NOD_EXPR_INC);
    return s;
}

struct Stmt *parser_new_dec_stmt(struct Expr *l)
{
    struct Stmt *s = new_stmt(NOD_STMT_ASSIGN);
    s->expr = new_incdec_expr(l, NOD_EXPR_DEC);
    return s;
}

bool IsGlobal(const struct Expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_IDENT:
        return e->var->is_global;

    case NOD_EXPR_SELECT:
        return IsGlobal(e->l);

    default:
        return false;
    }
}

bool IsMutable(const struct Expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_IDENT:
        if (e->var->is_param == true && IsPtr(e->type))
            return true;
        return e->var->is_param == false;

    case NOD_EXPR_SELECT:
        return IsMutable(e->l);

    default:
        return true;
    }
}

const struct Var *FindRootObject(const struct Expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_IDENT:
        return e->var;

    case NOD_EXPR_SELECT:
        return FindRootObject(e->l);

    default:
        return NULL;
    }
}

int Addr(const struct Expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_IDENT:
        return e->var->offset;

    case NOD_EXPR_FIELD:
        return e->field->offset;

    case NOD_EXPR_SELECT:
        return Addr(e->l) + Addr(e->r);

    default:
        return -1;
    }
}

static bool eval_binary(const struct Expr *e, int64_t *result)
{
    int64_t L = 0, R = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    if (!EvalExpr(e->r, &R))
        return false;

    switch (e->kind) {
    case NOD_EXPR_ADD: *result = L + R; return true;
    case NOD_EXPR_SUB: *result = L - R; return true;
    case NOD_EXPR_MUL: *result = L * R; return true;
    case NOD_EXPR_DIV: *result = L / R; return true;
    case NOD_EXPR_REM: *result = L % R; return true;
    default: return false;
    }
}

static bool eval_unary(const struct Expr *e, int64_t *result)
{
    int64_t L = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    switch (e->kind) {
    case NOD_EXPR_POS:    *result = +L; return true;
    case NOD_EXPR_NEG:    *result = -L; return true;
    case NOD_EXPR_LOGNOT: *result = !L; return true;
    case NOD_EXPR_NOT:    *result = ~L; return true;
    default: return false;
    }
}

bool EvalExpr(const struct Expr *e, int64_t *result)
{
    switch (e->kind) {

    case NOD_EXPR_INTLIT:
        *result = e->ival;
        return true;

    case NOD_EXPR_FUNCLIT:
        *result = e->func->id;
        return true;

    case NOD_EXPR_ADD: case NOD_EXPR_SUB:
    case NOD_EXPR_MUL: case NOD_EXPR_DIV: case NOD_EXPR_REM:
        return eval_binary(e, result);

    case NOD_EXPR_POS: case NOD_EXPR_NEG:
    case NOD_EXPR_LOGNOT: case NOD_EXPR_NOT:
        return eval_unary(e, result);

    default:
        return false;
    }
}

bool EvalAddr(const struct Expr *e, int *result)
{
    switch (e->kind) {

    case NOD_EXPR_IDENT:
        *result = e->var->offset;
        return true;

    case NOD_EXPR_FIELD:
        *result = e->field->offset;
        return true;

    default:
        return false;
    }
}
