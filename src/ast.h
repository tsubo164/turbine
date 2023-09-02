#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual long Eval() const = 0;
    virtual void Print(int depth = 0) const = 0;
    virtual void Gen(Bytecode &code) const = 0;
};

struct Expr : public Node {
};

struct NullExpr : public Expr {
    long Eval() const override final { return 0; }
    void Print(int depth) const override final {}
    void Gen(Bytecode &code) const override final {}
};

struct IntNumExpr : public Expr {
    IntNumExpr(long val) : ival(val) {}
    long ival;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct IdentExpr : public Expr {
    IdentExpr(Variable *var_) : var(var_) {}
    const Variable *var;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct FuncCallExpr : public Expr {
    FuncCallExpr(FuncObj *func_) : func(func_) {}
    void AddArgument(Expr *expr) { args.push_back(expr); }
    std::vector<Expr*> args;
    const FuncObj *func;

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
    void AddStatement(Stmt *stmt) { stmts.push_back(stmt); }
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
    ReturnStmt(int argc_) : expr(new NullExpr()), argc(argc_) {}
    ReturnStmt(Expr *e, int argc_) : expr(e), argc(argc_) {}
    std::unique_ptr<Expr> expr;
    int argc;

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
    FuncDef(FuncObj *f, BlockStmt *b) : func(f), block(b) {}
    ~FuncDef() {}
    FuncObj *func = nullptr;
    std::unique_ptr<BlockStmt> block;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

struct Prog: public Node {
    void AddFuncDef(FuncDef *func) { funcs.push_back(func); }
    std::vector<FuncDef*> funcs;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

#endif // _H
