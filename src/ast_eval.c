#include "ast_eval.h"
#include "scope.h"
#include "type.h"

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
