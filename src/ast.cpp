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
void NopStmt::Gen(Bytecode &code) const
{
}

void BlockStmt::Gen(Bytecode &code) const
{
    for (const auto &stmt: stmts)
        stmt->Gen(code);
}

void OrStmt::Gen(Bytecode &code) const
{
    Int next = 0;

    if (!IsNull(cond)) {
        // cond
        gen_expr(&code, cond);
        next = code.JumpIfZero(-1);
    }

    // true
    body->Gen(code);

    if (!IsNull(cond)) {
        // close
        const Int addr = code.Jump(-1);
        code.PushOrClose(addr);
        code.BackPatch(next);
    }
}

void IfStmt::Gen(Bytecode &code) const
{
    code.BeginIf();

    for (const auto &stmt: orstmts)
        stmt->Gen(code);

    // exit
    code.BackPatchOrCloses();
}

void ForStmt::Gen(Bytecode &code) const
{
    // init
    code.BeginFor();
    gen_expr(&code, init);

    // body
    const Int begin = code.NextAddr();
    body->Gen(code);

    // post
    code.BackPatchContinues();
    gen_expr(&code, post);

    // cond
    gen_expr(&code, cond);
    const Int exit = code.JumpIfZero(-1);
    code.Jump(begin);

    // exit
    code.BackPatch(exit);
    code.BackPatchBreaks();
}

void JumpStmt::Gen(Bytecode &code) const
{
    const Int addr = code.Jump(-1);

    if (kind == TK::BREAK)
        code.PushBreak(addr);
    else if (kind == TK::CONTINUE)
        code.PushContinue(addr);
}

void CaseStmt::Gen(Bytecode &code) const
{
    Int exit = 0;

    if (kind == TK::CASE) {
        std::vector<Int> trues;
        // eval conds
        for (auto &cond: conds) {
            Int tru = 0;
            Int fls = 0;
            code.DuplicateTop();
            gen_expr(&code, cond);
            code.EqualInt();
            fls = code.JumpIfZero(-1);
            tru = code.Jump(-1);
            code.BackPatch(fls);
            trues.push_back(tru);
        }
        // all conds false -> close case
        exit = code.Jump(-1);
        // one of cond true -> go to body
        for (auto t: trues)
            code.BackPatch(t);
    }

    // body
    body->Gen(code);

    if (kind == TK::CASE) {
        // close
        const Int addr = code.Jump(-1);
        code.PushCaseClose(addr);
        code.BackPatch(exit);
    }
}

void SwitchStmt::Gen(Bytecode &code) const
{
    // init
    code.BeginSwitch();
    gen_expr(&code, cond);

    // cases
    for (const auto &cs: cases)
        cs->Gen(code);

    // quit
    code.BackPatchCaseCloses();
    // remove cond val
    code.Pop();
}

void ReturnStmt::Gen(Bytecode &code) const
{
    gen_expr(&code, expr);
    code.Return();
}

void ExprStmt::Gen(Bytecode &code) const
{
    gen_expr(&code, expr);
}

void FuncDef::Gen(Bytecode &code) const
{
    code.RegisterFunction(funclit_id, func->ParamCount());

    // local vars
    code.Allocate(func->scope->TotalVarSize());

    block->Gen(code);
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
        gvar->Gen(code);

    // call main
    code.CallFunction(main_func->id, main_func->type->func->IsBuiltin());
    code.Exit();

    // global funcs
    for (const auto &func: funcs)
        func->Gen(code);
}
