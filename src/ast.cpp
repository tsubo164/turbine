#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

static bool optimize = false;

void SetOptimize(bool enable)
{
    optimize = enable;
}

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
void FuncDef::Print(int depth) const
{
    print_node("FuncDef", depth, false);
    std::cout << "\"" << var->name << "\" " <<
        func->return_type << std::endl;
    PrintStmt(block, depth + 1);
}

void Prog::Print(int depth) const
{
    print_node("Prog", depth);

    for (const auto &gvar: gvars)
        gvar->Print(depth + 1);

    for (const auto &func: funcs)
        func->Print(depth + 1);
}

// Gen
void FuncDef::Gen(Bytecode &code) const
{
    code.RegisterFunction(funclit_id, func->ParamCount());

    // local vars
    code.Allocate(func->scope->TotalVarSize());

    gen_stmt(&code, block);
}

void Prog::Gen(Bytecode &code) const
{
    if (!main_func) {
        std::cerr << "'main' function not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // global vars
    code.Allocate(scope->VarSize());
    for (const auto &gvar: gvars)
        gen_stmt(&code, gvar.get());

    // call main
    code.CallFunction(main_func->id, main_func->type->func->IsBuiltin());
    code.Exit();

    // global funcs
    for (const auto &func: funcs)
        func->Gen(code);
}
