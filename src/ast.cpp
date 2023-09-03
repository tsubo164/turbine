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
    std::cout << var->name << " @" << var->id << std::endl;
}

void FuncCallExpr::Print(int depth) const
{
    print_node("FuncCallExpr", depth, false);
    std::cout << func->name << std::endl;
    for (auto arg: args)
        arg->Print(depth + 1);
}

void AddExpr::Print(int depth) const
{
    print_node("AddExpr", depth);
    lhs->Print(depth + 1);
    rhs->Print(depth + 1);
}

void EqualExpr::Print(int depth) const
{
    print_node("EqualExpr", depth);
    lhs->Print(depth + 1);
    rhs->Print(depth + 1);
}

void AssignExpr::Print(int depth) const
{
    print_node("AssignExpr", depth);
    lval->Print(depth + 1);
    rval->Print(depth + 1);
}

void BlockStmt::Print(int depth) const
{
    print_node("BlockStmt", depth);
    for (const auto stmt: stmts)
        stmt->Print(depth + 1);
}

void IfStmt::Print(int depth) const
{
    print_node("IfStmt", depth);
    cond->Print(depth + 1);
    then->Print(depth + 1);
    if (els)
        els->Print(depth + 1);
}

void ReturnStmt::Print(int depth) const
{
    print_node("ReturnStmt", depth);
    expr->Print(depth + 1);
}

void ExprStmt::Print(int depth) const
{
    print_node("ExprStmt", depth);
    expr->Print(depth + 1);
}

void FuncDef::Print(int depth) const
{
    print_node("FuncDef", depth, false);
    std::cout << func->name << std::endl;
    block->Print(depth + 1);
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

long FuncCallExpr::Eval() const
{
    return 0;
}

long AddExpr::Eval() const
{
    const long l = lhs->Eval();
    const long r = rhs->Eval();
    return l + r;
}

long EqualExpr::Eval() const
{
    const long l = lhs->Eval();
    const long r = rhs->Eval();
    return l + r;
}

long AssignExpr::Eval() const
{
    return rval->Eval();
}

long BlockStmt::Eval() const
{
    long ret = 0;
    for (const auto stmt: stmts)
        ret = stmt->Eval();
    return ret;
}

long IfStmt::Eval() const
{
    return cond->Eval();
}

long ReturnStmt::Eval() const
{
    return expr->Eval();
}

long ExprStmt::Eval() const
{
    return expr->Eval();
}

long FuncDef::Eval() const
{
    return block->Eval();
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
    if (var->is_global)
        code.LoadGlobal(var->id);
    else
        code.LoadLocal(var->id);
}

void FuncCallExpr::Gen(Bytecode &code) const
{
    for (auto it = args.rbegin(); it != args.rend(); ++it)
        (*it)->Gen(code);
    code.CallFunction(func->id);
}

void AddExpr::Gen(Bytecode &code) const
{
    lhs->Gen(code);
    rhs->Gen(code);
    code.AddInt();
}

void EqualExpr::Gen(Bytecode &code) const
{
    lhs->Gen(code);
    rhs->Gen(code);
    code.EqualInt();
}

void AssignExpr::Gen(Bytecode &code) const
{
    // rval first
    rval->Gen(code);
    // TODO remove dynamic_cast
    IdentExpr *ident = dynamic_cast<IdentExpr*>(lval.get());
    const int id = ident->var->id;

    if (ident->var->is_global)
        code.StoreGlobal(id);
    else
        code.StoreLocal(id);
}

void BlockStmt::Gen(Bytecode &code) const
{
    for (const auto stmt: stmts)
        stmt->Gen(code);
}

void IfStmt::Gen(Bytecode &code) const
{
    Int jiz = 0;
    Int jmp = 0;

    // cond
    cond->Gen(code);
    jiz = code.JumpIfZero(-1);

    // true
    then->Gen(code);
    if (els)
        jmp = code.Jump(-1);

    // false
    code.BackPatch(jiz);
    if (els) {
        els->Gen(code);
        code.BackPatch(jmp);
    }
}

void ReturnStmt::Gen(Bytecode &code) const
{
    expr->Gen(code);
    code.Return();
}

void ExprStmt::Gen(Bytecode &code) const
{
    expr->Gen(code);
}

void FuncDef::Gen(Bytecode &code) const
{
    code.RegisterFunction(func->id, func->ParamCount());

    // local vars
    if (func->VarCount() > 0)
        code.AllocateLocal(func->VarCount());

    block->Gen(code);
}

void Prog::Gen(Bytecode &code) const
{
    if (!main_func) {
        std::cerr << "'main' function not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // global vars
    if (scope->VarCount() > 0)
        code.AllocateLocal(scope->VarCount());

    // call main
    code.CallFunction(main_func->id);
    code.Exit();

    // global funcs
    for (const auto func: funcs)
        func->Gen(code);
}
