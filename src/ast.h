#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"

struct Type;

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual long Eval() const = 0;
    virtual void Print(int depth = 0) const = 0;
    virtual void Gen(Bytecode &code) const = 0;
};

struct Expr : public Node {
    const Type *type = nullptr;

    virtual int Addr() const { return -1; }
    virtual bool IsGlobal() const { return false; }
};

struct NullExpr : public Expr {
    long Eval() const override final { return 0; }
    void Print(int depth) const override final {}
    void Gen(Bytecode &code) const override final {}
};

struct IntNumExpr : public Expr {
    IntNumExpr(long n) : ival(n) {}
    long ival;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IdentExpr : public Expr {
    IdentExpr(const Var *v) : var(v) { type = var->type; }
    const Var *var;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override;
    bool IsGlobal() const override { return var->is_global; }
};

struct FieldExpr : public Expr {
    FieldExpr(const Field *f) : fld(f) { type = fld->type; }
    const Field *fld;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override { return fld->id; }
};

struct SelectExpr : public Expr {
    SelectExpr(Expr *i, Expr *f) : inst(i), fld(f) {}
    Expr *inst;
    Expr *fld;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;

    int Addr() const override { return inst->Addr() + fld->Addr(); }
    bool IsGlobal() const override { return inst->IsGlobal(); }
};

struct CallExpr : public Expr {
    CallExpr(Func *f) : func(f) {}
    void AddArg(Expr *e) { args.push_back(e); }
    std::vector<Expr*> args;
    const Func *func;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct AddExpr : public Expr {
    AddExpr(Expr *l, Expr *r) : lhs(l), rhs(r) {}
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct EqualExpr : public Expr {
    EqualExpr(Expr *l, Expr *r) : lhs(l), rhs(r) {}
    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r) : lval(l), rval(r) {}
    std::unique_ptr<Expr> lval;
    std::unique_ptr<Expr> rval;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Stmt : public Node {
};

struct BlockStmt : public Stmt {
    BlockStmt() {}
    ~BlockStmt()
    {
        for (auto stmt: stmts)
            delete stmt;
    }
    void AddStmt(Stmt *stmt) { stmts.push_back(stmt); }
    std::vector<Stmt*> stmts;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IfStmt : public Stmt {
    IfStmt(Expr *cond_, BlockStmt *then_)
        : cond(cond_), then(then_), els(nullptr) {}
    IfStmt(Expr *cond_, BlockStmt *then_, BlockStmt *els_)
        : cond(cond_), then(then_), els(els_) {}
    std::unique_ptr<Expr> cond;
    std::unique_ptr<BlockStmt> then;
    std::unique_ptr<BlockStmt> els;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ReturnStmt : public Stmt {
    ReturnStmt() : expr(new NullExpr()) {}
    ReturnStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct ExprStmt : public Stmt {
    ExprStmt(Expr *e) : expr(e) {}
    std::unique_ptr<Expr> expr;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct FuncDef : public Node {
    FuncDef(Func *f, BlockStmt *b) : func(f), block(b) {}
    ~FuncDef() {}
    Func *func = nullptr;
    std::unique_ptr<BlockStmt> block;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Prog: public Node {
    Prog(Scope *sc) : scope(sc) {}
    void AddFuncDef(FuncDef *func) { funcs.push_back(func); }
    std::vector<FuncDef*> funcs;
    const Scope *scope;
    // TODO remove this
    const Func *main_func = nullptr;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

#endif // _H
