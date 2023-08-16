#ifndef AST_H
#define AST_H

#include <iostream>
#include <memory>
#include "bytecode.h"

struct Node {
    Node() {}
    virtual ~Node() {}
    virtual long Eval() const = 0;
    virtual void Gen(Bytecode &code) const = 0;
};

struct Expr : public Node {
};

struct IntNumExpr : public Expr {
    IntNumExpr(long val) : ival(val) {}

    long ival = 0;
    long Eval() const override final;
    void Gen(Bytecode &code) const override final;
};

struct IdentExpr : public Expr {
    IdentExpr(int name) {}

    int name = 0;
    long Eval() const override final;
    void Gen(Bytecode &code) const override final;
};

struct AddExpr : public Expr {
    AddExpr(Expr *l, Expr *r) : lhs(l), rhs(r) {}

    std::unique_ptr<Expr> lhs;
    std::unique_ptr<Expr> rhs;
    long Eval() const override final;
    void Gen(Bytecode &code) const override final;
};

struct AssignExpr : public Expr {
    AssignExpr(Expr *l, Expr *r) : lval(l), rval(r) {}

    std::unique_ptr<Expr> lval;
    std::unique_ptr<Expr> rval;
    long Eval() const override final;
    void Gen(Bytecode &code) const override final;
};

void DeleteTree(Node *tree);

#endif // _H
