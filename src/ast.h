#ifndef AST_H
#define AST_H

#include <memory>
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

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r) : lval(l), rval(r) {}

    std::unique_ptr<Expr> lval;
    std::unique_ptr<Expr> rval;
    long Eval() const override final;
    void Print(int depth) const override final;
    void Gen(Bytecode &code) const override final;
};

void DeleteTree(Node *tree);

#endif // _H
