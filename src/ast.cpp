#include "ast.h"
#include <iostream>
#include <limits>

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

void ExprStmt::Print(int depth) const
{
    print_node("ExprStmt", depth);
    expr->Print(depth + 1);
}

void FuncDef::Print(int depth) const
{
    print_node("FuncDef", depth);
    for (const auto stmt: stmts)
        stmt->Print(depth + 1);
}

void Prog::Print(int depth) const
{
    print_node("Prog", depth);
    for (const auto func: funcs)
        func->Print(depth + 1);
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

long ExprStmt::Eval() const
{
    return expr->Eval();
}

long FuncDef::Eval() const
{
    long ret = 0;
    for (const auto stmt: stmts)
        ret = stmt->Eval();
    return ret;
}

long Prog::Eval() const
{
    long ret = 0;
    for (const auto func: funcs)
        ret = func->Eval();
    return ret;
}

// Gen
void IntNumExpr::Gen(Bytecode &code) const
{
    constexpr Int bytemin = std::numeric_limits<Byte>::min();
    constexpr Int bytemax = std::numeric_limits<Byte>::max();

    if (ival >= bytemin && ival <= bytemax)
        code.LoadByte(ival);
    else
        code.LoadInt(ival);
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


void ExprStmt::Gen(Bytecode &code) const
{
    expr->Gen(code);
}

void FuncDef::Gen(Bytecode &code) const
{
    for (const auto stmt: stmts)
        stmt->Gen(code);
}

void Prog::Gen(Bytecode &code) const
{
    for (const auto func: funcs)
        func->Gen(code);
}
