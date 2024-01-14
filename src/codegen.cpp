#include "compiler.h"
#include "ast.h"

static bool optimize = false;

void SetOptimize(bool enable)
{
    optimize = enable;
}

#define EMIT(code, ty, op) \
    do { \
    if ((ty)->IsInt() || (ty)->IsBool()) \
        (code)->op##Int(); \
    else if ((ty)->IsFloat()) \
        (code)->op##Float(); \
    } while (0)

#define EMITS(code, ty, op, ops) \
    do { \
    if ((ty)->IsInt() || (ty)->IsBool()) \
        (code)->op##Int(); \
    else if ((ty)->IsFloat()) \
        (code)->op##Float(); \
    else if ((ty)->IsString()) \
        (code)->ops##String(); \
    } while (0)

static void gen_expr(Bytecode *code, const Expr *e);
static void gen_addr(Bytecode *code, const Expr *e);

static void gen_convert(Bytecode *code, TY from, TY to)
{
    switch (from) {
    case TY::BOOL:
        switch (to) {
        case TY::BOOL:  break;
        case TY::INT:   code->BoolToInt(); break;
        case TY::FLOAT: code->BoolToFloat(); break;
        default: break;
        }
        break;

    case TY::INT:
        switch (to) {
        case TY::BOOL:  code->IntToBool(); break;
        case TY::INT:   break;
        case TY::FLOAT: code->IntToFloat(); break;
        default: break;
        }
        break;

    case TY::FLOAT:
        switch (to) {
        case TY::BOOL:  code->FloatToBool(); break;
        case TY::INT:   code->FloatToInt(); break;
        case TY::FLOAT: break;
        default: break;
        }
        break;

    default:
        break;
    }
}

static void gen_call(Bytecode *code, const Expr *e)
{
    // TODO need CallExpr::func?
    const Func *func = e->l->type->func;

    if (func->IsVariadic()) {
        int argc = 0;
        for (const Expr *arg = e->list; arg; arg = arg->next, argc++) {
            // arg value
            gen_expr(code, arg);

            switch (arg->type->kind) {
            case TY::NIL:
                code->LoadTypeNil();
                break;
            case TY::BOOL:
                code->LoadTypeBool();
                break;
            case TY::INT:
                code->LoadTypeInt();
                break;
            case TY::FLOAT:
                code->LoadTypeFloat();
                break;
            case TY::STRING:
                code->LoadTypeString();
                break;
            case TY::CLASS:
            case TY::FUNC:
            case TY::PTR:
            case TY::ARRAY:
            case TY::ANY:
                code->LoadTypeNil();
                break;
            }
        }
        // arg count
        code->LoadByte(argc);
    }
    else {
        for (const Expr *arg = e->list; arg; arg = arg->next)
            gen_expr(code, arg);
    }

    // TODO remove this by doing expr->Gen()
    int addr = 0;
    if (EvalAddr(e->l, &addr)) {
        code->CallFunction(addr, func->IsBuiltin());
    }
}

static void gen_logor(Bytecode *code, const Expr *e)
{
    Int ELSE = 0;
    Int EXIT = 0;

    // eval
    gen_expr(code, e->l);
    ELSE = code->JumpIfZero(-1);

    // true
    code->LoadByte(1);
    EXIT = code->Jump(-1);

    // false
    code->BackPatch(ELSE);
    gen_expr(code, e->r);
    code->BackPatch(EXIT);
}

static void gen_logand(Bytecode *code, const Expr *e)
{
    Int ELSE = 0;
    Int EXIT = 0;

    // eval
    gen_expr(code, e->l);
    ELSE = code->JumpIfZero(-1);

    // true
    gen_expr(code, e->r);
    EXIT = code->Jump(-1);

    // false
    code->BackPatch(ELSE);
    code->LoadByte(0);
    code->BackPatch(EXIT);
}

static void gen_assign(Bytecode *code, const Expr *e)
{
    if (e->kind == T_ASSN) {
        // rval first
        gen_expr(code, e->r);
    }
    else {
        gen_expr(code, e->l);
        gen_expr(code, e->r);

        switch (e->kind) {
        case T_AADD:
            EMITS(code, e->type, Add, Concat);
            break;

        case T_ASUB:
            EMIT(code, e->type, Sub);
            break;

        case T_AMUL:
            EMIT(code, e->type, Mul);
            break;

        case T_ADIV:
            EMIT(code, e->type, Div);
            break;

        case T_AREM:
            EMIT(code, e->type, Rem);
            break;
        }
    }

    //if (optimize) {
    //    int addr = 0;
    //    const bool isconst = lval->EvalAddr(addr);
    //    if (isconst) {
    //        if (lval->IsGlobal())
    //            code.StoreGlobal(addr);
    //        else
    //            code.StoreLocal(addr);
    //        return;
    //    }
    //}
    gen_addr(code, e->l);
    code->Store();
}

static void gen_expr(Bytecode *code, const Expr *e)
{
    switch (e->kind) {

    case T_NILLIT:
        code->LoadByte(0);
        return;

    case T_BOLLIT:
        code->LoadByte(e->val.i);
        return;

    case T_INTLIT:
        if (e->val.i >= 0 && e->val.i <= UINT8_MAX)
            code->LoadByte(e->val.i);
        else
            code->LoadInt(e->val.i);
        return;

    case T_FLTLIT:
        code->LoadFloat(e->val.f);
        return;

    case T_STRLIT:
        {
            // TODO remove string_view
            std::string_view s;

            if (!e->converted)
                s = e->val.s;
            else
                s = std::string_view(e->converted, strlen(e->converted));

            const Word id = code->RegisterConstString(s);
            code->LoadString(id);
        }
        return;

    case T_CONV:
        gen_expr(code, e->l);
        gen_convert(code, e->l->type->kind, e->type->kind);
        return;

    case T_IDENT:
        if (e->var->is_global)
            code->LoadGlobal(e->var->id);
        else
            code->LoadLocal(e->var->id);
        return;

    case T_SELECT:
        gen_addr(code, e);
        code->Load();
        return;

    case T_INDEX:
        gen_addr(code, e);
        code->Load();
        return;

    case T_CALL:
        gen_call(code, e);
        return;

    case T_LOR:
        gen_logor(code, e);
        return;

    case T_LAND:
        gen_logand(code, e);
        return;

    // TODO binary op
    //if (optimize) {
    //    long val = 0;
    //    bool ok;
    //    ok = Eval(val);
    //    if (ok) {
    //        code.LoadInt(val);
    //        return;
    //    }
    //}
    case T_ADD:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->type, Add, Concat);
        return;

    case T_SUB:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Sub);
        return;

    case T_MUL:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Mul);
        return;

    case T_DIV:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Div);
        return;

    case T_REM:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Rem);
        return;

    case T_EQ:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->l->type, Equal, Equal);
        return;

    case T_NEQ:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->l->type, NotEqual, NotEqual);
        return;

    case T_LT:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, Less);
        return;

    case T_LTE:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, LessEqual);
        return;

    case T_GT:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, Greater);
        return;

    case T_GTE:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, GreaterEqual);
        return;

    case T_AND:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        code->And();
        return;

    case T_OR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        code->Or();
        return;

    case T_XOR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        code->Xor();
        return;

    case T_SHL:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        code->ShiftLeft();
        return;

    case T_SHR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        code->ShiftRight();
        return;

    case T_ADR:
        code->LoadAddress(Addr(e->l));
        return;

    case T_POS:
        gen_expr(code, e->l);
        return;

    case T_NEG:
        gen_expr(code, e->l);
        EMIT(code, e->type, Negate);
        return;

    case T_LNOT:
        gen_expr(code, e->l);
        code->SetIfZero();
        return;

    case T_NOT:
        gen_expr(code, e->l);
        code->Not();
        return;

    case T_DRF:
        gen_expr(code, e->l);
        code->Dereference();
        return;

    case T_ASSN:
    case T_AADD: case T_ASUB:
    case T_AMUL: case T_ADIV: case T_AREM:
        gen_assign(code, e);
        return;

    case T_INC:
        if (IsGlobal(e->l))
            code->IncGlobal(Addr(e->l));
        else
            code->IncLocal(Addr(e->l));
        return;

    case T_DEC:
        if (IsGlobal(e->l))
            code->DecGlobal(Addr(e->l));
        else
            code->DecLocal(Addr(e->l));
        return;
    }
}

static void gen_addr(Bytecode *code, const Expr *e)
{
    switch (e->kind) {

    case T_IDENT:
        if (e->var->is_global)
            code->LoadByte(e->var->id + 1);
        else
            code->LoadAddress(e->var->id);
        return;

    case T_FIELD:
        code->LoadByte(e->field->id);
        return;

    case T_SELECT:
        //if (optimize) {
        //    int base = 0;
        //    int offset = 0;
        //    if (inst->EvalAddr(base) && fld->EvalAddr(offset)) {
        //        if (inst->IsGlobal())
        //            code.LoadInt(base + offset + 1);
        //        else
        //            code.LoadAddress(base + offset);
        //        return;
        //    }
        //}
        gen_addr(code, e->l);
        gen_addr(code, e->r);
        code->AddInt();
        return;

    case T_INDEX:
        //if (optimize) {
        //    int base = 0;
        //    long index = 0;
        //    if (ary->EvalAddr(base) && idx->Eval(index)) {
        //        if (ary->IsGlobal())
        //            code.LoadInt(base + index + 1);
        //        else
        //            // index from next to base
        //            code.LoadAddress(base + index + 1);
        //        return;
        //    }
        //}
        gen_addr(code, e->l);
        gen_expr(code, e->r);
        code->Index();
        return;

    case T_DRF:
        // deref *i = ...
        gen_expr(code, e->l);
        return;

    }
}

static void gen_stmt(Bytecode *code, const Stmt *s)
{
    if (!s)
        return;

    switch (s->kind) {

    case T_NOP:
        return;

    case T_BLOCK:
        for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);
        return;

    case T_ELS:
        {
            Int next = 0;

            if (!IsNull(s->cond)) {
                // cond
                gen_expr(code, s->cond);
                next = code->JumpIfZero(-1);
            }

            // true
            gen_stmt(code, s->body);

            if (!IsNull(s->cond)) {
                // close
                const Int addr = code->Jump(-1);
                code->PushOrClose(addr);
                code->BackPatch(next);
            }
        }
        return;

    case T_IF:
        code->BeginIf();

        for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);

        // exit
        code->BackPatchOrCloses();
        return;

    case T_FOR:
        {
            // init
            code->BeginFor();
            gen_expr(code, s->expr);

            // FIXME cond first??
            // body
            const Int begin = code->NextAddr();
            gen_stmt(code, s->body);

            // post
            code->BackPatchContinues();
            gen_expr(code, s->post);

            // cond
            gen_expr(code, s->cond);
            const Int exit = code->JumpIfZero(-1);
            code->Jump(begin);

            // exit
            code->BackPatch(exit);
            code->BackPatchBreaks();
        }
        return;

    case T_BRK:
        {
            const Int addr = code->Jump(-1);
            code->PushBreak(addr);
        }
        return;

    case T_CNT:
        {
            const Int addr = code->Jump(-1);
            code->PushContinue(addr);
        }
        return;

    case T_CASE:
        {
            Int exit = 0;

            std::vector<Int> trues;
            // eval conds
            //for (auto &cond: s->conds) {
            for (Stmt *cond = s->children; cond; cond = cond->next) {
                Int tru = 0;
                Int fls = 0;
                code->DuplicateTop();
                //gen_expr(code, cond);
                gen_stmt(code, cond);
                code->EqualInt();
                fls = code->JumpIfZero(-1);
                tru = code->Jump(-1);
                code->BackPatch(fls);
                trues.push_back(tru);
            }
            // all conds false -> close case
            exit = code->Jump(-1);
            // one of cond true -> go to body
            for (auto t: trues)
                code->BackPatch(t);

            // body
            gen_stmt(code, s->body);

            // close
            const Int addr = code->Jump(-1);
            code->PushCaseClose(addr);
            code->BackPatch(exit);
        }
        return;

    case T_DFLT:
        // body
        gen_stmt(code, s->body);
        return;

    case T_SWT:
        // init
        code->BeginSwitch();
        gen_expr(code, s->cond);

        // cases
        for (Stmt *cas = s->children; cas; cas = cas->next)
            gen_stmt(code, cas);

        // quit
        code->BackPatchCaseCloses();
        // remove cond val
        code->Pop();
        return;

    case T_RET:
        gen_expr(code, s->expr);
        code->Return();
        return;

    case T_EXPR:
        gen_expr(code, s->expr);
        return;
    }
}

static void gen_funcdef(Bytecode *code, const FuncDef *f)
{
    code->RegisterFunction(f->funclit_id, f->func->ParamCount());

    // local vars
    code->Allocate(f->func->scope->TotalVarSize());

    gen_stmt(code, f->body);
}

static void gen_prog(Bytecode *code, const Prog *p)
{
    if (!p->main_func) {
        std::cerr << "'main' function not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    // global vars
    code->Allocate(p->scope->VarSize());
    for (const Stmt *gvar = p->gvars; gvar; gvar = gvar->next)
        gen_stmt(code, gvar);

    // call main
    code->CallFunction(p->main_func->id, p->main_func->type->func->IsBuiltin());
    code->Exit();

    // global funcs
    for (const FuncDef *func = p->funcs; func; func = func->next)
        gen_funcdef(code, func);
}

void GenerateCode(Bytecode *code, const Prog *prog)
{
    gen_prog(code, prog);
    code->End();
}
