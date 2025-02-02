#include "parser_ast_eval.h"
#include "parser_symbol.h"
#include "parser_type.h"

bool parser_ast_is_global(const struct parser_expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_VAR:
        return e->var->is_global;

    case NOD_EXPR_STRUCTACCESS:
        return parser_ast_is_global(e->l);

    default:
        return false;
    }
}

bool parser_ast_is_mutable(const struct parser_expr *e)
{
    switch (e->kind) {
    case NOD_EXPR_VAR:
        return e->var->is_param == false;

    default:
        return true;
    }
}

static bool eval_binary(const struct parser_expr *e, int64_t *result)
{
    int64_t L = 0, R = 0;

    if (!parser_eval_expr(e->l, &L))
        return false;

    if (!parser_eval_expr(e->r, &R))
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

static bool eval_unary(const struct parser_expr *e, int64_t *result)
{
    int64_t L = 0;

    if (!parser_eval_expr(e->l, &L))
        return false;

    switch (e->kind) {
    case NOD_EXPR_POS:    *result = +L; return true;
    case NOD_EXPR_NEG:    *result = -L; return true;
    case NOD_EXPR_LOGNOT: *result = !L; return true;
    case NOD_EXPR_NOT:    *result = ~L; return true;
    default: return false;
    }
}

bool parser_eval_expr(const struct parser_expr *e, int64_t *result)
{
    switch (e->kind) {

    case NOD_EXPR_MODULEACCESS:
        return parser_eval_expr(e->r, result);

    case NOD_EXPR_BOOLLIT:
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

/* TODO remove this */
bool parser_eval_addr(const struct parser_expr *e, int *result)
{
    switch (e->kind) {

    case NOD_EXPR_VAR:
        *result = e->var->offset;
        return true;

    case NOD_EXPR_STRUCTFIELD:
        *result = e->struct_field->offset;
        return true;

    default:
        return false;
    }
}
