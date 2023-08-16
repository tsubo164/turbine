#include "ast.h"
#include <iostream>


// Eval
long IntNumExpr::Eval() const
{
    return ival;
}

long IdentExpr::Eval() const
{
    return 0;
}

long AddExpr::Eval() const
{
    const long l = lhs->Eval();
    const long r = rhs->Eval();
    return l + r;
}

long AssignExpr::Eval() const
{
    return 0;
}

// Gen
void IntNumExpr::Gen(Bytecode &code) const
{
    code.LoadByte(ival);
}

void IdentExpr::Gen(Bytecode &code) const
{
}

void AddExpr::Gen(Bytecode &code) const
{
    lhs->Gen(code);
    rhs->Gen(code);
    code.AddInt();
}

void AssignExpr::Gen(Bytecode &code) const
{
}

void DeleteTree(Node *tree)
{
    delete tree;
}
