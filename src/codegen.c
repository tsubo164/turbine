#include "codegen.h"
#include "bytecode.h"
#include "error.h"
#include "ast.h"
#include <stdlib.h>
#include <stdio.h>

typedef struct IntVec {
    Int *data;
    int cap;
    int len;
} IntVec;

static int new_cap(int cur_cap, int min_cap)
{
    return cur_cap < min_cap ? min_cap : cur_cap * 2;
}

static void push_int(IntVec *v, Int data)
{
    if (v->len + 1 > v->cap) {
        v->cap = new_cap(v->cap, 16);
        // TODO Remove cast
        v->data = (Int *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = data;
}

static bool optimize = false;

void SetOptimize(bool enable)
{
    optimize = enable;
}

#define EMIT(code, ty, op) \
    do { \
    if (IsInt((ty)) || IsBool((ty))) \
        op##Int((code)); \
    else if (IsFloat((ty))) \
        op##Float((code)); \
    } while (0)

#define EMITS(code, ty, op, ops) \
    do { \
    if (IsInt((ty)) || IsBool((ty))) \
        op##Int((code)); \
    else if (IsFloat((ty))) \
        op##Float((code)); \
    else if (IsString((ty))) \
        ops##String((code)); \
    } while (0)

static void gen_expr(Bytecode *code, const Expr *e);
static void gen_addr(Bytecode *code, const Expr *e);

static void gen_convert(Bytecode *code, enum TY from, enum TY to)
{
    switch (from) {
    case TY_BOOL:
        switch (to) {
        case TY_BOOL:  break;
        case TY_INT:   BoolToInt(code); break;
        case TY_FLOAT: BoolToFloat(code); break;
        default: break;
        }
        break;

    case TY_INT:
        switch (to) {
        case TY_BOOL:  IntToBool(code); break;
        case TY_INT:   break;
        case TY_FLOAT: IntToFloat(code); break;
        default: break;
        }
        break;

    case TY_FLOAT:
        switch (to) {
        case TY_BOOL:  FloatToBool(code); break;
        case TY_INT:   FloatToInt(code); break;
        case TY_FLOAT: break;
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

    if (IsVariadic(func)) {
        int argc = 0;
        for (const Expr *arg = e->list; arg; arg = arg->next, argc++) {
            // arg value
            gen_expr(code, arg);

            switch (arg->type->kind) {
            case TY_NIL:
                LoadTypeNil(code);
                break;
            case TY_BOOL:
                LoadTypeBool(code);
                break;
            case TY_INT:
                LoadTypeInt(code);
                break;
            case TY_FLOAT:
                LoadTypeFloat(code);
                break;
            case TY_STRING:
                LoadTypeString(code);
                break;
            case TY_FUNC:
            case TY_CLASS:
            case TY_TABLE:
            case TY_MODULE:
            case TY_PTR:
            case TY_ARRAY:
            case TY_ANY:
                LoadTypeNil(code);
                break;
            }
        }
        // arg count
        LoadByte(code, argc);
    }
    else {
        for (const Expr *arg = e->list; arg; arg = arg->next)
            gen_expr(code, arg);
    }

    // TODO remove this by doing expr->Gen()
    int addr = 0;
    if (EvalAddr(e->l, &addr)) {
        CallFunction(code, addr, IsBuiltin(func));
    }
}

static void gen_logor(Bytecode *code, const Expr *e)
{
    Int ELSE = 0;
    Int EXIT = 0;

    // eval
    gen_expr(code, e->l);
    ELSE = JumpIfZero(code, -1);

    // true
    LoadByte(code, 1);
    EXIT = Jump(code, -1);

    // false
    BackPatch(code, ELSE);
    gen_expr(code, e->r);
    BackPatch(code, EXIT);
}

static void gen_logand(Bytecode *code, const Expr *e)
{
    Int ELSE = 0;
    Int EXIT = 0;

    // eval
    gen_expr(code, e->l);
    ELSE = JumpIfZero(code, -1);

    // true
    gen_expr(code, e->r);
    EXIT = Jump(code, -1);

    // false
    BackPatch(code, ELSE);
    LoadByte(code, 0);
    BackPatch(code, EXIT);
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
    Store(code);
}

static void gen_expr(Bytecode *code, const Expr *e)
{
    switch (e->kind) {

    case T_NILLIT:
        LoadByte(code, 0);
        return;

    case T_BOLLIT:
        LoadByte(code, e->ival);
        return;

    case T_INTLIT:
        if (e->ival >= 0 && e->ival <= UINT8_MAX)
            LoadByte(code, e->ival);
        else
            LoadInt(code, e->ival);
        return;

    case T_FLTLIT:
        LoadFloat(code, e->fval);
        return;

    case T_STRLIT:
        {
            const char *s;

            // TODO could remove e->converted
            if (!e->converted)
                s = e->sval;
            else
                s = e->converted;

            const Word id = RegisterConstString(code, s);
            LoadString(code, id);
        }
        return;

    case T_CONV:
        gen_expr(code, e->l);
        gen_convert(code, e->l->type->kind, e->type->kind);
        return;

    case T_IDENT:
        if (e->var->is_global)
            LoadGlobal(code, e->var->id);
        else
            LoadLocal(code, e->var->id);
        return;

    case T_SELECT:
        gen_addr(code, e);
        Load(code);
        return;

    case T_INDEX:
        gen_addr(code, e);
        Load(code);
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
        And(code);
        return;

    case T_OR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        Or(code);
        return;

    case T_XOR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        Xor(code);
        return;

    case T_SHL:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        ShiftLeft(code);
        return;

    case T_SHR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        ShiftRight(code);
        return;

    case T_ADR:
        LoadAddress(code, Addr(e->l));
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
        SetIfZero(code);
        return;

    case T_NOT:
        gen_expr(code, e->l);
        Not(code);
        return;

    case T_DRF:
        gen_expr(code, e->l);
        Dereference(code);
        return;

    case T_ASSN:
    case T_AADD: case T_ASUB:
    case T_AMUL: case T_ADIV: case T_AREM:
        gen_assign(code, e);
        return;

    case T_INC:
        if (IsGlobal(e->l))
            IncGlobal(code, Addr(e->l));
        else
            IncLocal(code, Addr(e->l));
        return;

    case T_DEC:
        if (IsGlobal(e->l))
            DecGlobal(code, Addr(e->l));
        else
            DecLocal(code, Addr(e->l));
        return;
    }
}

static void gen_addr(Bytecode *code, const Expr *e)
{
    switch (e->kind) {

    case T_IDENT:
        if (e->var->is_global)
            LoadByte(code, e->var->id + 1);
        else
            LoadAddress(code, e->var->id);
        return;

    case T_FIELD:
        LoadByte(code, e->field->id);
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
        AddInt(code);
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
        Index(code);
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
                next = JumpIfZero(code, -1);
            }

            // true
            gen_stmt(code, s->body);

            if (!IsNull(s->cond)) {
                // close
                const Int addr = Jump(code, -1);
                PushOrClose(code, addr);
                BackPatch(code, next);
            }
        }
        return;

    case T_IF:
        BeginIf(code);

        for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);

        // exit
        BackPatchOrCloses(code);
        return;

    case T_FOR:
        {
            // init
            BeginFor(code);
            gen_expr(code, s->expr);

            // FIXME cond first??
            // body
            const Int begin = NextAddr(code);
            gen_stmt(code, s->body);

            // post
            BackPatchContinues(code);
            gen_expr(code, s->post);

            // cond
            gen_expr(code, s->cond);
            const Int exit = JumpIfZero(code, -1);
            Jump(code, begin);

            // exit
            BackPatch(code, exit);
            BackPatchBreaks(code);
        }
        return;

    case T_BRK:
        {
            const Int addr = Jump(code, -1);
            PushBreak(code, addr);
        }
        return;

    case T_CNT:
        {
            const Int addr = Jump(code, -1);
            PushContinue(code, addr);
        }
        return;

    case T_CASE:
        {
            Int exit = 0;

            IntVec trues = {0};
            // eval conds
            for (Stmt *cond = s->children; cond; cond = cond->next) {
                Int tru = 0;
                Int fls = 0;
                DuplicateTop(code);
                //gen_expr(code, cond);
                gen_stmt(code, cond);
                EqualInt(code);
                fls = JumpIfZero(code, -1);
                tru = Jump(code, -1);
                BackPatch(code, fls);
                push_int(&trues, tru);
            }
            // all conds false -> close case
            exit = Jump(code, -1);
            // one of cond true -> go to body
            for (int i = 0; i < trues.len; i++)
                BackPatch(code, trues.data[i]);
            free(trues.data);

            // body
            gen_stmt(code, s->body);

            // close
            const Int addr = Jump(code, -1);
            PushCaseClose(code, addr);
            BackPatch(code, exit);
        }
        return;

    case T_DFLT:
        // body
        gen_stmt(code, s->body);
        return;

    case T_SWT:
        // init
        BeginSwitch(code);
        gen_expr(code, s->cond);

        // cases
        for (Stmt *cas = s->children; cas; cas = cas->next)
            gen_stmt(code, cas);

        // quit
        BackPatchCaseCloses(code);
        // remove cond val
        Pop(code);
        return;

    case T_RET:
        gen_expr(code, s->expr);
        Return(code);
        return;

    case T_EXPR:
        gen_expr(code, s->expr);
        return;
    }
}

static void gen_funcdef(Bytecode *code, const FuncDef *f)
{
    RegisterFunction(code, f->funclit_id, ParamCount(f->func));

    // local vars
    Allocate(code, TotalVarSize(f->func->scope));

    gen_stmt(code, f->body);
}

static void gen_prog(Bytecode *code, const Prog *p)
{
    if (!p->main_func) {
        fprintf(stderr, "error: 'main' function not found");
    }

    // global vars
    Allocate(code, VarSize(p->scope));
    for (const Stmt *gvar = p->gvars; gvar; gvar = gvar->next)
        gen_stmt(code, gvar);

    // call main
    CallFunction(code, p->main_func->id, IsBuiltin(p->main_func->type->func));
    Exit(code);

    // global funcs
    for (const FuncDef *func = p->funcs; func; func = func->next)
        gen_funcdef(code, func);
}

void GenerateCode(Bytecode *code, const Prog *prog)
{
    gen_prog(code, prog);
    End(code);
}
