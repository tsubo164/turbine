#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

//--------------------------------
// Stmt
Stmt *NewNopStmt(void)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_NOP;
    return s;
}

Stmt *NewBlockStmt(Stmt *children)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_BLOCK;
    s->children = children;
    return s;
}

Stmt *NewOrStmt(Expr *cond, Stmt *body)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_ELS;
    s->cond = cond;
    s->body = body;
    return s;
}

Stmt *NewIfStmt(Stmt *or_list)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_IF;
    s->children = or_list;
    return s;
}

Stmt *NewForStmt(Expr *init, Expr *cond, Expr *post, Stmt *body)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_FOR;
    s->expr = init;
    s->cond = cond;
    s->post = post;
    s->body = body;
    return s;
}

Stmt *NewJumpStmt(TK k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case TK::BREAK:    s->kind = T_BRK; break;
    case TK::CONTINUE: s->kind = T_CNT; break;
    default:           s->kind = T_NUL; break;
    }
    return s;
}

Stmt *NewCaseStmt(Stmt *conds, Stmt *body, TK k)
{
    Stmt *s = CALLOC(Stmt);
    switch (k) {
    case TK::CASE:    s->kind = T_CASE; break;
    case TK::DEFAULT: s->kind = T_DFLT; break;
    default:          s->kind = T_NUL; break;
    }
    s->children = conds;
    s->body = body;
    return s;
}

Stmt *NewSwitchStmt(Expr *cond, Stmt *cases)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_SWT;
    s->cond = cond;
    s->children = cases;
    return s;
}

Stmt *NewReturnStmt(Expr *e)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_RET;
    s->expr = e;
    return s;
}

Stmt *NewExprStmt(Expr *e)
{
    Stmt *s = CALLOC(Stmt);
    s->kind = T_EXPR;
    s->expr = e;
    return s;
}

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
        PrintStmt(gvar.get(), depth + 1);

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
