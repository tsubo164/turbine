#include "codegen.h"
#include "bytecode.h"
#include "scope.h"
#include "error.h"
#include "type.h"
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

#define BINOP__(code, ty, op, r0, r1, r2) \
    do { \
    if (IsInt((ty)) || IsBool((ty))) \
        op##Int__((code), (r0), (r1), (r2)); \
    else if (IsFloat((ty))) \
        ;/*op##Float__((code));*/ \
    } while (0)

#define BINOP_S__(code, ty, op, ops, r0, r1, r2) \
    do { \
    if (IsInt((ty)) || IsBool((ty))) \
        op##Int__((code), (r0), (r1), (r2)); \
    else if (IsFloat((ty))) \
        ;/*op##Float__((code));*/ \
    else if (IsString((ty))) \
        ops##String__((code), (r0), (r1), (r2)); \
    } while (0)

static void gen_expr(Bytecode *code, const struct Expr *e);
static void gen_addr(Bytecode *code, const struct Expr *e);

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

static void gen_call(Bytecode *code, const struct Expr *call)
{
    const struct FuncType *func_type = call->l->type->func_type;

    if (func_type->is_variadic) {
        int argc = 0;
        for (const struct Expr *arg = call->r; arg; arg = arg->next, argc++) {
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
            case TY_STRUCT:
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
        for (const struct Expr *arg = call->r; arg; arg = arg->next)
            gen_expr(code, arg);
    }

    int64_t func_id = 0;
    if (EvalExpr(call->l, &func_id)) {
        CallFunction(code, func_id, func_type->is_builtin);
    }
    else {
        gen_expr(code, call->l);
        CallFunctionPointer(code);
    }
}

static void gen_logor(Bytecode *code, const struct Expr *e)
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

static void gen_logand(Bytecode *code, const struct Expr *e)
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

/*static*/ void gen_store(Bytecode *code, const struct Expr *l, int offset)
{
    // lval
    int addr = 0;
    const bool isconst = EvalAddr(l, &addr);

    // store
    if (isconst) {
        if (IsGlobal(l))
            StoreGlobal(code, addr + offset);
        else
            StoreLocal(code, addr + offset);
    }
    else {
        gen_addr(code, l);
        Store(code);
    }
}

static void gen_clear_block(Bytecode *code, const struct Expr *dst)
{
    int dst_addr = 0;
    EvalAddr(dst, &dst_addr);

    if (IsGlobal(dst)) {
        // TODO support variable addresses
        ClearGlobal(code, dst_addr, SizeOf(dst->type));
    }
    else {
        // TODO support variable addresses
        ClearLocal(code, dst_addr, SizeOf(dst->type));
    }
}

static void gen_copy_block(Bytecode *code,
        const struct Expr *src, const struct Expr *dst)
{
    if (IsGlobal(src)) {
        int src_addr = 0;
        int dst_addr = 0;

        EvalAddr(src, &src_addr);
        EvalAddr(dst, &dst_addr);
        // TODO support variable addresses
        CopyGlobal(code, src_addr, dst_addr, SizeOf(src->type));
    }
    else {
        int src_addr = 0;
        int dst_addr = 0;

        EvalAddr(src, &src_addr);
        EvalAddr(dst, &dst_addr);
        // TODO support variable addresses
        CopyLocal(code, src_addr, dst_addr, SizeOf(src->type));
    }
}


static void gen_assign(Bytecode *code, const struct Expr *e)
{
    if (IsStruct(e->type)) {
        gen_copy_block(code, e->r, e->l);
        return;
    }

    // rval first
    if (e->kind == T_ASSN) {
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

    // lval
    int addr = 0;
    const bool isconst = EvalAddr(e->l, &addr);

    // store
    if (isconst) {
        if (IsGlobal(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
    }
    else {
        gen_addr(code, e->l);
        Store(code);
    }
}

static void gen_init_array(Bytecode *code, const struct Expr *e)
{
    // TODO testing dynamic array
    {
        // lval
        int addr = 0;
        // an init expr always has identifier on the left
        EvalAddr(e->l, &addr);

        LoadInt(code, e->type->len);
        ArrayLocal(code, addr);
        StoreLocal(code, addr);

        //return;
    }
    // TODO =====================

    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    EvalAddr(e->l, &addr);

    // array len
    LoadInt(code, e->type->len);
    if (IsGlobal(e->l))
        StoreGlobal(code, addr);
    else
        StoreLocal(code, addr);
    addr++;

    // array lit
    struct Expr *array_lit = e->r;

    for (struct Expr *expr = array_lit->l; expr; expr = expr->next) {
        // rval
        gen_expr(code, expr);

        // store
        if (IsGlobal(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
        addr++;
    }
}

static void gen_init_struct(Bytecode *code, const struct Expr *e)
{
    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    EvalAddr(e->l, &addr);

    if (e->r && e->r->kind == T_NILLIT) {
        // no initializer
        // clear zero
        gen_clear_block(code, e->l);
        return;
    }

    if (e->r && e->r->kind != T_STRUCTLIT) {
        // initialized by another object
        gen_copy_block(code, e->r, e->l);
        return;
    }
    // struct literal initializer

    // clear zero
    gen_clear_block(code, e->l);

    // struct lit
    struct Expr *struct_lit = e->r;

    for (struct Expr *elem = struct_lit->l; elem; elem = elem->next) {
        // rval
        gen_expr(code, elem->r);

        // lval
        int offset = 0;
        EvalAddr(elem->l, &offset);

        // store
        if (IsGlobal(e->l))
            StoreGlobal(code, addr + offset);
        else
            StoreLocal(code, addr + offset);
    }
}

static void gen_init(Bytecode *code, const struct Expr *e)
{
    // rval
    gen_expr(code, e->r);

    // lval
    int addr = 0;
    const bool isconst = EvalAddr(e->l, &addr);

    // store
    if (isconst) {
        if (IsGlobal(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
    }
    else {
        gen_addr(code, e->l);
        Store(code);
    }
}

static void gen_expr(Bytecode *code, const struct Expr *e)
{
    if (!e)
        return;

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

    case T_FUNCLIT:
        LoadInt(code, e->func->id);
        return;

    case T_CONV:
        gen_expr(code, e->l);
        gen_convert(code, e->l->type->kind, e->type->kind);
        return;

    case T_IDENT:
        if (IsStruct(e->type)) {
            gen_addr(code, e);
            return;
        }

        if (e->var->is_global)
            LoadGlobal(code, e->var->offset);
        else
            LoadLocal(code, e->var->offset);
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

    case T_INIT:
        if (IsArray(e->type))
            gen_init_array(code, e);
        else if (IsStruct(e->type))
            gen_init_struct(code, e);
        else
            gen_init(code, e);
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

static void gen_addr(Bytecode *code, const struct Expr *e)
{
    if (!e)
        return;

    switch (e->kind) {

    case T_IDENT:
        if (IsPtr(e->type)) {
            gen_expr(code, e);
            return;
        }

        if (IsStruct(e->type)) {
            if (e->var->is_param) {
                LoadAddress(code, e->var->offset);
                Dereference(code);
            }
            else if (e->var->is_global) {
                LoadInt(code, e->var->offset + 1);
            }
            else {
                LoadAddress(code, e->var->offset);
            }
            return;
        }

        if (e->var->is_global)
            LoadByte(code, e->var->offset + 1);
        else
            LoadAddress(code, e->var->offset);
        return;

    case T_FIELD:
        LoadByte(code, e->field->offset);
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

static void gen_stmt(Bytecode *code, const struct Stmt *s)
{
    if (!s)
        return;

    switch (s->kind) {

    case T_NOP:
        return;

    case T_BLOCK:
        for (struct Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);
        return;

    case T_ELS:
        {
            Int next = 0;

            if (s->cond) {
                // cond
                gen_expr(code, s->cond);
                next = JumpIfZero(code, -1);
            }

            // true
            gen_stmt(code, s->body);

            if (s->cond) {
                // close
                const Int addr = Jump(code, -1);
                PushOrClose(code, addr);
                BackPatch(code, next);
            }
        }
        return;

    case T_IF:
        BeginIf(code);

        for (struct Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);

        // exit
        BackPatchOrCloses(code);
        return;

    case T_FOR:
        {
            // init
            BeginFor(code);
            gen_stmt(code, s->init);

            // cond
            const Int begin = NextAddr(code);
            gen_expr(code, s->cond);
            const Int exit = JumpIfZero(code, -1);

            // body
            gen_stmt(code, s->body);

            // post
            BackPatchContinues(code);
            gen_stmt(code, s->post);
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
            for (struct Expr *cond = s->cond; cond; cond = cond->next) {
                Int tru = 0;
                Int fls = 0;
                DuplicateTop(code);
                gen_expr(code, cond);
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
        for (struct Stmt *cas = s->children; cas; cas = cas->next)
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
        // remove the result
        Pop(code);
        return;

    case T_ASSN:
    case T_INIT:
        gen_expr(code, s->expr);
        return;
    }
}

static void gen_func(Bytecode *code, const struct Func *func, int func_id)
{
    //BackPatchFuncAddr(code, func->fullname);
    RegisterFunction(code, func_id, func->params.len);

    // local vars
    Allocate(code, func->scope->size);

    gen_stmt(code, func->body);
}

static void gen_gvars(Bytecode *code, const struct Module *mod)
{
    struct Scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_gvars(code, sym->module);
    }

    // self module next
    for (const struct Stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        gen_stmt(code, gvar);
}

static void gen_funcs(Bytecode *code, const struct Module *mod)
{
    struct Scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_funcs(code, sym->module);
    }

    // self module next
    for (int i = 0; i < mod->funcs.len; i++) {
        struct Func *func = mod->funcs.data[i];
        if (!func->is_builtin)
            gen_func(code, func, func->id);
    }
}

static void gen_module(Bytecode *code, const struct Module *mod)
{
    if (!mod->main_func) {
        fprintf(stderr, "error: 'main' function not found");
    }

    // global vars
    Allocate(code, mod->scope->size);
    gen_gvars(code, mod);

    // TODO maybe better to search "main" module and "main" func in there
    // instead of holding main_func
    // call main
    CallFunction(code, mod->main_func->id, mod->main_func->is_builtin);
    Exit(code);

    // global funcs
    gen_funcs(code, mod);
}

/*static*/ void register_funcs(Bytecode *code, const struct Module *mod)
{
    for (int i = 0; i < mod->funcs.len; i++) {
        struct Func *func = mod->funcs.data[i];
        if (!func->is_builtin) {
            func->id = RegisterFunc(code, func->fullname, func->params.len);
        }
    }
}

// XXX TEST ----------------
static void gen_module__(Bytecode *code, const struct Module *mod);
void GenerateCode__(struct Bytecode *code, const struct Module *mod)
{
    gen_module__(code, mod);
    End__(code);
}

void GenerateCode(struct Bytecode *code, const struct Module *mod)
{
    //register_funcs(code, mod);
    // XXX TEST ----------------
    if (REGISTER_MACHINE) {
        GenerateCode__(code, mod);
        return;
    }
    gen_module(code, mod);
    End(code);
}

static int resolve_func_id(struct Scope *scope, int start_id)
{
    int next_id = start_id;

    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_FUNC) {
            if (!sym->func->is_builtin)
                sym->func->id = next_id++;
        }
        else if (sym->kind == SYM_MODULE) {
            next_id = resolve_func_id(sym->module->scope, next_id);
        }
    }

    return next_id;
}

static int max(int a, int b)
{
    return a < b ? b : a;
}

static int resolve_offset(struct Scope *scope, int start_offset)
{
    int cur_offset = start_offset;
    int max_offset = start_offset;
    int cur_size = 0;
    int max_size = 0;

    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_VAR) {
            struct Var *var = sym->var;
            // offset
            var->offset = cur_offset;
            cur_offset += SizeOf(var->type);
            max_offset = max(max_offset, cur_offset);
            // size
            if (!var->is_param)
                cur_size += SizeOf(var->type);
            max_size = max(max_size, cur_size);
        }
        else if (sym->kind == SYM_FUNC) {
            struct Scope *child = sym->func->scope;
            // start over from offset 0
            resolve_offset(child, 0);
        }
        else if (sym->kind == SYM_SCOPE) {
            struct Scope *child = sym->scope;
            int child_max = resolve_offset(child, cur_offset);
            // offset
            max_offset = max(max_offset, child_max);
            // size
            max_size = max(max_size, cur_size + child->size);
        }
        else if (sym->kind == SYM_MODULE) {
            struct Scope *child = sym->module->scope;
            // offset
            cur_offset = resolve_offset(child, cur_offset);
            max_offset = max(max_offset, cur_offset);
            // size
            cur_size += child->size;
            max_size = max(max_size, cur_size);
        }
    }

    scope->size = max_size;
    return max_offset;
}

void ResolveOffset(struct Module *mod)
{
    resolve_offset(mod->scope, 0);
    resolve_func_id(mod->scope, 0);
}

// XXX TEST compiling to register-based machine code
static int gen_expr__(Bytecode *code, const struct Expr *e);
static int gen_addr__(Bytecode *code, const struct Expr *e);

static int gen_init_array__(Bytecode *code, const struct Expr *e)
{
    // TODO testing dynamic array
    {
        // an init expr always has identifier on the left
        int reg0 = gen_addr__(code, e->l);
        int reg1 = gen_expr__(code, e->r);
        NewArray__(code, reg0, reg1);

        return reg0;
    }
    // TODO =====================

    /*
    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    EvalAddr(e->l, &addr);

    // array len
    LoadInt(code, e->type->len);
    if (IsGlobal(e->l))
        StoreGlobal(code, addr);
    else
        StoreLocal(code, addr);
    addr++;

    // array lit
    struct Expr *array_lit = e->r;

    for (struct Expr *expr = array_lit->l; expr; expr = expr->next) {
        // rval
        gen_expr(code, expr);

        // store
        if (IsGlobal(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
        addr++;
    }
    */
}

static int gen_init__(Bytecode *code, const struct Expr *e)
{
    // rval
    int reg0 = 0xff;
    int reg1 = gen_expr__(code, e->r);

    if (IsGlobal(e->l)) {
        reg0 = gen_addr__(code, e->l);
        Store__(code, reg0, reg1);
    }
    else {
        // TODO handle case where lhs is not addressable
        reg0 = gen_addr__(code, e->l);
        Move__(code, reg0, reg1);
    }

        /*
    // store
    if (isconst) {
        if (IsGlobal(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
    }
    else {
        gen_addr(code, e->l);
        Store(code);
    }
        */
    return 0;
}

static int gen_dst_register(Bytecode *code, int reg1, int reg2)
{
    int reg0 = -1;

    // determine the destination register
    if (IsTempRegister(code, reg1))
        reg0 = reg1;
    else if (IsTempRegister(code, reg2))
        reg0 = reg2;
    else
        reg0 = NewRegister__(code);

    return reg0;
}

static int gen_binop__(Bytecode *code, const struct Type *type, int kind,
        int reg0, int reg1, int reg2)
{
    switch (kind) {
    case T_ADD:
    case T_AADD:
        BINOP_S__(code, type, Add, Concat, reg0, reg1, reg2);
        break;

    case T_REM:
    case T_AREM:
        BINOP__(code, type, Rem, reg0, reg1, reg2);
        break;
    }
    return reg0;
}

static int gen_assign__(Bytecode *code, const struct Expr *e)
{
    const struct Expr *lval = e->l;
    const struct Expr *rval = e->r;
    int reg0 = -1;
    int reg1 = -1;
    int reg2 = -1;

    if (lval->kind == T_INDEX) {
        reg0 = gen_addr__(code, lval->l);
        reg1 = gen_expr__(code, lval->r);
        reg2 = gen_expr__(code, rval);
        StoreArray__(code, reg0, reg1, reg2);
        return reg0;
    }

    // check the rvalue expression to see if binop r0, r1, r2 can be applied
    // e.g. a = b + c
    //            ^ here
    switch (rval->kind) {
    case T_ADD:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP_S__(code, e->type, Add, Concat, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    case T_REM:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP__(code, e->type, Rem, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    case T_LT:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP__(code, e->type, Less, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    default:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval);
        reg0 = Move__(code, reg0, reg1);
        break;
    }

    return reg0;
}

static int gen_binop_assign__(Bytecode *code, const struct Expr *e)
{
    if (e->l->kind == T_INDEX) {
        // lval
        int reg0 = gen_addr__(code, e->l->l);
        int reg1 = gen_expr__(code, e->l->r);
        // rval
        int tmp1 = gen_expr__(code, e->l);
        int tmp2 = gen_expr__(code, e->r);
        // binop
        int reg2 = gen_dst_register(code, tmp1, tmp2);
        gen_binop__(code, e->type, e->kind, reg2, tmp1, tmp2);
        // store
        StoreArray__(code, reg0, reg1, reg2);
        return reg0;
    }
    else {
        // primitives
        int reg0 = reg0 = gen_addr__(code, e->l);
        int reg1 = reg1 = gen_expr__(code, e->l);
        int reg2 = reg2 = gen_expr__(code, e->r);

        return gen_binop__(code, e->type, e->kind, reg0, reg1, reg2);
    }
}

static int gen_call__(Bytecode *code, const struct Expr *call)
{
    const struct FuncType *func_type = call->l->type->func_type;

    if (func_type->is_variadic) {
        /*
        int argc = 0;
        for (const struct Expr *arg = call->r; arg; arg = arg->next, argc++) {
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
            case TY_STRUCT:
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
        */
    }
    else {
        for (const struct Expr *arg = call->r; arg; arg = arg->next) {
            int cur = GetCurrentRegister__(code);
            int src = gen_expr__(code, arg);
            int dst = GetNextRegister__(code, cur);
            Move__(code, dst, src);
        }
    }

    // Get the returned value register right next to current
    //int retval_reg = NewRegister__(code);
    int curr_reg = code->curr_reg;
    int retval_reg = GetNextRegister__(code, curr_reg);

    int64_t func_id = 0;
    if (EvalExpr(call->l, &func_id)) {
        CallFunction__(code, retval_reg, func_id, func_type->is_builtin);
    }
    else {
        /*
        gen_expr(code, call->l);
        CallFunctionPointer(code);
        */
    }

    // update current register pointer
    code->curr_reg = retval_reg;

    return retval_reg;
}

static int gen_expr__(Bytecode *code, const struct Expr *e)
{
    if (!e)
        return -1;

    int reg0 = -1;
    int reg1 = -1;
    int reg2 = -1;

    switch (e->kind) {

        /*
    case T_NILLIT:
        LoadByte(code, 0);
        return;

    case T_BOLLIT:
        LoadByte(code, e->ival);
        return;
        */

    case T_INTLIT:
        // TODO LoadInt could take care of this
        reg0 = PoolInt__(code, e->ival);
        if (reg0 == -1) {
            // reg0 = LoadInt(code, , e->ival)
        }
        return reg0;

        /*
    case T_FLTLIT:
        LoadFloat(code, e->fval);
        return;
        */

    case T_STRLIT:
        {
            const char *s = NULL;

            // TODO could remove e->converted
            if (!e->converted)
                s = e->sval;
            else
                s = e->converted;

            reg0 = PoolString__(code, s);
        }
        return reg0;

        /*
    case T_FUNCLIT:
        LoadInt(code, e->func->id);
        return;

    case T_CONV:
        gen_expr(code, e->l);
        gen_convert(code, e->l->type->kind, e->type->kind);
        return;
        */

    case T_IDENT:
        if (e->var->is_global) {
            //LoadGlobal(code, e->var->offset);
            // TODO LoadInt could take care of this
            //reg0 = LoadInt__(code, e->var->offset + 1);
            reg1 = PoolInt__(code, e->var->offset);
            if (reg0 == -1) {
                // reg0 = LoadInt(code, , e->ival)
            }
            reg0 = NewRegister__(code);
            reg0 = Load__(code, reg0, reg1);
        } else {
            reg0 = e->var->offset;
        }
        return reg0;
        /*
        if (IsStruct(e->type)) {
            gen_addr(code, e);
            return;
        }

        if (e->var->is_global)
            LoadGlobal(code, e->var->offset);
        else
            LoadLocal(code, e->var->offset);
        return;
        */

        /*
    case T_SELECT:
        gen_addr(code, e);
        Load(code);
        return;
        */

    case T_INDEX:
        reg0 = NewRegister__(code);
        reg1 = gen_addr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        LoadArray__(code, reg0, reg1, reg2);
        return reg0;

    case T_CALL:
        return gen_call__(code, e);

        /*
    case T_LOR:
        gen_logor(code, e);
        return;

    case T_LAND:
        gen_logand(code, e);
        return;
        */

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
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);

        if (IsTempRegister(code, reg1))
            reg0 = reg1;
        else if (IsTempRegister(code, reg2))
            reg0 = reg2;
        else
            reg0 = NewRegister__(code);

        BINOP_S__(code, e->type, Add, Concat, reg0, reg1, reg2);
        return reg0;

        /*
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
        */

    case T_REM:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);

        if (IsTempRegister(code, reg1))
            reg0 = reg1;
        else if (IsTempRegister(code, reg2))
            reg0 = reg2;
        else
            reg0 = NewRegister__(code);

        BINOP__(code, e->type, Rem, reg0, reg1, reg2);
        return reg0;

    case T_EQ:
        //gen_expr(code, e->l);
        //gen_expr(code, e->r);
        //EMITS(code, e->l->type, Equal, Equal);
        //return;
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);

        if (IsTempRegister(code, reg1))
            reg0 = reg1;
        else if (IsTempRegister(code, reg2))
            reg0 = reg2;
        else
            reg0 = NewRegister__(code);

        BINOP_S__(code, e->type, Equal, Equal, reg0, reg1, reg2);
        return reg0;

    case T_LT:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);

        if (IsTempRegister(code, reg1))
            reg0 = reg1;
        else if (IsTempRegister(code, reg2))
            reg0 = reg2;
        else
            reg0 = NewRegister__(code);

        BINOP__(code, e->type, Less, reg0, reg1, reg2);
        return reg0;

        /*
    case T_NEQ:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->l->type, NotEqual, NotEqual);
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
        */

    case T_ASSN:
        return gen_assign__(code, e);

    case T_AADD: case T_ASUB:
    case T_AMUL: case T_ADIV: case T_AREM:
        return gen_binop_assign__(code, e);

    case T_INIT:
        if (IsArray(e->type))
            gen_init_array__(code, e);
        else if (IsStruct(e->type))
            ;//gen_init_struct(code, e);
        else
            gen_init__(code, e);
        return 0;

    case T_INC:
        if (IsGlobal(e->l))
            ;//IncGlobal(code, Addr(e->l));
        else {
            reg0 = gen_addr__(code, e->l);
            Inc__(code, reg0);
        }
        return reg0;

        /*
    case T_DEC:
        if (IsGlobal(e->l))
            DecGlobal(code, Addr(e->l));
        else
            DecLocal(code, Addr(e->l));
        return;
        */
    }

    return -1;
}

static int gen_addr__(Bytecode *code, const struct Expr *e)
{
    if (!e)
        return -1;

    int reg0 = -1;

    switch (e->kind) {

    case T_IDENT:
        /*
        if (IsPtr(e->type)) {
            gen_expr(code, e);
            return;
        }

        if (IsStruct(e->type)) {
            if (e->var->is_param) {
                LoadAddress(code, e->var->offset);
                Dereference(code);
            }
            else if (e->var->is_global) {
                LoadInt(code, e->var->offset + 1);
            }
            else {
                LoadAddress(code, e->var->offset);
            }
            return;
        }
        */

        if (e->var->is_global) {
            // TODO LoadInt could take care of this
            //reg0 = LoadInt__(code, e->var->offset + 1);
            reg0 = PoolInt__(code, e->var->offset);
            if (reg0 == -1) {
                // reg0 = LoadInt(code, , e->ival)
            }
        }
        else {
            reg0 = e->var->offset;
        }
        return reg0;

        /*
    case T_FIELD:
        LoadByte(code, e->field->offset);
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
        */

    }

    return reg0;
}
static void gen_stmt__(Bytecode *code, const struct Stmt *s)
{
    if (!s)
        return;

    int reg0 = 0;

    switch (s->kind) {

    case T_NOP:
        return;

    case T_BLOCK:
        for (struct Stmt *stmt = s->children; stmt; stmt = stmt->next) {
            gen_stmt__(code, stmt);
            ResetCurrentRegister__(code);
        }
        return;

    case T_IF:
        BeginIf__(code);

        for (struct Stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt__(code, stmt);

        // exit
        BackPatchElseEnds__(code);
        return;

    case T_ELS:
        {
            Int next = 0;

            if (s->cond) {
                // cond
                int reg0 = gen_expr__(code, s->cond);
                next = JumpIfZero__(code, reg0, -1);
            }

            // true
            gen_stmt__(code, s->body);

            if (s->cond) {
                // close
                Int addr = Jump__(code, -1);
                PushElseEnd__(code, addr);
                BackPatch__(code, next);
            }
        }
        return;

    case T_FOR:
        {
            // init
            BeginFor(code);
            gen_stmt__(code, s->init);

            // cond
            Int begin = NextAddr__(code);
            int reg0 = gen_expr__(code, s->cond);
            Int exit = JumpIfZero__(code, reg0, -1);

            // body
            gen_stmt__(code, s->body);

            // post
            BackPatchContinues__(code);
            gen_stmt__(code, s->post);
            Jump__(code, begin);

            // exit
            BackPatch__(code, exit);
            BackPatchBreaks__(code);
        }
        return;

        /*
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
            for (struct Expr *cond = s->cond; cond; cond = cond->next) {
                Int tru = 0;
                Int fls = 0;
                DuplicateTop(code);
                gen_expr(code, cond);
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
        for (struct Stmt *cas = s->children; cas; cas = cas->next)
            gen_stmt(code, cas);

        // quit
        BackPatchCaseCloses(code);
        // remove cond val
        Pop(code);
        return;
        */

    case T_RET:
        reg0 = gen_expr__(code, s->expr);
        Return__(code, reg0);
        return;

        /*
    case T_EXPR:
        gen_expr(code, s->expr);
        // remove the result
        Pop(code);
        return;
        */

        // XXX need T_ASSNSTMT?
    case T_ASSN:
    case T_INIT:
        gen_expr__(code, s->expr);
        return;
    }
}

static void gen_func__(Bytecode *code, const struct Func *func, int func_id)
{
    // Register function
    RegisterFunction__(code, func_id, func->params.len);

    // TODO solve param count and reg count at a time
    // Local var registers
    int param_count = func->params.len;
    int lvar_count = func->scope->size;
    InitLocalVarRegister__(code, lvar_count + param_count);

    // Function body
    gen_stmt__(code, func->body);

    // Back patch used registers
    SetMaxRegisterCount__(code, func_id);
}

static void gen_funcs__(Bytecode *code, const struct Module *mod)
{
    struct Scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_funcs(code, sym->module);
    }

    // self module next
    for (int i = 0; i < mod->funcs.len; i++) {
        struct Func *func = mod->funcs.data[i];
        if (!func->is_builtin)
            gen_func__(code, func, func->id);
    }
}

static void gen_gvars__(Bytecode *code, const struct Module *mod)
{
    struct Scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct Symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_gvars__(code, sym->module);
    }

    // self module next
    for (const struct Stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        gen_stmt__(code, gvar);
}

static void gen_module__(Bytecode *code, const struct Module *mod)
{
    if (!mod->main_func) {
        fprintf(stderr, "error: 'main' function not found");
    }

    int gvar_count = mod->scope->size;
    int retval_count = 1;

    // Global var registers
    InitLocalVarRegister__(code, gvar_count);
    Allocate__(code, gvar_count + retval_count);

    // Global vars
    gen_gvars__(code, mod);

    // TODO maybe better to search "main" module and "main" func in there
    // instead of holding main_func
    // Call main
    int reg0 = NewRegister__(code);
    CallFunction__(code, reg0, mod->main_func->id, mod->main_func->is_builtin);
    Exit__(code);

    // Global funcs
    gen_funcs__(code, mod);
}
