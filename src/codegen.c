#include "codegen.h"
#include "bytecode.h"
#include "error.h"
#include "parser_ast.h"
#include "parser_ast_eval.h"
#include "parser_symbol.h"
#include "parser_type.h"
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

/*static*/ void push_int(IntVec *v, Int data)
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
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty))) \
        op##Int((code)); \
    else if (parser_is_float_type((ty))) \
        op##Float((code)); \
    } while (0)

#define EMITS(code, ty, op, ops) \
    do { \
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty))) \
        op##Int((code)); \
    else if (parser_is_float_type((ty))) \
        op##Float((code)); \
    else if (parser_is_string_type((ty))) \
        ops##String((code)); \
    } while (0)

#define BINOP__(code, ty, op, r0, r1, r2) \
    do { \
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty))) \
        op##Int__((code), (r0), (r1), (r2)); \
    else if (parser_is_float_type((ty))) \
        op##Float__((code), (r0), (r1), (r2)); \
    } while (0)

#define BINOP_S__(code, ty, op, ops, r0, r1, r2) \
    do { \
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty))) \
        op##Int__((code), (r0), (r1), (r2)); \
    else if (parser_is_float_type((ty))) \
        op##Float__((code), (r0), (r1), (r2)); \
    else if (parser_is_string_type((ty))) \
        ops##String__((code), (r0), (r1), (r2)); \
    } while (0)

static void gen_expr(Bytecode *code, const struct parser_expr *e);
static void gen_addr(Bytecode *code, const struct parser_expr *e);

/*static*/ void gen_convert(Bytecode *code, int from, int to)
{
    switch (from) {
    case TYP_BOOL:
        switch (to) {
        case TYP_BOOL:  break;
        case TYP_INT:   BoolToInt(code); break;
        case TYP_FLOAT: BoolToFloat(code); break;
        default: break;
        }
        break;

    case TYP_INT:
        switch (to) {
        case TYP_BOOL:  IntToBool(code); break;
        case TYP_INT:   break;
        case TYP_FLOAT: IntToFloat(code); break;
        default: break;
        }
        break;

    case TYP_FLOAT:
        switch (to) {
        case TYP_BOOL:  FloatToBool(code); break;
        case TYP_INT:   FloatToInt(code); break;
        case TYP_FLOAT: break;
        default: break;
        }
        break;

    default:
        break;
    }
}

/*static*/ void gen_call(Bytecode *code, const struct parser_expr *call)
{
    const struct parser_func_type *func_type = call->l->type->func_type;

    if (func_type->is_variadic) {
        int argc = 0;
        for (const struct parser_expr *arg = call->r; arg; arg = arg->next, argc++) {
            // arg value
            gen_expr(code, arg);

            switch (arg->type->kind) {
            case TYP_NIL:
                LoadTypeNil(code);
                break;
            case TYP_BOOL:
                LoadTypeBool(code);
                break;
            case TYP_INT:
                LoadTypeInt(code);
                break;
            case TYP_FLOAT:
                LoadTypeFloat(code);
                break;
            case TYP_STRING:
                LoadTypeString(code);
                break;
            case TYP_FUNC:
            case TYP_STRUCT:
            case TYP_TABLE:
            case TYP_MODULE:
            case TYP_PTR:
            case TYP_ARRAY:
            case TYP_ANY:
                LoadTypeNil(code);
                break;
            }
        }
        // arg count
        LoadByte(code, argc);
    }
    else {
        for (const struct parser_expr *arg = call->r; arg; arg = arg->next)
            gen_expr(code, arg);
    }

    int64_t func_id = 0;
    if (parser_eval_expr(call->l, &func_id)) {
        CallFunction(code, func_id, func_type->is_builtin);
    }
    else {
        gen_expr(code, call->l);
        CallFunctionPointer(code);
    }
}

/*static*/ void gen_logor(Bytecode *code, const struct parser_expr *e)
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

/*static*/ void gen_logand(Bytecode *code, const struct parser_expr *e)
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

/*static*/ void gen_store(Bytecode *code, const struct parser_expr *l, int offset)
{
    // lval
    int addr = 0;
    const bool isconst = parser_eval_addr(l, &addr);

    // store
    if (isconst) {
        if (parser_ast_is_global(l))
            StoreGlobal(code, addr + offset);
        else
            StoreLocal(code, addr + offset);
    }
    else {
        gen_addr(code, l);
        Store(code);
    }
}

/*static*/ void gen_clear_block(Bytecode *code, const struct parser_expr *dst)
{
    int dst_addr = 0;
    parser_eval_addr(dst, &dst_addr);

    if (parser_ast_is_global(dst)) {
        // TODO support variable addresses
        ClearGlobal(code, dst_addr, parser_sizeof_type(dst->type));
    }
    else {
        // TODO support variable addresses
        ClearLocal(code, dst_addr, parser_sizeof_type(dst->type));
    }
}

/*static*/ void gen_copy_block(Bytecode *code,
        const struct parser_expr *src, const struct parser_expr *dst)
{
    if (parser_ast_is_global(src)) {
        int src_addr = 0;
        int dst_addr = 0;

        parser_eval_addr(src, &src_addr);
        parser_eval_addr(dst, &dst_addr);
        // TODO support variable addresses
        CopyGlobal(code, src_addr, dst_addr, parser_sizeof_type(src->type));
    }
    else {
        int src_addr = 0;
        int dst_addr = 0;

        parser_eval_addr(src, &src_addr);
        parser_eval_addr(dst, &dst_addr);
        // TODO support variable addresses
        CopyLocal(code, src_addr, dst_addr, parser_sizeof_type(src->type));
    }
}


/*static*/ void gen_assign(Bytecode *code, const struct parser_expr *e)
{
    /*
    if (IsStruct(e->type)) {
        gen_copy_block(code, e->r, e->l);
        return;
    }

    // rval first
    if (e->kind == TOK_ASSN) {
        gen_expr(code, e->r);
    }
    else {
        gen_expr(code, e->l);
        gen_expr(code, e->r);

        switch (e->kind) {
        case TOK_AADD:
            EMITS(code, e->type, Add, Concat);
            break;

        case TOK_ASUB:
            EMIT(code, e->type, Sub);
            break;

        case TOK_AMUL:
            EMIT(code, e->type, Mul);
            break;

        case TOK_ADIV:
            EMIT(code, e->type, Div);
            break;

        case TOK_AREM:
            EMIT(code, e->type, Rem);
            break;
        }
    }

    // lval
    int addr = 0;
    const bool isconst = parser_eval_addr(e->l, &addr);

    // store
    if (isconst) {
        if (parser_ast_is_global(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
    }
    else {
        gen_addr(code, e->l);
        Store(code);
    }
    */
}

/*static*/ void gen_init_array(Bytecode *code, const struct parser_expr *e)
{
    // TODO testing dynamic array
    {
        // lval
        int addr = 0;
        // an init expr always has identifier on the left
        parser_eval_addr(e->l, &addr);

        LoadInt(code, e->type->len);
        ArrayLocal(code, addr);
        StoreLocal(code, addr);

        //return;
    }
    // TODO =====================

    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    parser_eval_addr(e->l, &addr);

    // array len
    LoadInt(code, e->type->len);
    if (parser_ast_is_global(e->l))
        StoreGlobal(code, addr);
    else
        StoreLocal(code, addr);
    addr++;

    // array lit
    struct parser_expr *array_lit = e->r;

    for (struct parser_expr *expr = array_lit->l; expr; expr = expr->next) {
        // rval
        gen_expr(code, expr);

        // store
        if (parser_ast_is_global(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
        addr++;
    }
}

/*static*/ void gen_init_struct(Bytecode *code, const struct parser_expr *e)
{
    /*
    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    parser_eval_addr(e->l, &addr);

    if (e->r && e->r->kind == TOK_NILLIT) {
        // no initializer
        // clear zero
        gen_clear_block(code, e->l);
        return;
    }

    if (e->r && e->r->kind != TOK_STRUCTLIT) {
        // initialized by another object
        gen_copy_block(code, e->r, e->l);
        return;
    }
    // struct literal initializer

    // clear zero
    gen_clear_block(code, e->l);

    // struct lit
    struct parser_expr *struct_lit = e->r;

    for (struct parser_expr *elem = struct_lit->l; elem; elem = elem->next) {
        // rval
        gen_expr(code, elem->r);

        // lval
        int offset = 0;
        parser_eval_addr(elem->l, &offset);

        // store
        if (parser_ast_is_global(e->l))
            StoreGlobal(code, addr + offset);
        else
            StoreLocal(code, addr + offset);
    }
    */
}

/*static*/ void gen_init(Bytecode *code, const struct parser_expr *e)
{
    // rval
    gen_expr(code, e->r);

    // lval
    int addr = 0;
    const bool isconst = parser_eval_addr(e->l, &addr);

    // store
    if (isconst) {
        if (parser_ast_is_global(e->l))
            StoreGlobal(code, addr);
        else
            StoreLocal(code, addr);
    }
    else {
        gen_addr(code, e->l);
        Store(code);
    }
}

static void gen_expr(Bytecode *code, const struct parser_expr *e)
{
    /*
    if (!e)
        return;

    switch (e->kind) {

    case TOK_NILLIT:
        LoadByte(code, 0);
        return;

    case TOK_BOOLLIT:
        LoadByte(code, e->ival);
        return;

    case TOK_INTLIT:
        if (e->ival >= 0 && e->ival <= UINT8_MAX)
            LoadByte(code, e->ival);
        else
            LoadInt(code, e->ival);
        return;

    case TOK_FLOATLIT:
        LoadFloat(code, e->fval);
        return;

    case TOK_STRINGLIT:
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

    case TOK_FUNCLIT:
        LoadInt(code, e->func->id);
        return;

    case TOK_CONV:
        gen_expr(code, e->l);
        gen_convert(code, e->l->type->kind, e->type->kind);
        return;

    case TOK_IDENT:
        if (IsStruct(e->type)) {
            gen_addr(code, e);
            return;
        }

        if (e->var->is_global)
            LoadGlobal(code, e->var->offset);
        else
            LoadLocal(code, e->var->offset);
        return;

    case TOK_SELECT:
        gen_addr(code, e);
        Load(code);
        return;

    case TOK_INDEX:
        gen_addr(code, e);
        Load(code);
        return;

    case TOK_CALL:
        gen_call(code, e);
        return;

    case TOK_LOR:
        gen_logor(code, e);
        return;

    case TOK_LAND:
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
    case TOK_ADD:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->type, Add, Concat);
        return;

    case TOK_SUB:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Sub);
        return;

    case TOK_MUL:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Mul);
        return;

    case TOK_DIV:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Div);
        return;

    case TOK_REM:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->type, Rem);
        return;

    case TOK_EQ:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->l->type, Equal, Equal);
        return;

    case TOK_NEQ:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMITS(code, e->l->type, NotEqual, NotEqual);
        return;

    case TOK_LT:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, Less);
        return;

    case TOK_LTE:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, LessEqual);
        return;

    case TOK_GT:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, Greater);
        return;

    case TOK_GTE:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        EMIT(code, e->l->type, GreaterEqual);
        return;

    case TOK_AND:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        And(code);
        return;

    case TOK_OR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        Or(code);
        return;

    case TOK_XOR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        Xor(code);
        return;

    case TOK_SHL:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        ShiftLeft(code);
        return;

    case TOK_SHR:
        gen_expr(code, e->l);
        gen_expr(code, e->r);
        ShiftRight(code);
        return;

    case TOK_ADR:
        LoadAddress(code, Addr(e->l));
        return;

    case TOK_POS:
        gen_expr(code, e->l);
        return;

    case TOK_NEG:
        gen_expr(code, e->l);
        EMIT(code, e->type, Negate);
        return;

    case TOK_LNOT:
        gen_expr(code, e->l);
        SetIfZero(code);
        return;

    case TOK_NOT:
        gen_expr(code, e->l);
        Not(code);
        return;

    case TOK_DRF:
        gen_expr(code, e->l);
        Dereference(code);
        return;

    case TOK_ASSN:
    case TOK_AADD: case TOK_ASUB:
    case TOK_AMUL: case TOK_ADIV: case TOK_AREM:
        gen_assign(code, e);
        return;

    case TOK_INIT:
        if (IsArray(e->type))
            gen_init_array(code, e);
        else if (IsStruct(e->type))
            gen_init_struct(code, e);
        else
            gen_init(code, e);
        return;

    case TOK_INC:
        if (parser_ast_is_global(e->l))
            IncGlobal(code, Addr(e->l));
        else
            IncLocal(code, Addr(e->l));
        return;

    case TOK_DEC:
        if (parser_ast_is_global(e->l))
            DecGlobal(code, Addr(e->l));
        else
            DecLocal(code, Addr(e->l));
        return;
    }
    */
}

static void gen_addr(Bytecode *code, const struct parser_expr *e)
{
    /*
    if (!e)
        return;

    switch (e->kind) {

    case TOK_IDENT:
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

    case TOK_FIELD:
        LoadByte(code, e->field->offset);
        return;

    case TOK_SELECT:
        //if (optimize) {
        //    int base = 0;
        //    int offset = 0;
        //    if (inst->parser_eval_addr(base) && fld->EvalAddr(offset)) {
        //        if (inst->parser_ast_is_global())
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

    case TOK_INDEX:
        //if (optimize) {
        //    int base = 0;
        //    long index = 0;
        //    if (ary->parser_eval_addr(base) && idx->Eval(index)) {
        //        if (ary->parser_ast_is_global())
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

    case TOK_DRF:
        // deref *i = ...
        gen_expr(code, e->l);
        return;

    }
    */
}

static void gen_stmt(Bytecode *code, const struct parser_stmt *s)
{
    /*
    if (!s)
        return;

    switch (s->kind) {

    case TOK_NOP:
        return;

    case TOK_BLOCK:
        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);
        return;

    case TOK_ELSE:
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

    case TOK_IF:
        BeginIf(code);

        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);

        // exit
        BackPatchOrCloses(code);
        return;

    case TOK_FOR:
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

    case TOK_BREAK:
        {
            const Int addr = Jump(code, -1);
            PushBreak(code, addr);
        }
        return;

    case TOK_CONTINUE:
        {
            const Int addr = Jump(code, -1);
            PushContinue(code, addr);
        }
        return;

    case TOK_CASE:
        {
            Int exit = 0;

            IntVec trues = {0};
            // eval conds
            for (struct parser_expr *cond = s->cond; cond; cond = cond->next) {
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

    case TOK_DEFAULT:
        // body
        gen_stmt(code, s->body);
        return;

    case TOK_SWITCH:
        // init
        BeginSwitch(code);
        gen_expr(code, s->cond);

        // cases
        for (struct parser_stmt *cas = s->children; cas; cas = cas->next)
            gen_stmt(code, cas);

        // quit
        BackPatchCaseCloses(code);
        // remove cond val
        Pop(code);
        return;

    case TOK_RETURN:
        gen_expr(code, s->expr);
        Return(code);
        return;

    case TOK_EXPR:
        gen_expr(code, s->expr);
        // remove the result
        Pop(code);
        return;

    case TOK_ASSN:
    case TOK_INIT:
        gen_expr(code, s->expr);
        return;
    }
    */
}

static void gen_func(Bytecode *code, const struct parser_func *func, int func_id)
{
    //BackPatchFuncAddr(code, func->fullname);
    RegisterFunction(code, func_id, func->params.len);

    // local vars
    Allocate(code, func->scope->size);

    gen_stmt(code, func->body);
}

static void gen_gvars(Bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_gvars(code, sym->module);
    }

    // self module next
    for (const struct parser_stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        gen_stmt(code, gvar);
}

static void gen_funcs(Bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_funcs(code, sym->module);
    }

    // self module next
    for (int i = 0; i < mod->funcs.len; i++) {
        struct parser_func *func = mod->funcs.data[i];
        if (!func->is_builtin)
            gen_func(code, func, func->id);
    }
}

static void gen_module(Bytecode *code, const struct parser_module *mod)
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

/*static*/ void register_funcs(Bytecode *code, const struct parser_module *mod)
{
    for (int i = 0; i < mod->funcs.len; i++) {
        struct parser_func *func = mod->funcs.data[i];
        if (!func->is_builtin) {
            func->id = RegisterFunc(code, func->fullname, func->params.len);
        }
    }
}

// XXX TEST ----------------
static void gen_module__(Bytecode *code, const struct parser_module *mod);
void GenerateCode__(struct Bytecode *code, const struct parser_module *mod)
{
    gen_module__(code, mod);
    End__(code);
}

void GenerateCode(struct Bytecode *code, const struct parser_module *mod)
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

static int resolve_func_id(struct parser_scope *scope, int start_id)
{
    int next_id = start_id;

    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

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

static int resolve_offset(struct parser_scope *scope, int start_offset)
{
    int cur_offset = start_offset;
    int max_offset = start_offset;
    int cur_size = 0;
    int max_size = 0;

    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_VAR) {
            struct parser_var *var = sym->var;
            // offset
            var->id = cur_offset;
            cur_offset += parser_sizeof_type(var->type);
            max_offset = max(max_offset, cur_offset);
            // size
            if (!var->is_param)
                cur_size += parser_sizeof_type(var->type);
            max_size = max(max_size, cur_size);
        }
        else if (sym->kind == SYM_FUNC) {
            struct parser_scope *child = sym->func->scope;
            // start over from offset 0
            resolve_offset(child, 0);
        }
        else if (sym->kind == SYM_SCOPE) {
            struct parser_scope *child = sym->scope;
            int child_max = resolve_offset(child, cur_offset);
            // offset
            max_offset = max(max_offset, child_max);
            // size
            max_size = max(max_size, cur_size + child->size);
        }
        else if (sym->kind == SYM_MODULE) {
            struct parser_scope *child = sym->module->scope;
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

void ResolveOffset(struct parser_module *mod)
{
    resolve_offset(mod->scope, 0);
    resolve_func_id(mod->scope, 0);
}

// XXX TEST compiling to register-based machine code
static int gen_expr__(Bytecode *code, const struct parser_expr *e);
static int gen_addr__(Bytecode *code, const struct parser_expr *e);

static int gen_convert__(Bytecode *code, int reg0, int reg1, int from, int to)
{
    int reg = -1;

    switch (from) {
    case TYP_BOOL:
        switch (to) {
        case TYP_BOOL:  break;
        case TYP_INT:   reg = BoolToInt__(code, reg0, reg1); break;
        /*
        case TYP_FLOAT: BoolToFloat(code); break;
        */
        default: break;
        }
        break;

                       /*
    case TYP_INT:
        switch (to) {
        case TYP_BOOL:  reg = IntToBool__(code, reg0, reg1); break;
        case TYP_INT:   break;
        case TYP_FLOAT: IntToFloat(code); break;
        default: break;
        }
        break;
                       */

        /*
    case TYP_FLOAT:
        switch (to) {
        case TYP_BOOL:  FloatToBool(code); break;
        case TYP_INT:   FloatToInt(code); break;
        case TYP_FLOAT: break;
        default: break;
        }
        break;
        */

    default:
        break;
    }

    return reg;
}

// TODO remove forward decls
static int gen_dst_register(Bytecode *code, int reg1, int reg2);
static int gen_dst_register2(struct Bytecode *code, int reg1);
static int gen_store2__(Bytecode *code, const struct parser_expr *lval, int src_reg);
static int gen_init_array__(Bytecode *code, const struct parser_expr *e)
{
    int dst = 0;

    // TODO testing dynamic array
    if (parser_ast_is_global(e->l)) {
        int addr = gen_addr__(code, e->l);
        int len = gen_expr__(code, e->r);
        int dst = gen_dst_register(code, addr, len);
        NewArray__(code, dst, len);
        Store__(code, addr, dst);
        return addr;
    }
    else {
        // an init expr always has identifier on the left
        int reg0 = gen_expr__(code, e->l);
        int reg1 = LoadInt__(code, e->type->len);
        NewArray__(code, reg0, reg1);
        dst = reg0;
    }

    // TODO eval array lit expr and return an array object
    // array lit
    struct parser_expr *array_lit = e->r;
    int index = 0;

    for (struct parser_expr *expr = array_lit->l; expr; expr = expr->next) {
        int src = gen_expr__(code, expr);
        int idx = LoadInt__(code, index);
        StoreArray__(code, dst, idx, src);
        index++;
    }

    return dst;
}

static int gen_struct_lit__(Bytecode *code, const struct parser_expr *e,
        const struct parser_type *type, /* XXX TEMP */
        int dst_reg)
{
    /* XXX TEMP */
    if (e && e->kind == NOD_EXPR_IDENT) {
        /* initialized by another object */
        if (dst_reg == -1) {
            /* global */
            int src = gen_expr__(code, e);
            return src;
        }
        else {
            /* local */
            int src = gen_addr__(code, e);
            Move__(code, dst_reg, src);
            return dst_reg;
        }
    }
    /*
    int len = parser_struct_get_field_count(e->type->strct);
    */ /* XXX TEMP */
    int len = parser_struct_get_field_count(type->strct);
    int dst = 0;

    if (dst_reg == -1)
        dst = NewRegister__(code);
    else
        dst = dst_reg;

    NewStruct__(code, dst, len);

    const struct parser_expr *struct_lit = e;

    for (struct parser_expr *elem = struct_lit->l; elem; elem = elem->next) {
        int src = gen_expr__(code, elem->r);
        int idx = gen_addr__(code, elem->l);

        StoreStruct__(code, dst, idx, src);
    }

    return dst;
}

static int gen_init_struct__(Bytecode *code, const struct parser_expr *e)
{
    if (parser_ast_is_global(e->l)) {
        int tmp = gen_struct_lit__(code, e->r, e->type, -1);
        int dst = gen_addr__(code, e->l);
        Store__(code, dst, tmp);
        return dst;
    }
    else {
        int dst = gen_addr__(code, e->l);
        gen_struct_lit__(code, e->r, e->type, dst);
        return dst;
    }

    /*
    // lval
    int addr = 0;
    // an init expr always has identifier on the left
    parser_eval_addr(e->l, &addr);

    if (e->r && e->r->kind == NOD_EXPR_NILLIT) {
        // no initializer
        // clear zero
        gen_clear_block(code, e->l);
        return;
    }

    if (e->r && e->r->kind != NOD_EXPR_STRUCTLIT) {
        // initialized by another object
        gen_copy_block(code, e->r, e->l);
        return;
    }
    // struct literal initializer

    // clear zero
    gen_clear_block(code, e->l);

    // struct lit
    struct parser_expr *struct_lit = e->r;

    for (struct parser_expr *elem = struct_lit->l; elem; elem = elem->next) {
        // rval
        gen_expr(code, elem->r);

        // lval
        int offset = 0;
        parser_eval_addr(elem->l, &offset);

        // store
        if (parser_ast_is_global(e->l))
            StoreGlobal(code, addr + offset);
        else
            StoreLocal(code, addr + offset);
    }
    */
}

static int gen_store__(Bytecode *code, const struct parser_expr *lval, const struct parser_expr *rval)
{
    int reg0 = 0xff;
    int reg1 = gen_expr__(code, rval);

    if (parser_ast_is_global(lval)) {
        reg0 = gen_addr__(code, lval);
        Store__(code, reg0, reg1);
    }
    else {
        // TODO handle case where lhs is not addressable
        reg0 = gen_addr__(code, lval);
        Move__(code, reg0, reg1);
    }

    return reg0;
}

static int gen_store2__(Bytecode *code, const struct parser_expr *lval, int src_reg)
{
    int dst_reg = -1;

    if (parser_ast_is_global(lval)) {
        dst_reg = gen_addr__(code, lval);
        Store__(code, dst_reg, src_reg);
    }
    else {
        // TODO handle case where lhs is not addressable
        dst_reg = gen_addr__(code, lval);
        Move__(code, dst_reg, src_reg);
    }

    return dst_reg;
}

static int gen_init__(Bytecode *code, const struct parser_expr *e)
{
    // rval
    int reg0 = -1;
    int reg1 = gen_expr__(code, e->r);

    if (parser_ast_is_global(e->l)) {
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
        if (parser_ast_is_global(e->l))
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

// TODO move to bytecode.c
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

// TODO move to bytecode.c
static int gen_dst_register2(struct Bytecode *code, int reg1)
{
    int reg0 = -1;

    // determine the destination register
    if (IsTempRegister(code, reg1))
        reg0 = reg1;
    else
        reg0 = NewRegister__(code);

    return reg0;
}

static int gen_binop__(Bytecode *code, const struct parser_type *type, int kind,
        int reg0, int reg1, int reg2)
{
    switch (kind) {
    case NOD_EXPR_ADD:
    case NOD_EXPR_ADDASSIGN:
        BINOP_S__(code, type, Add, Concat, reg0, reg1, reg2);
        break;

    case NOD_EXPR_SUB:
    case NOD_EXPR_SUBASSIGN:
        BINOP__(code, type, Sub, reg0, reg1, reg2);
        break;

    case NOD_EXPR_MUL:
    case NOD_EXPR_MULASSIGN:
        BINOP__(code, type, Mul, reg0, reg1, reg2);
        break;

    case NOD_EXPR_DIV:
    case NOD_EXPR_DIVASSIGN:
        BINOP__(code, type, Div, reg0, reg1, reg2);
        break;

    case NOD_EXPR_REM:
    case NOD_EXPR_REMASSIGN:
        BINOP__(code, type, Rem, reg0, reg1, reg2);
        break;

    case NOD_EXPR_EQ:
        BINOP_S__(code, type, Equal, Equal, reg0, reg1, reg2);
        break;

    case NOD_EXPR_NEQ:
        BINOP_S__(code, type, NotEqual, NotEqual, reg0, reg1, reg2);
        break;

    case NOD_EXPR_LT:
        BINOP__(code, type, Less, reg0, reg1, reg2);
        break;

    case NOD_EXPR_LTE:
        BINOP__(code, type, LessEqual, reg0, reg1, reg2);
        break;

    case NOD_EXPR_GT:
        BINOP__(code, type, Greater, reg0, reg1, reg2);
        break;

    case NOD_EXPR_GTE:
        BINOP__(code, type, GreaterEqual, reg0, reg1, reg2);
        break;

    }
    return reg0;
}

static int gen_assign__(Bytecode *code, const struct parser_expr *e)
{
    const struct parser_expr *lval = e->l;
    const struct parser_expr *rval = e->r;
    int reg0 = -1;
    int reg1 = -1;
    int reg2 = -1;

    if (lval->kind == NOD_EXPR_INDEX) {
        int reg0 = gen_expr__(code, lval->l);
        int reg1 = gen_expr__(code, lval->r);
        int reg2 = gen_expr__(code, rval);
        StoreArray__(code, reg0, reg1, reg2);
        return reg0;
    }
    else if (lval->kind == NOD_EXPR_SELECT) {
        // eval struct value
        reg0 = gen_expr__(code, lval->l);
        // get field offset
        reg1 = gen_addr__(code, lval->r);
        // eval rval
        reg2 = gen_expr__(code, rval);
        StoreStruct__(code, reg0, reg1, reg2);
        return reg0;
    }
    else if (lval->kind == NOD_EXPR_DEREF) {
        // TODO remove
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval);
        Store__(code, reg0, reg1);
        return reg0;
    }

    // TODO if rval is global then skip binop anyway.
    // Binop is one of `move` instruction (store value into register)

    // check the rvalue expression to see if binop r0, r1, r2 can be applied
    // e.g. a = b + c
    //            ^ here
    switch (rval->kind) {
    case NOD_EXPR_ADD:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP_S__(code, e->type, Add, Concat, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    case NOD_EXPR_MUL:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP__(code, e->type, Mul, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    case NOD_EXPR_REM:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP__(code, e->type, Rem, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    case NOD_EXPR_LT:
        reg0 = gen_addr__(code, lval);
        reg1 = gen_expr__(code, rval->l);
        reg2 = gen_expr__(code, rval->r);
        //BINOP__(code, e->type, Less, reg0, reg1, reg2);
        gen_binop__(code, e->type, rval->kind, reg0, reg1, reg2);
        break;

    default:
        gen_store__(code, lval, rval);
        break;
    }

    return reg0;
}

static int gen_binop_assign__(Bytecode *code, const struct parser_expr *e)
{
    if (e->l->kind == NOD_EXPR_INDEX) {
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

static int gen_call__(Bytecode *code, const struct parser_expr *call)
{
    const struct parser_func_type *func_type = call->l->type->func_type;

    // Save the current register right before evaluating args
    //int retval_reg = NewRegister__(code);
    int curr_reg = GetCurrentRegister__(code);

    if (func_type->is_variadic) {

        // arg count
        int argc = 0;
        int argc_dst = GetNextRegister__(code, curr_reg);

        for (const struct parser_expr *arg = call->r; arg; arg = arg->next, argc++) {
            // arg value
            int head_reg = GetCurrentRegister__(code);
            int arg_src = gen_expr__(code, arg);
            int arg_dst = GetNextRegister__(code, head_reg);
            Move__(code, arg_dst, arg_src);

            int type_dst = GetNextRegister__(code, arg_dst);

            switch (arg->type->kind) {
            case TYP_NIL:
                LoadTypeNil__(code, type_dst);
                break;

            case TYP_BOOL:
                LoadTypeBool__(code, type_dst);
                break;

            case TYP_INT:
                LoadTypeInt__(code, type_dst);
                break;

            case TYP_FLOAT:
                LoadTypeFloat__(code, type_dst);
                break;

            case TYP_STRING:
                LoadTypeString__(code, type_dst);
                break;

            case TYP_FUNC:
            case TYP_STRUCT:
            case TYP_TABLE:
            case TYP_MODULE:
            case TYP_PTR:
            case TYP_ARRAY:
            case TYP_ANY:
                LoadTypeNil__(code, type_dst);
                break;
            }
        }

        // arg count
        int argc_src = LoadInt__(code, argc);
        Move__(code, argc_dst, argc_src);
    }
    else {
        for (const struct parser_expr *arg = call->r; arg; arg = arg->next) {
            int cur = GetCurrentRegister__(code);
            int src = gen_expr__(code, arg);
            int dst = GetNextRegister__(code, cur);
            Move__(code, dst, src);
        }
    }

    // Get the returned value register right next to current
    int retval_reg = 0;

    int64_t func_id = 0;
    if (parser_eval_expr(call->l, &func_id)) {
        retval_reg = GetNextRegister__(code, curr_reg);
        CallFunction__(code, retval_reg, func_id, func_type->is_builtin);
    }
    else {
        int src = gen_expr__(code, call->l);
        retval_reg = GetNextRegister__(code, curr_reg);
        CallFunctionPointer__(code, retval_reg, src);
    }

    // update current register pointer
    /* TODO may be removed. GetNextRegister__() does this */
    code->curr_reg = retval_reg;

    return retval_reg;
}

static int gen_expr__(Bytecode *code, const struct parser_expr *e)
{
    if (!e)
        return -1;

    int reg0 = -1;
    int reg1 = -1;
    int reg2 = -1;

    switch (e->kind) {

    case NOD_EXPR_NILLIT:
        {
            int reg0 = LoadInt__(code, 0);
            return reg0;
        }

    case NOD_EXPR_BOOLLIT:
    case NOD_EXPR_INTLIT:
        {
            int reg0 = LoadInt__(code, e->ival);
            return reg0;
        }

    case NOD_EXPR_FLOATLIT:
        {
            int reg0 = LoadFloat__(code, e->fval);
            return reg0;
        }

    case NOD_EXPR_STRINGLIT:
        {
            const char *s = NULL;

            // TODO could remove e->converted
            if (!e->converted)
                s = e->sval;
            else
                s = e->converted;

            reg0 = LoadString__(code, s);
            return reg0;
        }

    case NOD_EXPR_FUNCLIT:
        {
            int reg0 = LoadInt__(code, e->func->id);
            return reg0;
        }

    case NOD_EXPR_CONV:
        reg1 = gen_expr__(code, e->l);
        reg0 = gen_dst_register2(code, reg1);
        reg0 = gen_convert__(code, reg0, reg1, e->l->type->kind, e->type->kind);
        return reg0;

    case NOD_EXPR_IDENT:
        if (e->var->is_global) {
            int reg1 = LoadInt__(code, e->var->id);
            int reg0 = NewRegister__(code);

            // TODO rename it to LoadGlobal()
            reg0 = Load__(code, reg0, reg1);
            return reg0;
        }
        else {
            int reg0 = e->var->id;
            return reg0;
        }

    case NOD_EXPR_SELECT:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg0, reg1);
        LoadStruct__(code, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_FIELD:
        return e->field->offset;


    case NOD_EXPR_INDEX:
        {
            int src = gen_expr__(code, e->l);
            int idx = gen_expr__(code, e->r);
            int dst = gen_dst_register(code, src, idx);
            LoadArray__(code, dst, src, idx);
            return dst;
        }

    case NOD_EXPR_CALL:
        return gen_call__(code, e);

    case NOD_EXPR_LOGOR:
        {
            // eval
            reg1 = gen_expr__(code, e->l);
            Int els = JumpIfZero__(code, reg1, -1);

            // true
            reg0 = gen_dst_register2(code, reg1);
            Move__(code, reg0, reg1);
            Int exit = Jump__(code, -1);

            // false
            BackPatch__(code, els);
            reg0 = gen_expr__(code, e->r);
            BackPatch__(code, exit);
        }
        return reg0;

    case NOD_EXPR_LOGAND:
        {
            // eval
            reg1 = gen_expr__(code, e->l);
            Int els = JumpIfZero__(code, reg1, -1);

            // true
            reg2 = gen_expr__(code, e->r);
            reg0 = gen_dst_register(code, reg1, reg2);
            Move__(code, reg0, reg2);
            Int exit = Jump__(code, -1);

            // false
            BackPatch__(code, els);
            Move__(code, reg0, reg1);
            BackPatch__(code, exit);
        }
        return reg0;

    case NOD_EXPR_ADD:
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

    case NOD_EXPR_SUB:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_MUL:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_DIV:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_REM:
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

    case NOD_EXPR_EQ:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);

        // e->type is always result type bool. e->l->type for operand type.
        //BINOP_S__(code, e->l->type, Equal, Equal, reg0, reg1, reg2);
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_NEQ:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);

        // e->type is always result type bool. e->l->type for operand type.
        //BINOP_S__(code, e->l->type, Equal, Equal, reg0, reg1, reg2);
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_LT:
    case NOD_EXPR_LTE:
    case NOD_EXPR_GT:
    case NOD_EXPR_GTE:
        reg1 = gen_expr__(code, e->l);
        reg2 = gen_expr__(code, e->r);
        reg0 = gen_dst_register(code, reg1, reg2);
        // e->type is always result type bool. e->l->type for operand type.
        gen_binop__(code, e->l->type, e->kind, reg0, reg1, reg2);
        return reg0;

    case NOD_EXPR_AND:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg2 = gen_expr__(code, e->r);
            int reg0 = gen_dst_register(code, reg1, reg2);
            reg0 = BitwiseAnd__(code, reg0, reg1, reg2);
            return reg0;
        }

    case NOD_EXPR_OR:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg2 = gen_expr__(code, e->r);
            int reg0 = gen_dst_register(code, reg1, reg2);
            reg0 = BitwiseOr__(code, reg0, reg1, reg2);
            return reg0;
        }

    case NOD_EXPR_XOR:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg2 = gen_expr__(code, e->r);
            int reg0 = gen_dst_register(code, reg1, reg2);
            reg0 = BitwiseXor__(code, reg0, reg1, reg2);
            return reg0;
        }

    case NOD_EXPR_NOT:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg0 = gen_dst_register2(code, reg1);
            reg0 = BitwiseNot__(code, reg0, reg1);
            return reg0;
        }

    case NOD_EXPR_SHL:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg2 = gen_expr__(code, e->r);
            int reg0 = gen_dst_register(code, reg1, reg2);
            reg0 = ShiftLeft__(code, reg0, reg1, reg2);
            return reg0;
        }

    case NOD_EXPR_SHR:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg2 = gen_expr__(code, e->r);
            int reg0 = gen_dst_register(code, reg1, reg2);
            reg0 = ShiftRight__(code, reg0, reg1, reg2);
            return reg0;
        }

    case NOD_EXPR_ADDRESS:
        if (parser_is_struct_type(e->l->type)) {
            int reg0 = gen_expr__(code, e->l);
            return reg0;
        }
        {
            int reg1 = gen_addr__(code, e->l);
            int reg0 = gen_dst_register2(code, reg1);
            LoadAddress__(code, reg0, reg1);
            return reg0;
        }

    case NOD_EXPR_POS:
        {
            int reg0 = gen_expr__(code, e->l);
            return reg0;
        }

    case NOD_EXPR_NEG:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg0 = gen_dst_register2(code, reg1);

            if (parser_is_int_type(e->type))
                reg0 = NegateInt__(code, reg0, reg1);
            else if (parser_is_float_type(e->type))
                reg0 = NegateFloat__(code, reg0, reg1);

            return reg0;
        }

    case NOD_EXPR_LOGNOT:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg0 = gen_dst_register2(code, reg1);
            reg0 = SetIfZero__(code, reg0, reg1);
            return reg0;
        }

    case NOD_EXPR_DEREF:
        {
            int reg1 = gen_expr__(code, e->l);
            int reg0 = gen_dst_register2(code, reg1);
            Dereference__(code, reg0, reg1);
            return reg0;
        }

    case NOD_EXPR_ASSIGN:
        return gen_assign__(code, e);

    case NOD_EXPR_ADDASSIGN:
    case NOD_EXPR_SUBASSIGN:
    case NOD_EXPR_MULASSIGN:
    case NOD_EXPR_DIVASSIGN:
    case NOD_EXPR_REMASSIGN:
        return gen_binop_assign__(code, e);

    case NOD_EXPR_INIT:
        if (parser_is_array_type(e->type))
            gen_init_array__(code, e);
        else if (parser_is_struct_type(e->type))
            gen_init_struct__(code, e);
        else
            gen_init__(code, e);
        return 0;

    case NOD_EXPR_INC:
        if (parser_ast_is_global(e->l)) {
            int src = gen_expr__(code, e->l);
            int dst = gen_dst_register2(code, src);
            Move__(code, dst, src);
            Inc__(code, dst);

            gen_store2__(code, e->l, dst);
            return dst;
        }
        else {
            int src = gen_addr__(code, e->l);
            Inc__(code, src);
            return src;
        }

    case NOD_EXPR_DEC:
        {
            if (parser_ast_is_global(e->l)) {
                int src = gen_expr__(code, e->l);
                int dst = gen_dst_register2(code, src);
                Move__(code, dst, src);
                Dec__(code, dst);

                gen_store2__(code, e->l, dst);
                return dst;
            }
            else {
                int src = gen_addr__(code, e->l);
                Dec__(code, src);
                return src;
            }
        }
    }

    return -1;
}

static int gen_addr__(Bytecode *code, const struct parser_expr *e)
{
    if (!e)
        return -1;

    int reg0 = -1;

    switch (e->kind) {

    case NOD_EXPR_IDENT:
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
            int reg0 = LoadInt__(code, e->var->id);
            return reg0;
        }
        else {
            int reg0 = e->var->id;
            return reg0;
        }

    case NOD_EXPR_FIELD:
        //LoadByte(code, e->field->offset);
        reg0 = e->field->offset;
        return reg0;

        /*
    case NOD_EXPR_SELECT:
        //if (optimize) {
        //    int base = 0;
        //    int offset = 0;
        //    if (inst->parser_eval_addr(base) && fld->EvalAddr(offset)) {
        //        if (inst->parser_ast_is_global())
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

    case NOD_EXPR_INDEX:
        //if (optimize) {
        //    int base = 0;
        //    long index = 0;
        //    if (ary->parser_eval_addr(base) && idx->Eval(index)) {
        //        if (ary->parser_ast_is_global())
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
        */

    case NOD_EXPR_DEREF:
        {
            int reg0 = gen_expr__(code, e->l);
            return reg0;
        }

    }

    return reg0;
}
static void gen_stmt__(Bytecode *code, const struct parser_stmt *s)
{
    if (!s)
        return;

    switch (s->kind) {

    case NOD_STMT_NOP:
        break;

    case NOD_STMT_BLOCK:
        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt__(code, stmt);
        break;

    case NOD_STMT_IF:
        BeginIf__(code);

        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt__(code, stmt);

        // exit
        BackPatchElseEnds__(code);
        break;

    case NOD_STMT_ELSE:
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
        break;

    case NOD_STMT_FOR:
        {
            // init
            BeginFor__(code);
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
        break;

    case NOD_STMT_BREAK:
        {
            Int addr = Jump__(code, -1);
            PushBreak__(code, addr);
        }
        break;

    case NOD_STMT_CONTINUE:
        {
            Int addr = Jump__(code, -1);
            //PushContinue__(code, addr);
            // TODO testing new naming convention
            code_push_continue(code, addr);
        }
        break;


    case NOD_STMT_SWITCH:
        {
            // init
            code_begin_switch(code);
            int curr = GetCurrentRegister__(code);
            int reg0 = GetNextRegister__(code, curr);
            int reg1 = gen_expr__(code, s->cond);
            reg0 = Move__(code, reg0, reg1);

            // cases
            for (struct parser_stmt *cas = s->children; cas; cas = cas->next) {
                // set current where expr is stored
                SetCurrentRegister__(code, reg0);
                gen_stmt__(code, cas);
            }

            // quit
            code_backpatch_case_ends(code);
#if TEST
            CODE_BackPatchCaseEnds(code);
#endif

            // remove cond val
            SetCurrentRegister__(code, curr);
        }
        break;

    case NOD_STMT_CASE:
        {
            // cond
            int reg1 = GetCurrentRegister__(code);

            // backpatch address for each expression
            struct data_intvec trues = {0};
            data_intvec_init(&trues);

            // eval conds
            for (struct parser_expr *cond = s->cond; cond; cond = cond->next) {
                // cond test
                int reg2 = gen_expr__(code, cond);
                int reg0 = GetNextRegister__(code, reg1);
                EqualInt__(code, reg0, reg1, reg2);

                // jump if true otherwise fallthrough
                Int tru = JumpIfNotZero__(code, reg0, -1);
                data_intvec_push(&trues, tru);
            }
            // all conds false -> close case
            Int exit = Jump__(code, -1);
            // one of cond true -> go to body
            for (int i = 0; i < trues.len; i++)
                BackPatch__(code, trues.data[i]);
            data_intvec_free(&trues);
#if TEST
            DATA_IntVecFree(&trues);
#endif

            // body
            gen_stmt__(code, s->body);

            // end
            Int addr = Jump__(code, -1);
            PushCaseEnd__(code, addr);
            BackPatch__(code, exit);
        }
        break;

    case NOD_STMT_DEFAULT:
        // body
        gen_stmt__(code, s->body);
        break;

    case NOD_STMT_RETURN:
        {
            int reg0 = gen_expr__(code, s->expr);
            Return__(code, reg0);
        }
        break;

    case NOD_STMT_EXPR:
        gen_expr__(code, s->expr);
        // remove the result
        //Pop(code);
        break;

        // XXX need NOD_STMT_ASSNSTMT?
    case NOD_STMT_ASSIGN:
    case NOD_STMT_INIT:
        gen_expr__(code, s->expr);
        break;
    }

    ResetCurrentRegister__(code);
}

static void gen_func__(Bytecode *code, const struct parser_func *func, int func_id)
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

static void gen_funcs__(Bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_funcs__(code, sym->module);
    }

    // self module next
    for (int i = 0; i < mod->funcs.len; i++) {
        struct parser_func *func = mod->funcs.data[i];
        if (!func->is_builtin)
            gen_func__(code, func, func->id);
    }
}

static void gen_gvars__(Bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    // imported modules first
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_gvars__(code, sym->module);
    }

    // self module next
    for (const struct parser_stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        gen_stmt__(code, gvar);
}

static void gen_module__(Bytecode *code, const struct parser_module *mod)
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
