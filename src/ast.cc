#include "ast.h"
#include <iostream>

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

// Print
void IntNumExpr::Print(int depth) const
{
    print_node("IntNumExpr", depth, false);
    std::cout << ival << std::endl;
}

void IdentExpr::Print(int depth) const
{
    print_node("IdentExpr", depth, false);
    std::cout << name << std::endl;
}

void AddExpr::Print(int depth) const
{
    print_node("AddExpr", depth);
    lhs->Print(depth + 1);
    rhs->Print(depth + 1);
}

void AssignExpr::Print(int depth) const
{
    print_node("AssignExpr", depth);
    lval->Print(depth + 1);
    rval->Print(depth + 1);
}

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
    return rval->Eval();
}

// Gen
void IntNumExpr::Gen(Bytecode &code) const
{
    code.LoadByte(ival);
}

void IdentExpr::Gen(Bytecode &code) const
{
    code.LoadLocal(0);
}

void AddExpr::Gen(Bytecode &code) const
{
    lhs->Gen(code);
    rhs->Gen(code);
    code.AddInt();
}

void AssignExpr::Gen(Bytecode &code) const
{
    // rval first
    rval->Gen(code);
    const int id = lval->Eval();
    code.StoreLocal(id);
}

void DeleteTree(Node *tree)
{
    delete tree;
}
