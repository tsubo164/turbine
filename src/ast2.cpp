#include "compiler.h"

#include "ast.h"

static const TokInfo table[] = {
    { T_NUL,     "nul" },
    // type
    { T_NIL,     "nil" },
    { T_BOL,     "bool" },
    { T_INT,     "int" },
    { T_FLT,     "float" },
    // stmt
    { T_IF,      "if" },
    { T_FOR,     "for" },
    { T_ELS,     "or" },
    { T_BRK,     "break" },
    { T_CNT,     "continue" },
    { T_SWT,     "switch" },
    { T_CASE,    "case" },
    { T_DFLT,    "default" },
    { T_RET,     "return" },
    { T_NOP,     "nop" },
    { T_EXPR,    "expr" },
    { T_BLOCK,   "block" },
    { T_END_OF_KEYWORD,   "end_of_keyword" },
    // identifier
    { T_FIELD,   "field",  'v' },
    { T_IDENT,   "ident",  'v' },
    { T_FUNC,    "func",   'v' },
    { T_VAR,     "var",    'v' },
    // literal
    { T_NILLIT,  "nil_lit" },
    { T_BOLLIT,  "bool_lit",    'i' },
    { T_INTLIT,  "int_lit",     'i' },
    { T_FLTLIT,  "float_lit",   'f' },
    { T_STRLIT,  "string_lit",  's' },
    // separator
    { T_LPAREN,  "(" },
    { T_RPAREN,  ")" },
    { T_SEM,     ";" },
    // binop
    { T_ADD,     "+" },
    { T_SUB,     "-" },
    { T_MUL,     "*" },
    { T_DIV,     "/" },
    { T_REM,     "%" },
    //
    { T_EQ,      "==" },
    { T_NEQ,     "!=" },
    { T_LT,      "<" },
    { T_LTE,     "<=" },
    { T_GT,      ">" },
    { T_GTE,     ">=" },
    //
    { T_SHL,     "<<" },
    { T_SHR,     ">>" },
    { T_OR,      "|" },
    { T_XOR,     "^" },
    { T_AND,     "&" },
    { T_LOR,     "||" },
    { T_LAND,    "&&" },
    //
    { T_SELECT,  "." },
    { T_INDEX,   "[]" },
    { T_CALL,    "call" },
    // unary
    { T_LNOT,    "!" },
    { T_POS,     "+" },
    { T_NEG,     "-" },
    { T_ADR,     "&" },
    { T_DRF,     "*" },
    { T_NOT,     "~" },
    { T_INC,     "++" },
    { T_DEC,     "--" },
    { T_CONV,    "conversion" },
    // assign
    { T_ASSN,    "=" },
    { T_AADD,    "+=" },
    { T_ASUB,    "-=" },
    { T_AMUL,    "*=" },
    { T_ADIV,    "/=" },
    { T_AREM,    "%=" },
    // eof
    { T_EOF,     "eof" },
};

// make an array of size 1 if table covers all kinds
// other wise size -1 which leads to compile error
// to avoid missing string impl of tokens
#define MISSING_TOKEN_STRING_IMPL \
    (sizeof(table)/sizeof(table[0])==T_EOF+1?1:-1)
//static const int assert_impl[MISSING_TOKEN_STRING_IMPL] = {0};

const TokInfo *find_tokinfo(int kind)
{
    int N = sizeof(table)/sizeof(table[0]);
    int i;

    for (i = 0; i < N; i++) {
        if (kind == table[i].kind)
            return &table[i];
    }
    return &table[0];
}

bool IsNull(const Expr *e)
{
    return e->kind == T_NUL;
}

bool IsGlobal(const Expr *e)
{
    switch (e->kind) {
    case T_IDENT:
        return e->var->is_global;

    case T_SELECT:
        return IsGlobal(e->l);

    default:
        return false;
    }
}

int Addr(const Expr *e)
{
    switch (e->kind) {
    case T_IDENT:
        return e->var->id;

    case T_FIELD:
        return e->fld->id;

    case T_SELECT:
        return Addr(e->l) + Addr(e->r);

    default:
        return -1;
    }
}

static bool eval_binary(const Expr *e, long *result)
{
    long L = 0, R = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    if (!EvalExpr(e->r, &R))
        return false;

    switch (e->kind) {
    case T_ADD: *result = L + R; return true;
    case T_SUB: *result = L - R; return true;
    case T_MUL: *result = L * R; return true;
    case T_DIV: *result = L / R; return true;
    case T_REM: *result = L % R; return true;
    default: return false;
    }
}

static bool eval_unary(const Expr *e, long *result)
{
    long L = 0;

    if (!EvalExpr(e->l, &L))
        return false;

    switch (e->kind) {
    case T_POS:  *result = +L; return true;
    case T_NEG:  *result = -L; return true;
    case T_LNOT: *result = !L; return true;
    case T_NOT:  *result = ~L; return true;
    default: return false;
    }
}

bool EvalExpr(const Expr *e, long *result)
{
    switch (e->kind) {
    case T_INTLIT:
        *result = e->val.i;
        return true;

    case T_ADD: case T_SUB:
    case T_MUL: case T_DIV: case T_REM:
        return eval_binary(e, result);

    case T_POS: case T_NEG:
    case T_LNOT: case T_NOT:
        return eval_unary(e, result);

    default:
        return false;
    }
}

bool EvalAddr(const Expr *e, int *result)
{
    switch (e->kind) {
    case T_IDENT:
        *result = e->var->id;
        return true;

    case T_FIELD:
        *result = e->fld->id;
        return true;

    default:
        return false;
    }
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
        for (auto arg: e->args) {
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
        code->LoadByte(e->args.size());
    }
    else {
        for (auto arg: e->args)
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
void gen_expr(Bytecode *code, const Expr *e)
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

            if (e->converted.empty())
                s = e->val.sv;
            else
                s = std::string_view(e->converted.c_str(), e->converted.length());

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

void gen_addr(Bytecode *code, const Expr *e)
{
    switch (e->kind) {

    case T_IDENT:
        if (e->var->is_global)
            code->LoadByte(e->var->id + 1);
        else
            code->LoadAddress(e->var->id);
        return;

    case T_FIELD:
        code->LoadByte(e->fld->id);
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

void gen_stmt(Bytecode *code, const Stmt *s)
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
            gen_expr(code, s->init);

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
            for (auto &cond: s->conds) {
                Int tru = 0;
                Int fls = 0;
                code->DuplicateTop();
                gen_expr(code, cond);
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
        for (const auto &cs: s->cases)
            gen_stmt(code, cs);

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

void print_expr(const Expr *e, int depth)
{
    const TokInfo *info;
    int i;

    if (!e || e->kind == T_NUL)
        return;

    // indentation
    for (i = 0; i < depth; i++) {
        printf("  ");
    }

    // basic info
    info = find_tokinfo(e->kind);
    printf("%d. <%s>", depth, info->str);

    // extra value
    switch (info->type) {
    case 'i':
        printf(" (%ld)", e->val.i);
        break;
    case 'f':
        printf(" (%g)", e->val.f);
        break;
    case 's':
        printf(" (%s)", std::string(e->val.sv).c_str());
        break;
    case 'v':
        printf(" (%s)", std::string(e->var->name).c_str());
        break;
    }
    printf("\n");

    // children
    if (e->l)
        print_expr(e->l, depth + 1);
    if (e->r)
        print_expr(e->r, depth + 1);
}

void PrintStmt(const Stmt *s, int depth)
{
    const TokInfo *info;
    int i;

    if (!s)
        return;

    // indentation
    for (i = 0; i < depth; i++)
        printf("  ");

    // basic info
    info = find_tokinfo(s->kind);
    printf("%d. <%s>", depth, info->str);
    printf("\n");

    // children
    for (Stmt *stmt = s->children; stmt; stmt = stmt->next)
        PrintStmt(stmt, depth + 1);

    for (const auto &cs: s->cases)
        PrintStmt(cs, depth + 1);

    for (auto &cond: s->conds)
        print_expr(cond, depth + 1);

    print_expr(s->expr, depth + 1);
    print_expr(s->init, depth + 1);
    print_expr(s->cond, depth + 1);
    print_expr(s->post, depth + 1);
    PrintStmt(s->body, depth + 1);
}
