#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

// TODO move this to str.c
int ConvertEscSeq(std::string_view s, std::string &converted)
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

Expr *NewStringLitExpr(std::string_view s)
{
    Expr *e = CALLOC(Expr);
    e->type = NewStringType();
    e->kind = T_STRLIT;
    e->val.sv = s;
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

Expr *NewBinaryExpr(Expr *L, Expr *R, TK k)
{
    Expr *e = CALLOC(Expr);
    e->type = L->type;
    switch (k) {
    case TK::PLUS:    e->kind = T_ADD; break;
    case TK::MINUS:   e->kind = T_SUB; break;
    case TK::STAR:    e->kind = T_MUL; break;
    case TK::SLASH:   e->kind = T_DIV; break;
    case TK::PERCENT: e->kind = T_REM; break;
    case TK::BAR:     e->kind = T_OR;  break;
    case TK::BAR2:    e->kind = T_LOR; break;
    case TK::AMP:     e->kind = T_AND; break;
    case TK::AMP2:    e->kind = T_LAND; break;
    case TK::EXCL:    e->kind = T_LNOT; break;
    case TK::CARET:   e->kind = T_XOR; break;
    case TK::TILDA:   e->kind = T_NOT; break;
    case TK::LT2:     e->kind = T_SHL; break;
    case TK::GT2:     e->kind = T_SHR; break;
    default:          e->kind = T_NUL; break;
    }
    e->l = L;
    e->r = R;
    return e;
}

Expr *NewRelationalExpr(Expr *L, Expr *R, TK k)
{
    Expr *e = CALLOC(Expr);
    e->type = NewBoolType();
    switch (k) {
    case TK::EQ2:     e->kind = T_EQ;  break;
    case TK::EXCLEQ:  e->kind = T_NEQ; break;
    case TK::LT:      e->kind = T_LT; break;
    case TK::GT:      e->kind = T_GT; break;
    case TK::LTE:     e->kind = T_LTE; break;
    case TK::GTE:     e->kind = T_GTE; break;
    default:          e->kind = T_NUL; break;
    }
    e->l = L;
    e->r = R;
    return e;
}

Expr *NewUnaryExpr(Expr *L, Type *t, TK k)
{
    Expr *e = CALLOC(Expr);
    e->type = t;
    switch (k) {
    case TK::AMP:    e->kind = T_ADR; break;
    case TK::PLUS:   e->kind = T_POS; break;
    case TK::MINUS:  e->kind = T_NEG; break;
    case TK::EXCL:   e->kind = T_LNOT; break;
    case TK::TILDA:  e->kind = T_NOT; break;
    case TK::STAR:   e->kind = T_DRF; break;
    default:         e->kind = T_NUL; break;
    }
    e->l = L;
    return e;
}

Expr *NewAssignExpr(Expr *l, Expr *r, TK k)
{
    Expr *e = CALLOC(Expr);
    e->type = l->type;
    switch (k) {
    case TK::EQ:        e->kind = T_ASSN; break;
    case TK::PLUSEQ:    e->kind = T_AADD; break;
    case TK::MINUSEQ:   e->kind = T_ASUB; break;
    case TK::STAREQ:    e->kind = T_AMUL; break;
    case TK::SLASHEQ:   e->kind = T_ADIV; break;
    case TK::PERCENTEQ: e->kind = T_AREM; break;
    default:            e->kind = T_NUL; break;
    }
    e->l = l;
    e->r = r;
    return e;
}

Expr *NewIncDecExpr(Expr *l, TK k)
{
    Expr *e = CALLOC(Expr);
    e->type = l->type;
    switch (k) {
    case TK::PLUS2:  e->kind = T_INC; break;
    case TK::MINUS2: e->kind = T_DEC; break;
    default:         e->kind = T_NUL; break;
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

Stmt *NewJumpStmt(TK k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case TK::BREAK:    s->kind = T_BRK; break;
    case TK::CONTINUE: s->kind = T_CNT; break;
    default:           s->kind = T_NUL; break;
    }
    return s;
}

Stmt *NewCaseStmt(Stmt *conds, Stmt *body, TK k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case TK::CASE:    s->kind = T_CASE; break;
    case TK::DEFAULT: s->kind = T_DFLT; break;
    default:          s->kind = T_NUL; break;
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
    return f;
}

static bool optimize = false;

void SetOptimize(bool enable)
{
    optimize = enable;
}

static void print_indent(int depth)
{
    for (int i = 0; i < depth; i++)
        std::cout << "  ";
}

static void print_node(const char *name, int depth, bool end_line = true)
{
    print_indent(depth);
    std::cout << depth << ". ";
    std::cout << "<" << name << ">";
    if (end_line)
        std::cout << std::endl;
    else
        std::cout << ' ';
}

void Prog::Print(int depth) const
{
    print_node("Prog", depth);

    for (const auto &gvar: gvars)
        PrintStmt(gvar.get(), depth + 1);

    for (const auto &func: funcs)
        print_funcdef(func.get(), depth + 1);
}

void Prog::Gen(Bytecode &code) const
{
    if (!main_func) {
        std::cerr << "'main' function not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // global vars
    code.Allocate(scope->VarSize());
    for (const auto &gvar: gvars)
        gen_stmt(&code, gvar.get());

    // call main
    code.CallFunction(main_func->id, main_func->type->func->IsBuiltin());
    code.Exit();

    // global funcs
    for (const auto &func: funcs)
        gen_funcdef(&code, func.get());
}
