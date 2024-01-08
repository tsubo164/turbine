#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"
#include "lexer.h"
#include "type.h"

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
Expr *NewNullExpr(void)
{
    Expr *e = CALLOC(Expr);
    e->type = NewNilType();
    e->kind = T_NUL;
    return e;
}

struct NilValExpr : public Expr {
    NilValExpr() : Expr(new Type(TY::NIL))
    {
        // XXX TEST ==============
        kind = T_NILLIT;
    }
};

struct BoolValExpr : public Expr {
    BoolValExpr(bool b) : Expr(new Type(TY::BOOL))
    {
        // XXX TEST ==============
        kind = T_BOLLIT;
        Expr::val.i = b;
    }
};

struct IntValExpr : public Expr {
    IntValExpr(long l) : Expr(new Type(TY::INT))
    {
        // XXX TEST ==============
        kind = T_INTLIT;
        Expr::val.i = l;
    }
};

struct FltValExpr : public Expr {
    FltValExpr(double d) : Expr(new Type(TY::FLOAT))
    {
        // XXX TEST ==============
        kind = T_FLTLIT;
        Expr::val.f = d;
    }
};

struct StrValExpr : public Expr {
    StrValExpr(std::string_view s) : Expr(new Type(TY::STRING)), val(s)
    {
        // XXX TEST ==============
        kind = T_STRLIT;
        Expr::val.sv = s;
    }
    std::string_view val;
    //std::string converted;

    int ConvertEscSeq();
};

struct ConvertExpr : public Expr {
    ConvertExpr(Expr *e, const Type *totype) : Expr(totype)
    {
        // XXX TEST ==============
        kind = T_CONV;
        Expr::l = e;
    }
};

struct IdentExpr : public Expr {
    IdentExpr(Var *v) : Expr(v->type)
    {
        // XXX TEST ==============
        kind = T_IDENT;
        Expr::var = v;
    }
};

struct FieldExpr : public Expr {
    FieldExpr(const Field *f) : Expr(f->type)
    {
        // XXX TEST ==============
        kind = T_FIELD;
        Expr::fld = f;
    }
};

struct SelectExpr : public Expr {
    SelectExpr(Expr *inst, Expr *fld) : Expr(fld->type)
    {
        // XXX TEST ==============
        kind = T_SELECT;
        Expr::l = inst;
        Expr::r = fld;
    }
};

struct IndexExpr : public Expr {
    IndexExpr(Expr *ary, Expr *idx)
        : Expr(ary->type->underlying)
    {
        // XXX TEST ==============
        kind = T_INDEX;
        Expr::l = ary;
        Expr::r = idx;
    }
};

struct CallExpr : public Expr {
    CallExpr(Expr *e, Pos p)
        : Expr(e->type->func->return_type)
    {
        // XXX TEST ==============
        kind = T_CALL;
        Expr::l = e;
        Expr::pos = p;
    }
    //std::vector<Expr*> args;
    // TODO need func for easy access?
    // const Func *func;
    //const Pos pos;

    void AddArg(Expr *e) { args.push_back(e); }
    int ArgCount() const { return args.size(); }
    const Expr *GetArg(int index)
    {
        if (index < 0 || index >= ArgCount())
            return nullptr;
        return args[index];
    }
};

struct BinaryExpr : public Expr {
    BinaryExpr(Expr *L, Expr *R, TK k)
        : Expr(L->type)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::PLUS:         Expr::kind = T_ADD; break;
        case TK::MINUS:        Expr::kind = T_SUB; break;
        case TK::STAR:         Expr::kind = T_MUL; break;
        case TK::SLASH:        Expr::kind = T_DIV; break;
        case TK::PERCENT:      Expr::kind = T_REM; break;
        case TK::BAR:          Expr::kind = T_OR;  break;
        case TK::BAR2:         Expr::kind = T_LOR; break;
        case TK::AMP:          Expr::kind = T_AND; break;
        case TK::AMP2:         Expr::kind = T_LAND; break;
        case TK::EQ2:          Expr::kind = T_EQ;  break;
        case TK::EXCLEQ:       Expr::kind = T_NEQ; break;
        case TK::EXCL:         Expr::kind = T_LNOT; break;
        case TK::CARET:        Expr::kind = T_XOR; break;
        case TK::TILDA:        Expr::kind = T_NOT; break;
        case TK::LT2:          Expr::kind = T_SHL; break;
        case TK::GT2:          Expr::kind = T_SHR; break;
        case TK::LT:           Expr::kind = T_LT; break;
        case TK::GT:           Expr::kind = T_GT; break;
        case TK::LTE:          Expr::kind = T_LTE; break;
        case TK::GTE:          Expr::kind = T_GTE; break;
        default:       Expr::kind = T_ADD; break;
        }
        Expr::l = L;
        Expr::r = R;
    }
    BinaryExpr(Expr *L, Expr *R, Type *t, TK k)
        : Expr(t)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::PLUS:         Expr::kind = T_ADD; break;
        case TK::MINUS:        Expr::kind = T_SUB; break;
        case TK::STAR:         Expr::kind = T_MUL; break;
        case TK::SLASH:        Expr::kind = T_DIV; break;
        case TK::PERCENT:      Expr::kind = T_REM; break;
        case TK::BAR:          Expr::kind = T_OR;  break;
        case TK::BAR2:         Expr::kind = T_LOR; break;
        case TK::AMP:          Expr::kind = T_AND; break;
        case TK::AMP2:         Expr::kind = T_LAND; break;
        case TK::EQ2:          Expr::kind = T_EQ;  break;
        case TK::EXCLEQ:       Expr::kind = T_NEQ; break;
        case TK::EXCL:         Expr::kind = T_LNOT; break;
        case TK::CARET:        Expr::kind = T_XOR; break;
        case TK::TILDA:        Expr::kind = T_NOT; break;
        case TK::LT2:          Expr::kind = T_SHL; break;
        case TK::GT2:          Expr::kind = T_SHR; break;
        case TK::LT:           Expr::kind = T_LT; break;
        case TK::GT:           Expr::kind = T_GT; break;
        case TK::LTE:          Expr::kind = T_LTE; break;
        case TK::GTE:          Expr::kind = T_GTE; break;
        default:       Expr::kind = T_ADD; break;
        }
        Expr::l = L;
        Expr::r = R;
    }
};

struct UnaryExpr : public Expr {
    UnaryExpr(Expr *R, const Type *t, TK k) : Expr(t)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::AMP:    Expr::kind = T_ADR; break;
        case TK::PLUS:   Expr::kind = T_POS; break;
        case TK::MINUS:  Expr::kind = T_NEG; break;
        case TK::EXCL:   Expr::kind = T_LNOT; break;
        case TK::TILDA:  Expr::kind = T_NOT; break;
        case TK::STAR:   Expr::kind = T_DRF; break;
        default:       Expr::kind = T_NUL; break;
        }
        Expr::l = R;
    }
    UnaryExpr(Expr *R, TK k) : Expr(R->type)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::AMP:    Expr::kind = T_ADR; break;
        case TK::PLUS:   Expr::kind = T_POS; break;
        case TK::MINUS:  Expr::kind = T_NEG; break;
        case TK::EXCL:   Expr::kind = T_LNOT; break;
        case TK::TILDA:  Expr::kind = T_NOT; break;
        case TK::STAR:   Expr::kind = T_DRF; break;
        default:       Expr::kind = T_NUL; break;
        }
        Expr::l = R;
    }
};

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r, TK k)
        : Expr(l->type)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::EQ:        Expr::kind = T_ASSN; break;
        case TK::PLUSEQ:    Expr::kind = T_AADD; break;
        case TK::MINUSEQ:   Expr::kind = T_ASUB; break;
        case TK::STAREQ:    Expr::kind = T_AMUL; break;
        case TK::SLASHEQ:   Expr::kind = T_ADIV; break;
        case TK::PERCENTEQ: Expr::kind = T_AREM; break;
        default:       Expr::kind = T_NUL; break;
        }
        Expr::l = l;
        Expr::r = r;
    }
};

struct IncDecExpr : public Expr {
    IncDecExpr(Expr *e, TK k) : Expr(e->type)
    {
        // XXX TEST ==============
        switch (k) {
        case TK::PLUS2:    Expr::kind = T_INC; break;
        case TK::MINUS2:   Expr::kind = T_DEC; break;
        default:       Expr::kind = T_ADD; break;
        }
        Expr::l = e;
    }
};

struct Stmt : public Node {
};

struct NopStmt : public Stmt {
    NopStmt() {}

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct BlockStmt : public Stmt {
    BlockStmt() {}
    std::vector<std::unique_ptr<Stmt>> stmts;

    void AddStmt(Stmt *stmt) { stmts.emplace_back(stmt); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct OrStmt : public Stmt {
    OrStmt(Expr *cond_, BlockStmt *body_)
        : cond(cond_), body(body_) {}
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> body;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IfStmt : public Stmt {
    IfStmt(Expr *cond, BlockStmt *body) { AddOr(new OrStmt(cond, body)); }
    std::vector<std::unique_ptr<OrStmt>> orstmts;

    void AddOr(OrStmt *ors) { orstmts.emplace_back(ors); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ForStmt : public Stmt {
    ForStmt(Expr *i, Expr *c, Expr *p, BlockStmt *b)
        : init(i), cond(c), post(p), body(b) {}
    std::unique_ptr<Expr> init;
    std::unique_ptr<Expr> cond;
    std::unique_ptr<Expr> post;
    std::unique_ptr<BlockStmt> body;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct JumpStmt : public Stmt {
    JumpStmt(TK k) : kind(k) {}
    TK kind;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct CaseStmt : public Stmt {
    CaseStmt(TK k) : kind(k) {}
    std::vector<std::unique_ptr<Expr>> conds;
    std::unique_ptr<BlockStmt> body;
    TK kind;

    void AddCond(Expr *cond) { conds.emplace_back(cond); }
    void AddBody(BlockStmt *b) { body.reset(b); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct SwitchStmt : public Stmt {
    SwitchStmt(Expr *c) : cond(c) {}
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
