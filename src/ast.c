#include "ast.h"
#include "scope.h"
#include "type.h"
#include "mem.h"

// Expr
struct Expr *NewNilLitExpr(void)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewNilType();
    e->kind = T_NILLIT;
    return e;
}

struct Expr *NewBoolLitExpr(bool b)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewBoolType();
    e->kind = T_BOLLIT;
    e->ival = b;
    return e;
}

struct Expr *NewIntLitExpr(long l)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewIntType();
    e->kind = T_INTLIT;
    e->ival = l;
    return e;
}

struct Expr *NewFloatLitExpr(double d)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewFloatType();
    e->kind = T_FLTLIT;
    e->fval = d;
    return e;
}

struct Expr *NewStringLitExpr(const char *s)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewStringType();
    e->kind = T_STRLIT;
    e->sval = s;
    return e;
}

struct Expr *NewFuncLitExpr(struct Func *func)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewStringType();
    e->kind = T_FUNCLIT;
    e->func = func;
    return e;
}

struct Expr *NewConversionExpr(struct Expr *from, Type *to)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = to;
    e->kind = T_CONV;
    e->l = from;
    return e;
}

struct Expr *NewIdentExpr(struct Symbol *sym)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = sym->type;
    e->kind = T_IDENT;
    e->var = sym->var;
    e->sym = sym;
    return e;
}

struct Expr *NewFieldExpr(struct Field *f)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = f->type;
    e->kind = T_FIELD;
    e->field = f;
    return e;
}

struct Expr *NewSelectExpr(struct Expr *inst, struct Expr *fld)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = fld->type;
    e->kind = T_SELECT;
    e->l = inst;
    e->r = fld;
    return e;
}

struct Expr *NewIndexExpr(struct Expr *ary, struct Expr *idx)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = ary->type->underlying;
    e->kind = T_INDEX;
    e->l = ary;
    e->r = idx;
    return e;
}

struct Expr *NewCallExpr(struct Expr *callee, struct Pos p)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = callee->type->func->return_type;
    e->kind = T_CALL;
    e->l = callee;
    e->pos = p;
    return e;
}

struct Expr *NewBinaryExpr(struct Expr *L, struct Expr *R, int k)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = L->type;
    switch (k) {
    case T_ADD: case T_SUB: case T_MUL: case T_DIV: case T_REM:
    case T_LOR: case T_LAND: case T_LNOT:
    case T_AND: case T_OR: case T_XOR: case T_NOT:
    case T_SHL: case T_SHR:
        e->kind = k;
        break;
    default:
        // error
        break;
    }
    e->l = L;
    e->r = R;
    return e;
}

struct Expr *NewRelationalExpr(struct Expr *L, struct Expr *R, int k)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = NewBoolType();
    switch (k) {
    case T_EQ: case T_NEQ:
    case T_LT: case T_LTE:
    case T_GT: case T_GTE:
        e->kind = k;
        break;
    default:
        // error
        break;
    }
    e->l = L;
    e->r = R;
    return e;
}

struct Expr *NewUnaryExpr(struct Expr *L, Type *t, int k)
{
    struct Expr *e = CALLOC(struct Expr);
    switch (k) {
    case T_AND:  e->kind = T_ADR; break;
    case T_ADD:  e->kind = T_POS; break;
    case T_SUB:  e->kind = T_NEG; break;
    case T_MUL:  e->kind = T_DRF; break;
    case T_LNOT: case T_NOT: e->kind = k; break;
    default:
        // error
        break;
    }
    e->type = t;
    e->l = L;
    return e;
}

static struct Expr *new_assign_expr(struct Expr *l, struct Expr *r, int k)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = l->type;
    switch (k) {
    case T_ASSN: case T_AADD: case T_ASUB:
    case T_AMUL: case T_ADIV: case T_AREM:
        e->kind = k;
        break;
    default:
        // error
        break;
    }
    e->l = l;
    e->r = r;
    return e;
}

static struct Expr *new_incdec_expr(struct Expr *l, int k)
{
    struct Expr *e = CALLOC(struct Expr);
    e->type = l->type;
    switch (k) {
    case T_INC: case T_DEC:
        e->kind = k;
    default:
        // error
        break;
    }
    e->l = l;
    return e;
}

// Stmt
struct Stmt *NewNopStmt(void)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_NOP;
    return s;
}

struct Stmt *NewBlockStmt(struct Stmt *children)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_BLOCK;
    s->children = children;
    return s;
}

struct Stmt *NewOrStmt(struct Expr *cond, struct Stmt *body)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_ELS;
    s->cond = cond;
    s->body = body;
    return s;
}

struct Stmt *NewIfStmt(struct Stmt *or_list)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_IF;
    s->children = or_list;
    return s;
}

struct Stmt *NewForStmt(struct Stmt *init, struct Expr *cond, struct Stmt *post,
        struct Stmt *body)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_FOR;
    s->init = init;
    s->cond = cond;
    s->post = post;
    s->body = body;
    return s;
}

struct Stmt *NewJumpStmt(int k)
{
    struct Stmt *s = CALLOC(struct Stmt);
    switch (k) {
    case T_BRK: case T_CNT:
        s->kind = k;
        break;
    default:
        // error
        break;
    }
    return s;
}

struct Stmt *NewCaseStmt(struct Stmt *conds, struct Stmt *body, int k)
{
    struct Stmt *s = CALLOC(struct Stmt);
    switch (k) {
    case T_CASE: case T_DFLT:
        s->kind = k;
        break;
    default:
        // error
        break;
    }
    s->children = conds;
    s->body = body;
    return s;
}

struct Stmt *NewSwitchStmt(struct Expr *cond, struct Stmt *cases)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_SWT;
    s->cond = cond;
    s->children = cases;
    return s;
}

struct Stmt *NewReturnStmt(struct Expr *e)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_RET;
    s->expr = e;
    return s;
}

struct Stmt *NewExprStmt(struct Expr *e)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_EXPR;
    s->expr = e;
    return s;
}

struct Stmt *NewAssignStmt(struct Expr *l, struct Expr *r, int kind)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_ASSN;
    s->expr = new_assign_expr(l, r, kind);
    return s;
}

struct Stmt *NewIncDecStmt(struct Expr *l, int kind)
{
    struct Stmt *s = CALLOC(struct Stmt);
    s->kind = T_ASSN;
    s->expr = new_incdec_expr(l, kind);
    return s;
}

bool IsNull(const struct Expr *e)
{
    return e->kind == T_NUL;
}

bool IsGlobal(const struct Expr *e)
{
    switch (e->kind) {
    case T_IDENT:
        return e->var->is_global;

    case T_SELECT:
        return IsGlobal(e->l);

    default:
        return false;
    }
}

int Addr(const struct Expr *e)
{
    switch (e->kind) {
    case T_IDENT:
        return e->var->offset;

    case T_FIELD:
        return e->field->offset;

    case T_SELECT:
        return Addr(e->l) + Addr(e->r);

    default:
        return -1;
    }
}

static bool eval_binary(const struct Expr *e, long *result)
{
    long L = 0, R = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    if (!EvalExpr(e->r, &R))
        return false;

    switch (e->kind) {
    case T_ADD: *result = L + R; return true;
    case T_SUB: *result = L - R; return true;
    case T_MUL: *result = L * R; return true;
    case T_DIV: *result = L / R; return true;
    case T_REM: *result = L % R; return true;
    default: return false;
    }
}

static bool eval_unary(const struct Expr *e, long *result)
{
    long L = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    switch (e->kind) {
    case T_POS:  *result = +L; return true;
    case T_NEG:  *result = -L; return true;
    case T_LNOT: *result = !L; return true;
    case T_NOT:  *result = ~L; return true;
    default: return false;
    }
}

bool EvalExpr(const struct Expr *e, long *result)
{
    switch (e->kind) {
    case T_INTLIT:
        *result = e->ival;
        return true;

    case T_ADD: case T_SUB:
    case T_MUL: case T_DIV: case T_REM:
        return eval_binary(e, result);

    case T_POS: case T_NEG:
    case T_LNOT: case T_NOT:
        return eval_unary(e, result);

    default:
        return false;
    }
}

bool EvalAddr(const struct Expr *e, int *result)
{
    switch (e->kind) {
    case T_IDENT:
        *result = e->var->offset;
        return true;

    case T_FIELD:
        *result = e->field->offset;
        return true;

    default:
        return false;
    }
}
