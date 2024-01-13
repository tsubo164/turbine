#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

// TODO move this to str.c
int ConvertEscSeq(const char *s, std::string &converted)
{
    return ConvertEscapeSequence(s, converted);
}


//--------------------------------
// Expr
Expr *NewNullExpr(void)
{
    Expr *e = CALLOC(Expr);
    e->type = NewNilType();
    e->kind = T_NUL;
    return e;
}

Expr *NewNilLitExpr(void)
{
    Expr *e = CALLOC(Expr);
    e->type = NewNilType();
    e->kind = T_NILLIT;
    return e;
}

Expr *NewBoolLitExpr(bool b)
{
    Expr *e = CALLOC(Expr);
    e->type = NewBoolType();
    e->kind = T_BOLLIT;
    e->val.i = b;
    return e;
}

Expr *NewIntLitExpr(long l)
{
    Expr *e = CALLOC(Expr);
    e->type = NewIntType();
    e->kind = T_INTLIT;
    e->val.i = l;
    return e;
}

Expr *NewFloatLitExpr(double d)
{
    Expr *e = CALLOC(Expr);
    e->type = NewFloatType();
    e->kind = T_FLTLIT;
    e->val.f = d;
    return e;
}

Expr *NewStringLitExpr(const char *s)
{
    Expr *e = CALLOC(Expr);
    e->type = NewStringType();
    e->kind = T_STRLIT;
    e->val.s = s;
    return e;
}

Expr *NewConversionExpr(Expr *from, Type *to)
{
    Expr *e = CALLOC(Expr);
    e->type = to;
    e->kind = T_CONV;
    e->l = from;
    return e;
}

Expr *NewIdentExpr(Var *v)
{
    Expr *e = CALLOC(Expr);
    e->type = v->type;
    e->kind = T_IDENT;
    e->var = v;
    return e;
}

Expr *NewFieldExpr(Field *f)
{
    Expr *e = CALLOC(Expr);
    e->type = f->type;
    e->kind = T_FIELD;
    e->field = f;
    return e;
}

Expr *NewSelectExpr(Expr *inst, Expr *fld)
{
    Expr *e = CALLOC(Expr);
    e->type = fld->type;
    e->kind = T_SELECT;
    e->l = inst;
    e->r = fld;
    return e;
}

Expr *NewIndexExpr(Expr *ary, Expr *idx)
{
    Expr *e = CALLOC(Expr);
    e->type = ary->type->underlying;
    e->kind = T_INDEX;
    e->l = ary;
    e->r = idx;
    return e;
}

Expr *NewCallExpr(Expr *callee, Pos p)
{
    Expr *e = CALLOC(Expr);
    e->type = callee->type->func->return_type;
    e->kind = T_CALL;
    e->l = callee;
    e->pos = p;
    return e;
}

Expr *NewBinaryExpr(Expr *L, Expr *R, int k)
{
    Expr *e = CALLOC(Expr);
    e->type = L->type;
    switch (k) {
    case T_ADD: e->kind = T_ADD; break;
    case T_SUB: e->kind = T_SUB; break;
    case T_MUL: e->kind = T_MUL; break;
    case T_DIV: e->kind = T_DIV; break;
    case T_REM: e->kind = T_REM; break;
    case T_OR:  e->kind = T_OR;  break;
    case T_LOR: e->kind = T_LOR; break;
    case T_AND: e->kind = T_AND; break;
    case T_LAND:e->kind = T_LAND; break;
    case T_LNOT:e->kind = T_LNOT; break;
    case T_XOR: e->kind = T_XOR; break;
    case T_NOT: e->kind = T_NOT; break;
    case T_SHL: e->kind = T_SHL; break;
    case T_SHR: e->kind = T_SHR; break;
    default:    e->kind = T_NUL; break;
    }
    e->l = L;
    e->r = R;
    return e;
}

Expr *NewRelationalExpr(Expr *L, Expr *R, int k)
{
    Expr *e = CALLOC(Expr);
    e->type = NewBoolType();
    switch (k) {
    case T_EQ:   e->kind = T_EQ;  break;
    case T_NEQ:  e->kind = T_NEQ; break;
    case T_LT:   e->kind = T_LT; break;
    case T_GT:   e->kind = T_GT; break;
    case T_LTE:  e->kind = T_LTE; break;
    case T_GTE:  e->kind = T_GTE; break;
    default:     e->kind = T_NUL; break;
    }
    e->l = L;
    e->r = R;
    return e;
}

Expr *NewUnaryExpr(Expr *L, Type *t, int k)
{
    Expr *e = CALLOC(Expr);
    e->type = t;
    switch (k) {
    case T_AND:  e->kind = T_ADR; break;
    case T_ADD:  e->kind = T_POS; break;
    case T_SUB:  e->kind = T_NEG; break;
    case T_LNOT: e->kind = T_LNOT; break;
    case T_NOT:  e->kind = T_NOT; break;
    case T_MUL:  e->kind = T_DRF; break;
    default:     e->kind = T_NUL; break;
    }
    e->l = L;
    return e;
}

Expr *NewAssignExpr(Expr *l, Expr *r, int k)
{
    Expr *e = CALLOC(Expr);
    e->type = l->type;
    switch (k) {
    case T_ASSN:  e->kind = T_ASSN; break;
    case T_AADD:  e->kind = T_AADD; break;
    case T_ASUB:  e->kind = T_ASUB; break;
    case T_AMUL:  e->kind = T_AMUL; break;
    case T_ADIV:  e->kind = T_ADIV; break;
    case T_AREM:  e->kind = T_AREM; break;
    default:           e->kind = T_NUL; break;
    }
    e->l = l;
    e->r = r;
    return e;
}

Expr *NewIncDecExpr(Expr *l, int k)
{
    Expr *e = CALLOC(Expr);
    e->type = l->type;
    switch (k) {
    case T_INC:  e->kind = T_INC; break;
    case T_DEC: e->kind = T_DEC; break;
    default:        e->kind = T_NUL; break;
    }
    e->l = l;
    return e;
}


//--------------------------------
// Stmt
Stmt *NewNopStmt(void)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_NOP;
    return s;
}

Stmt *NewBlockStmt(Stmt *children)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_BLOCK;
    s->children = children;
    return s;
}

Stmt *NewOrStmt(Expr *cond, Stmt *body)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_ELS;
    s->cond = cond;
    s->body = body;
    return s;
}

Stmt *NewIfStmt(Stmt *or_list)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_IF;
    s->children = or_list;
    return s;
}

Stmt *NewForStmt(Expr *init, Expr *cond, Expr *post, Stmt *body)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_FOR;
    s->expr = init;
    s->cond = cond;
    s->post = post;
    s->body = body;
    return s;
}

Stmt *NewJumpStmt(int k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case T_BRK: s->kind = T_BRK; break;
    case T_CNT: s->kind = T_CNT; break;
    default:    s->kind = T_NUL; break;
    }
    return s;
}

Stmt *NewCaseStmt(Stmt *conds, Stmt *body, int k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case T_CASE: s->kind = T_CASE; break;
    case T_DFLT: s->kind = T_DFLT; break;
    default:     s->kind = T_NUL; break;
    }
    s->children = conds;
    s->body = body;
    return s;
}

Stmt *NewSwitchStmt(Expr *cond, Stmt *cases)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_SWT;
    s->cond = cond;
    s->children = cases;
    return s;
}

Stmt *NewReturnStmt(Expr *e)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_RET;
    s->expr = e;
    return s;
}

Stmt *NewExprStmt(Expr *e)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_EXPR;
    s->expr = e;
    return s;
}


//--------------------------------
// FuncDef
FuncDef *NewFuncDef(Var *v, Stmt *body)
{
    FuncDef *f = CALLOC(FuncDef);
    f->var = v;
    f->body = body;
    f->func = v->type->func;
    f->funclit_id = 0;
    f->next = NULL;
    return f;
}

Prog *NewProg(Scope *sc)
{
    Prog *p = CALLOC(Prog);
    p->scope = sc;
    p->funcs = NULL;
    p->gvars = NULL;
    p->main_func = NULL;
    return p;
}

bool IsNull(const Expr *e)
{
    return e->kind == T_NUL;
}

bool IsGlobal(const Expr *e)
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

int Addr(const Expr *e)
{
    switch (e->kind) {
    case T_IDENT:
        return e->var->id;

    case T_FIELD:
        return e->field->id;

    case T_SELECT:
        return Addr(e->l) + Addr(e->r);

    default:
        return -1;
    }
}

static bool eval_binary(const Expr *e, long *result)
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

static bool eval_unary(const Expr *e, long *result)
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

bool EvalExpr(const Expr *e, long *result)
{
    switch (e->kind) {
    case T_INTLIT:
        *result = e->val.i;
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

bool EvalAddr(const Expr *e, int *result)
{
    switch (e->kind) {
    case T_IDENT:
        *result = e->var->id;
        return true;

    case T_FIELD:
        *result = e->field->id;
        return true;

    default:
        return false;
    }
}

void print_expr(const Expr *e, int depth)
{
    const TokInfo *info;
    int i;

    if (!e || e->kind == T_NUL)
        return;

    // indentation
    for (i = 0; i < depth; i++) {
        printf("  ");
    }

    // basic info
    info = find_tokinfo(e->kind);
    printf("%d. <%s>", depth, info->str);

    // extra value
    switch (info->type) {
    case 'i':
        printf(" (%ld)", e->val.i);
        break;
    case 'f':
        printf(" (%g)", e->val.f);
        break;
    case 's':
        printf(" (%s)", e->val.s);
        break;
    case 'v':
        printf(" (%s)", e->var->name);
        break;
    }
    printf("\n");

    // children
    if (e->l)
        print_expr(e->l, depth + 1);
    if (e->r)
        print_expr(e->r, depth + 1);
}

void PrintStmt(const Stmt *s, int depth)
{
    const TokInfo *info;
    int i;

    if (!s)
        return;

    // indentation
    for (i = 0; i < depth; i++)
        printf("  ");

    // basic info
    info = find_tokinfo(s->kind);
    printf("%d. <%s>", depth, info->str);
    printf("\n");

    // children
    for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
        PrintStmt(stmt, depth + 1);

    print_expr(s->expr, depth + 1);
    print_expr(s->cond, depth + 1);
    print_expr(s->post, depth + 1);
    PrintStmt(s->body, depth + 1);
}

void print_funcdef(const FuncDef *f, int depth)
{
    if (!f)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <func_def> \"%s\"", depth, std::string(f->var->name).c_str());
    printf(" %s", TypeString(f->var->type->func->return_type).c_str());
    printf("\n");

    // children
    PrintStmt(f->body, depth + 1);
}

void PrintProg(const Prog *p, int depth)
{
    if (!p)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <prog>", depth);
    printf("\n");

    // children
    for (const Stmt *gvar = p->gvars; gvar; gvar = gvar->next)
        PrintStmt(gvar, depth + 1);

    for (const FuncDef *func = p->funcs; func; func = func->next)
        print_funcdef(func, depth + 1);
}
