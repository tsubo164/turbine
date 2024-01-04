#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"
#include "lexer.h"
#include "type.h"

void SetOptimize(bool enable);

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual void Print(int depth = 0) const = 0;
    virtual void Gen(Bytecode &code) const = 0;
};

struct Expr : public Node {
    Expr(const Type *t) : type(t) {}
    const Type *type;

    virtual int Addr() const { return -1; }
    virtual bool IsAbsAddr() const { return false; }
    virtual bool IsGlobal() const { return false; }
    virtual bool IsNull() const { return false; }

    virtual void GenAddr(Bytecode &code) const {}
    virtual bool Eval(long &result) const { return false; }
    virtual bool EvalAddr(int &result) const { return false; }
};

struct NullExpr : public Expr {
    NullExpr() : Expr(new Type(TY::NIL)) {}

    void Print(int depth) const override final {}
    void Gen(Bytecode &code) const override final {}
    bool IsNull() const override final { return true; }
};

struct NilValExpr : public Expr {
    NilValExpr() : Expr(new Type(TY::NIL)) {}

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct BoolValExpr : public Expr {
    BoolValExpr(bool b) : Expr(new Type(TY::BOOL)), val(b) {}
    bool val;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IntValExpr : public Expr {
    IntValExpr(long l) : Expr(new Type(TY::INT)), val(l) {}
    long val;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    bool Eval(long &result) const override final { result = val; return true; }
};

struct FltValExpr : public Expr {
    FltValExpr(double d) : Expr(new Type(TY::FLOAT)), val(d) {}
    double val;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct StrValExpr : public Expr {
    StrValExpr(std::string_view s) : Expr(new Type(TY::STRING)), val(s) {}
    std::string_view val;
    std::string converted;

    int ConvertEscSeq();

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ConvertExpr : public Expr {
    ConvertExpr(const Expr *e, const Type *totype) : Expr(totype), expr(e) {}
    std::unique_ptr<const Expr> expr;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IdentExpr : public Expr {
    IdentExpr(const Var *v) : Expr(v->type), var(v) {}
    const Var *var;

    int Addr() const override { return var->id; }
    bool IsGlobal() const override { return var->is_global; }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    void GenAddr(Bytecode &code) const override final;
    bool EvalAddr(int &result) const override final { result = var->id; return true; }
};

struct FieldExpr : public Expr {
    FieldExpr(const Field *f) : Expr(f->type), fld(f) {}
    const Field *fld;

    int Addr() const override { return fld->id; }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    void GenAddr(Bytecode &code) const override final;
    bool EvalAddr(int &result) const override final { result = fld->id; return true; }
};

struct SelectExpr : public Expr {
    SelectExpr(Expr *i, Expr *f) : Expr(f->type), inst(i), fld(f) {}
    Expr *inst;
    Expr *fld;

    int Addr() const override { return inst->Addr() + fld->Addr(); }
    bool IsGlobal() const override { return inst->IsGlobal(); }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    void GenAddr(Bytecode &code) const override final;
};

struct IndexExpr : public Expr {
    IndexExpr(Expr *a, Expr *i)
        : Expr(a->type->underlying), ary(a), idx(i) {}
    std::unique_ptr<Expr> ary;
    std::unique_ptr<Expr> idx;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    void GenAddr(Bytecode &code) const override final;
};

struct CallExpr : public Expr {
    CallExpr(const Expr *e, Pos p)
        : Expr(e->type->func->return_type), expr(e), pos(p) {}
    std::vector<Expr*> args;
    const Expr *expr;
    // TODO need func for easy access?
    // const Func *func;
    const Pos pos;

    void AddArg(Expr *e) { args.push_back(e); }
    int ArgCount() const { return args.size(); }
    const Expr *GetArg(int index)
    {
        if (index < 0 || index >= ArgCount())
            return nullptr;
        return args[index];
    }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct BinaryExpr : public Expr {
    BinaryExpr(Expr *L, Expr *R, TK k)
        : Expr(L->type), l(L), r(R), kind(k) {}
    BinaryExpr(Expr *L, Expr *R, Type *t, TK k)
        : Expr(t), l(L), r(R), kind(k) {}
    std::unique_ptr<Expr> l;
    std::unique_ptr<Expr> r;
    TK kind;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    bool Eval(long &result) const override final;
};

struct UnaryExpr : public Expr {
    UnaryExpr(Expr *R, const Type *t, TK k) : Expr(t), r(R), kind(k) {}
    UnaryExpr(Expr *R, TK k) : Expr(R->type), r(R), kind(k) {}
    std::unique_ptr<Expr> r;
    const TK kind;

    int Addr() const override { return kind == TK::STAR ? r->Addr() : -1; }
    bool IsAbsAddr() const override { return kind == TK::STAR; }

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
    void GenAddr(Bytecode &code) const override final;
    bool Eval(long &result) const override final;
};

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r, TK k)
        : Expr(l->type), lval(l), rval(r), kind(k) {}
    std::unique_ptr<Expr> lval;
    std::unique_ptr<Expr> rval;
    TK kind;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IncDecExpr : public Expr {
    IncDecExpr(Expr *e, TK k) : Expr(e->type), lval(e), kind(k) {}
    std::unique_ptr<Expr> lval;
    TK kind;

    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
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
