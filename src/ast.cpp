#include "ast.h"
#include "escseq.h"
#include <iostream>
#include <limits>

#define EMIT(code, ty, op) \
    do { \
    if ((ty)->IsInteger() || (ty)->IsBool()) \
        (code).op##Int(); \
    else if ((ty)->IsFloat()) \
        (code).op##Float(); \
    } while (0)

#define EMITS(code, ty, op, ops) \
    do { \
    if ((ty)->IsInteger() || (ty)->IsBool()) \
        (code).op##Int(); \
    else if ((ty)->IsFloat()) \
        (code).op##Float(); \
    else if ((ty)->IsString()) \
        (code).ops##String(); \
    } while (0)

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

NullExpr::NullExpr()
    : Expr(new Type(TY::Integer))
{
}

BinaryExpr::BinaryExpr(TokenKind Kind, Expr *L, Expr *R)
    : Expr(PromoteType(L->type, R->type)), kind(Kind), l(L), r(R)
{
}

UnaryExpr::UnaryExpr(TokenKind Kind, Expr *R)
    : Expr(R->type), kind(Kind), r(R)
{
}

int StringLitExpr::ConvertEscSeq()
{
    return ConvertEscapeSequence(sval, converted);
}

// Print
void ConstExpr::Print(int depth) const
{
    print_node("ConstExpr", depth, false);
    std::cout << bval <<
        " " << type->kind << std::endl;
}

void IntNumExpr::Print(int depth) const
{
    print_node("IntNumExpr", depth, false);
    std::cout << ival <<
        " " << type->kind << std::endl;
}

void FpNumExpr::Print(int depth) const
{
    print_node("FpNumExpr", depth, false);
    std::cout << fval <<
        " " << type->kind << std::endl;
}

void StringLitExpr::Print(int depth) const
{
    print_node("StringLitExpr", depth, false);
    std::cout << "\"" << sval << "\"" <<
        " " << type->kind << std::endl;
}

void IdentExpr::Print(int depth) const
{
    print_node("IdentExpr", depth, false);
    std::cout << "\"" << var->name << "\" @" << var->id <<
        " " << type->kind << std::endl;
}

void FieldExpr::Print(int depth) const
{
    print_node("FieldExpr", depth, false);
    std::cout << fld->name << " @" << fld->id <<
        " " << type->kind << std::endl;
}

void SelectExpr::Print(int depth) const
{
    print_node("SelectExpr", depth);
    inst->Print(depth + 1);
    fld->Print(depth + 1);
}

void CallExpr::Print(int depth) const
{
    print_node("CallExpr", depth, false);
    std::cout << func->name << std::endl;
    for (auto arg: args)
        arg->Print(depth + 1);
}

void BinaryExpr::Print(int depth) const
{
    print_node("BinaryExpr", depth, false);
    std::cout << "\"" << kind << "\" ";
    std::cout << type->kind << std::endl;

    l->Print(depth + 1);
    r->Print(depth + 1);
}

void UnaryExpr::Print(int depth) const
{
    print_node("UnaryExpr", depth, false);
    std::cout << "\"" << kind << "\" ";
    std::cout << type->kind << std::endl;

    r->Print(depth + 1);
}

void AssignExpr::Print(int depth) const
{
    print_node("AssignExpr", depth, false);
    std::cout << "\"" << kind << "\" ";
    std::cout << type->kind << std::endl;

    lval->Print(depth + 1);
    rval->Print(depth + 1);
}

void IncDecExpr::Print(int depth) const
{
    print_node("IncDecExpr", depth, false);
    std::cout << "\"" << kind << "\" ";
    std::cout << type->kind << std::endl;

    lval->Print(depth + 1);
}

void BlockStmt::Print(int depth) const
{
    print_node("BlockStmt", depth);
    for (const auto &stmt: stmts)
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
    expr->Print(depth + 1);
    block->Print(depth + 1);
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
    std::cout << func->name << std::endl;
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

// Addr
int IdentExpr::Addr() const
{
    return var->id;
}

// Gen
void ConstExpr::Gen(Bytecode &code) const
{
    if (type->IsBool()) {
        code.LoadByte(bval);
    }
}

void IntNumExpr::Gen(Bytecode &code) const
{
    constexpr Int bytemin = std::numeric_limits<Byte>::min();
    constexpr Int bytemax = std::numeric_limits<Byte>::max();

    if (ival >= bytemin && ival <= bytemax)
        code.LoadByte(ival);
    else
        code.LoadInt(ival);
}

void FpNumExpr::Gen(Bytecode &code) const
{
    code.LoadFloat(fval);
}

void StringLitExpr::Gen(Bytecode &code) const
{
    std::string_view s;

    if (converted.empty())
        s = sval;
    else
        s = std::string_view(converted.c_str(), converted.length());

    const Word id = code.RegisterConstString(s);
    code.LoadString(id);
}

void IdentExpr::Gen(Bytecode &code) const
{
    if (var->is_global)
        code.LoadGlobal(var->id);
    else
        code.LoadLocal(var->id);
}

void FieldExpr::Gen(Bytecode &code) const
{
    //code.LoadLocal(fld->id);
}

void SelectExpr::Gen(Bytecode &code) const
{
    const int index = Addr();
    if (inst->IsGlobal())
        code.LoadGlobal(index);
    else
        code.LoadLocal(index);
}

void CallExpr::Gen(Bytecode &code) const
{
    for (auto arg: args)
        arg->Gen(code);
    code.CallFunction(func->id, func->is_builtin);
}

void BinaryExpr::Gen(Bytecode &code) const
{
    if (kind == TK::BAR2) {
        Int ELSE = 0;
        Int EXIT = 0;

        // eval
        l->Gen(code);
        ELSE = code.JumpIfZero(-1);

        // true
        code.LoadByte(1);
        EXIT = code.Jump(-1);

        // false
        code.BackPatch(ELSE);
        r->Gen(code);
        code.BackPatch(EXIT);

        return;
    }
    else if (kind == TK::AMP2) {
        Int ELSE = 0;
        Int EXIT = 0;

        // eval
        l->Gen(code);
        ELSE = code.JumpIfZero(-1);

        // true
        r->Gen(code);
        EXIT = code.Jump(-1);

        // false
        code.BackPatch(ELSE);
        code.LoadByte(0);
        code.BackPatch(EXIT);

        return;
    }

    l->Gen(code);
    r->Gen(code);

    switch (kind) {
    case TK::PLUS:
        EMITS(code, type, Add, Concat);
        return;

    case TK::MINUS:
        EMIT(code, type, Sub);
        return;

    case TK::STAR:
        EMIT(code, type, Mul);
        return;

    case TK::SLASH:
        EMIT(code, type, Div);
        return;

    case TK::PERCENT:
        EMIT(code, type, Rem);
        return;

    case TK::EQ2:
        EMITS(code, l->type, Equal, Equal);
        return;

    case TK::EXCLEQ:
        EMITS(code, l->type, NotEqual, NotEqual);
        return;

    case TK::LT:
        EMIT(code, l->type, Less);
        return;

    case TK::LTE:
        EMIT(code, l->type, LessEqual);
        return;

    case TK::GT:
        EMIT(code, l->type, Greater);
        return;

    case TK::GTE:
        EMIT(code, l->type, GreaterEqual);
        return;

    case TK::AMP:
        code.And();
        return;

    case TK::BAR:
        code.Or();
        return;

    case TK::CARET:
        code.Xor();
        return;

    case TK::LT2:
        code.ShiftLeft();
        return;

    case TK::GT2:
        code.ShiftRight();
        return;

    default:
        return;
    }
}

void UnaryExpr::Gen(Bytecode &code) const
{
    r->Gen(code);

    switch (kind) {
    case TK::PLUS:
        return;

    case TK::MINUS:
        EMIT(code, type, Negate);
        return;

    case TK::EXCL:
        code.SetIfZero();
        return;

    case TK::TILDA:
        code.Not();
        return;

    default:
        return;
    }
}

void AssignExpr::Gen(Bytecode &code) const
{
    if (kind == TK::EQ) {
        // rval first
        rval->Gen(code);
    }
    else {
        lval->Gen(code);
        rval->Gen(code);

        switch (kind) {
        case TK::PLUSEQ:
            EMITS(code, type, Add, Concat);
            break;;

        case TK::MINUSEQ:
            EMIT(code, type, Sub);
            break;;

        case TK::STAREQ:
            EMIT(code, type, Mul);
            break;;

        case TK::SLASHEQ:
            EMIT(code, type, Div);
            break;;

        case TK::PERCENTEQ:
            EMIT(code, type, Rem);
            break;;

        default:
            break;;
        }
    }

    const int index = lval->Addr();
    if (lval->IsGlobal())
        code.StoreGlobal(index);
    else
        code.StoreLocal(index);
}

void IncDecExpr::Gen(Bytecode &code) const
{
    const int index = lval->Addr();

    switch (kind) {
    case TK::PLUS2:
        if (lval->IsGlobal())
            code.IncGlobal(index);
        else
            code.IncLocal(index);
        return;

    case TK::MINUS2:
        if (lval->IsGlobal())
            code.DecGlobal(index);
        else
            code.DecLocal(index);
        return;

    default:
        return;
    }
}

void BlockStmt::Gen(Bytecode &code) const
{
    for (const auto &stmt: stmts)
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

void ForStmt::Gen(Bytecode &code) const
{
    // init
    code.BeginFor();
    init->Gen(code);

    // body
    const Int begin = code.NextAddr();
    body->Gen(code);

    // post
    code.BackPatchContinues();
    post->Gen(code);

    // cond
    cond->Gen(code);
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
    Int next = 0;

    if (kind == TK::CASE) {
        // expr
        code.DuplicateTop();
        expr->Gen(code);
        code.EqualInt();
        next = code.JumpIfZero(-1);
    }

    // body
    block->Gen(code);

    if (kind == TK::CASE) {
        // close
        const Int addr = code.Jump(-1);
        code.PushCaseCloses(addr);
        code.BackPatch(next);
    }
}

void SwitchStmt::Gen(Bytecode &code) const
{
    // init
    code.BeginSwitch();
    cond->Gen(code);

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
        code.Allocate(func->VarCount());

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
        code.Allocate(scope->VarSize());
    for (const auto &gvar: gvars)
        gvar->Gen(code);

    // call main
    code.CallFunction(main_func->id, main_func->is_builtin);
    code.Exit();

    // global funcs
    for (const auto &func: funcs)
        func->Gen(code);
}
