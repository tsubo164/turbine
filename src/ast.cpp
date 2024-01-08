#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

#define EMIT(code, ty, op) \
    do { \
    if ((ty)->IsInt() || (ty)->IsBool()) \
        (code).op##Int(); \
    else if ((ty)->IsFloat()) \
        (code).op##Float(); \
    } while (0)

#define EMITS(code, ty, op, ops) \
    do { \
    if ((ty)->IsInt() || (ty)->IsBool()) \
        (code).op##Int(); \
    else if ((ty)->IsFloat()) \
        (code).op##Float(); \
    else if ((ty)->IsString()) \
        (code).ops##String(); \
    } while (0)

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

int StrValExpr::ConvertEscSeq()
{
    return ConvertEscapeSequence(val, converted);
}

// Print
void NilValExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void BoolValExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void IntValExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void FltValExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void StrValExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void ConvertExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void IdentExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void FieldExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void SelectExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void IndexExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void CallExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void BinaryExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void UnaryExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void AssignExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void IncDecExpr::Print(int depth) const
{
    print_expr(this, depth);
}

void NopStmt::Print(int depth) const
{
    print_node("NopStmt", depth);
}

void BlockStmt::Print(int depth) const
{
    print_node("BlockStmt", depth);
    for (const auto &stmt: stmts)
        stmt->Print(depth + 1);
}

void OrStmt::Print(int depth) const
{
    print_node("OrStmt", depth);
    cond->Print(depth + 1);
    body->Print(depth + 1);
}

void IfStmt::Print(int depth) const
{
    print_node("IfStmt", depth);
    for (const auto &stmt: orstmts)
        stmt->Print(depth + 1);
}

void ForStmt::Print(int depth) const
{
    print_node("ForStmt", depth);
    init->Print(depth + 1);
    cond->Print(depth + 1);
    post->Print(depth + 1);
    body->Print(depth + 1);
}

void JumpStmt::Print(int depth) const
{
    print_node("JumpStmt", depth, false);
    std::cout << "\"" << kind << "\"" << std::endl;;
}

void CaseStmt::Print(int depth) const
{
    print_node("CaseStmt", depth, false);
    std::cout << "\"" << kind << "\"" << std::endl;;
    for (auto &cond: conds)
        cond->Print(depth + 1);
    body->Print(depth + 1);
}

void SwitchStmt::Print(int depth) const
{
    print_node("SwitchStmt", depth);
    for (const auto &cs: cases)
        cs->Print(depth + 1);
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
    std::cout << "\"" << var->name << "\" " <<
        func->return_type << std::endl;
    block->Print(depth + 1);
}

void Prog::Print(int depth) const
{
    print_node("Prog", depth);

    for (const auto &gvar: gvars)
        gvar->Print(depth + 1);

    for (const auto &func: funcs)
        func->Print(depth + 1);
}

// Eval
bool BinaryExpr::Eval(long &result) const
{
    long L = 0, R = 0;
    bool ok;

    ok = l->Eval(L);
    if (!ok)
        return false;
    ok = r->Eval(R);
    if (!ok)
        return false;

    switch (kind) {
    case TK::PLUS:    result = L + R; return true;
    case TK::MINUS:   result = L - R; return true;
    case TK::STAR:    result = L * R; return true;
    case TK::SLASH:   result = L / R; return true;
    case TK::PERCENT: result = L % R; return true;
    default: return false;
    }
}

bool UnaryExpr::Eval(long &result) const
{
    long R = 0;
    bool ok;

    ok = r->Eval(R);
    if (!ok)
        return false;

    switch (kind) {
    case TK::PLUS:  result = +R; return true;
    case TK::MINUS: result = -R; return true;
    case TK::EXCL:  result = !R; return true;
    case TK::TILDA: result = ~R; return true;
    default: return false;
    }
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

    if (!cond->IsNull()) {
        // cond
        gen_expr(&code, cond.get());
        next = code.JumpIfZero(-1);
    }

    // true
    body->Gen(code);

    if (!cond->IsNull()) {
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
    gen_expr(&code, init.get());

    // body
    const Int begin = code.NextAddr();
    body->Gen(code);

    // post
    code.BackPatchContinues();
    gen_expr(&code, post.get());

    // cond
    gen_expr(&code, cond.get());
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
            gen_expr(&code, cond.get());
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
    gen_expr(&code, cond.get());

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
    gen_expr(&code, expr.get());
    code.Return();
}

void ExprStmt::Gen(Bytecode &code) const
{
    gen_expr(&code, expr.get());
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
