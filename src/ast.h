#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"
#include "lexer.h"
#include "type.h"
#include "escseq.h"

// XXX TEST ==============
#include "compiler.h"

void SetOptimize(bool enable);

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual void Print(int depth = 0) const {};
    virtual void Gen(Bytecode &code) const {};
};

typedef union Val {
    long i;
    double f;
    const char *s;
    std::string_view sv;
} Val;

struct Expr : public Node {
    Expr() {}
    Expr(const Type *t) : type(t) {}
    const Type *type;

    // XXX TEST ==============
    int kind = T_NUL;
    Expr *l = nullptr;
    Expr *r = nullptr;
    Expr *next = nullptr;
    Var *var = nullptr;
    Val val = {0};
    const Field *fld = nullptr;
    std::string converted;

    std::vector<Expr*> args;
    // TODO need func for easy access?
    // const Func *func;
    Pos pos;
};

inline
void AddArg(Expr *e, Expr *arg) { e->args.push_back(arg); }
inline
int ArgCount(Expr *e) { return e->args.size(); }
inline
const Expr *GetArg(Expr *e, int index)
{
    if (index < 0 || index >= ArgCount(e))
        return nullptr;
    return e->args[index];
}

inline
int ConvertEscSeq(std::string_view s, std::string &converted)
{
    return ConvertEscapeSequence(s, converted);
}

inline
Expr *NewNullExpr(void)
{
    Expr *e = CALLOC(Expr);
    e->type = NewNilType();
    e->kind = T_NUL;
    return e;
}

inline
Expr *NewNilLitExpr(void)
{
    Expr *e = CALLOC(Expr);
    e->type = NewNilType();
    e->kind = T_NILLIT;
    return e;
}

inline
Expr *NewBoolLitExpr(bool b)
{
    Expr *e = CALLOC(Expr);
    e->type = NewBoolType();
    e->kind = T_BOLLIT;
    e->val.i = b;
    return e;
}

inline
Expr *NewIntLitExpr(long l)
{
    Expr *e = CALLOC(Expr);
    e->type = NewIntType();
    e->kind = T_INTLIT;
    e->val.i = l;
    return e;
}

inline
Expr *NewFloatLitExpr(double d)
{
    Expr *e = CALLOC(Expr);
    e->type = NewFloatType();
    e->kind = T_FLTLIT;
    e->val.f = d;
    return e;
}

inline
Expr *NewStringLitExpr(std::string_view s)
{
    Expr *e = CALLOC(Expr);
    e->type = NewStringType();
    e->kind = T_STRLIT;
    e->val.sv = s;
    return e;
}

inline
Expr *NewConversionExpr(Expr *from, Type *to)
{
    Expr *e = CALLOC(Expr);
    e->type = to;
    e->kind = T_CONV;
    e->l = from;
    return e;
}

inline
Expr *NewIdentExpr(Var *v)
{
    Expr *e = CALLOC(Expr);
    e->type = v->type;
    e->kind = T_IDENT;
    e->var = v;
    return e;
}

inline
Expr *NewFieldExpr(Field *f)
{
    Expr *e = CALLOC(Expr);
    e->type = f->type;
    e->kind = T_FIELD;
    e->fld = f;
    return e;
}

inline
Expr *NewSelectExpr(Expr *inst, Expr *fld)
{
    Expr *e = CALLOC(Expr);
    e->type = fld->type;
    e->kind = T_SELECT;
    e->l = inst;
    e->r = fld;
    return e;
}

inline
Expr *NewIndexExpr(Expr *ary, Expr *idx)
{
    Expr *e = CALLOC(Expr);
    e->type = ary->type->underlying;
    e->kind = T_INDEX;
    e->l = ary;
    e->r = idx;
    return e;
}

inline
Expr *NewCallExpr(Expr *callee, Pos p)
{
    Expr *e = CALLOC(Expr);
    e->type = callee->type->func->return_type;
    e->kind = T_CALL;
    e->l = callee;
    e->pos = p;
    return e;
}

inline
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

inline
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

inline
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

inline
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

inline
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

struct BlockStmt;
struct OrStmt;
struct CaseStmt;

struct Stmt : public Node {
    int kind = 0;
    std::vector<std::unique_ptr<Stmt>> stmts;
    //std::unique_ptr<Expr> cond;
    //std::unique_ptr<BlockStmt> body;
    Expr* init = nullptr;
    Expr* cond = nullptr;
    Expr* post = nullptr;
    BlockStmt* body = nullptr;
    std::vector<OrStmt*> orstmts;
    std::vector<Expr*> conds;
    //std::vector<CaseStmt*> cases;
    //std::vector<std::unique_ptr<CaseStmt>> cases;
};

struct NopStmt : public Stmt {
    NopStmt()
    {
        // XXX TEST
        Stmt::kind = T_NOP;
    }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct BlockStmt : public Stmt {
    BlockStmt()
    {
        // XXX TEST
        Stmt::kind = T_BLOCK;
    }
    //std::vector<std::unique_ptr<Stmt>> stmts;

    void AddStmt(Stmt *stmt) { stmts.emplace_back(stmt); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct OrStmt : public Stmt {
    OrStmt(Expr *cond_, BlockStmt *body_)
        //: cond(cond_), body(body_)
    {
        // XXX TEST
        Stmt::kind = T_ELS;
        cond = cond_;
        body = body_;
    }
    //std::unique_ptr<Expr> cond;
    //std::unique_ptr<BlockStmt> body;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IfStmt : public Stmt {
    IfStmt(Expr *cond, BlockStmt *body)
    {
        // XXX TEST
        Stmt::kind = T_IF;
        AddOr(new OrStmt(cond, body));
    }
    //std::vector<std::unique_ptr<OrStmt>> orstmts;

    void AddOr(OrStmt *ors) { orstmts.emplace_back(ors); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ForStmt : public Stmt {
    ForStmt(Expr *i, Expr *c, Expr *p, BlockStmt *b)
        //: init(i), cond(c), post(p), body(b)
    {
        // XXX TEST
        Stmt::kind = T_FOR;
        init = i;
        cond = c;
        post = p;
        body = b;
    }
    //std::unique_ptr<Expr> init;
    //std::unique_ptr<Expr> cond;
    //std::unique_ptr<Expr> post;
    //std::unique_ptr<BlockStmt> body;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct JumpStmt : public Stmt {
    JumpStmt(TK k) : kind(k)
    {
        // XXX TEST
        switch (k) {
        case TK::BREAK:    Stmt::kind = T_BRK; break;
        case TK::CONTINUE: Stmt::kind = T_CNT; break;
        default:           Stmt::kind = T_NUL; break;
        }

    }
    TK kind;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct CaseStmt : public Stmt {
    CaseStmt(TK k) : kind(k)
    {
        // XXX TEST
        switch (k) {
        case TK::CASE:    Stmt::kind = T_CASE; break;
        case TK::DEFAULT: Stmt::kind = T_DFLT; break;
        default:          Stmt::kind = T_NUL; break;
        }
    }
    //std::vector<std::unique_ptr<Expr>> conds;
    //std::unique_ptr<BlockStmt> body;
    TK kind;

    void AddCond(Expr *cond) { conds.emplace_back(cond); }
    //void AddBody(BlockStmt *b) { body.reset(b); }
    void AddBody(BlockStmt *b) { body = b; }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct SwitchStmt : public Stmt {
    SwitchStmt(Expr *c) : cond(c) {}
        /*
    {
        // XXX TEST
        Stmt::kind = T_SWT;
    }
        */
    void AddCase(CaseStmt *cs) { cases.emplace_back(cs); }
    std::unique_ptr<Expr> cond;
    std::vector<std::unique_ptr<CaseStmt>> cases;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ReturnStmt : public Stmt {
    ReturnStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ExprStmt : public Stmt {
    ExprStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct FuncDef : public Node {
    FuncDef(Func *f, BlockStmt *b) : func(f), block(b) {}
    FuncDef(Var *v, BlockStmt *b) : func(v->type->func), var(v), block(b) {}
    ~FuncDef() {}
    // TODO remove this
    const Func *func = nullptr;
    const Var *var = nullptr;

    std::unique_ptr<BlockStmt> block;
    // TODO make FuncLitExpr and remove this
    int funclit_id = 0;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Prog: public Node {
    Prog(Scope *sc) : scope(sc) {}
    void AddFuncDef(FuncDef *func) { funcs.emplace_back(func); }
    void AddGlobalVar(Stmt *gvar) { gvars.emplace_back(gvar); }

    const Scope *scope;
    std::vector<std::unique_ptr<FuncDef>> funcs;
    std::vector<std::unique_ptr<Stmt>> gvars;
    // TODO remove this
    const Var *main_func = nullptr;

    void Print(int depth = 0) const override final;
    void Gen(Bytecode &code) const override final;
};

#endif // _H
