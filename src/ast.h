#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"

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
    IdentExpr(int name_) : name(name_) {}
    int name;

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
    FuncDef() {}
    ~FuncDef()
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

struct Prog: public Node {
    void AddFuncDef(FuncDef *func) { funcs.push_back(func); }
    std::vector<FuncDef*> funcs;

    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

#endif // _H
