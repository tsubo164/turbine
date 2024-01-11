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
    // list
    { T_EXPRLIST, "expr_list" },
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
        return e->field->id;

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
        *result = e->field->id;
        return true;

    default:
        return false;
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

    print_expr(s->expr, depth + 1);
    print_expr(s->cond, depth + 1);
    print_expr(s->post, depth + 1);
    PrintStmt(s->body, depth + 1);
}

void print_funcdef(const FuncDef *f, int depth)
{
    if (!f)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <func_def> \"%s\"", depth, std::string(f->var->name).c_str());
    printf(" %s", TypeString(f->var->type->func->return_type).c_str());
    printf("\n");

    // children
    PrintStmt(f->body, depth + 1);
}

void print_prog(const Prog *p, int depth)
{
    if (!p)
        return;

    // indentation
    for (int i = 0; i < depth; i++)
        printf("  ");

    // basic info
    printf("%d. <prog>", depth);
    printf("\n");

    // children
    for (const Stmt *gvar = p->gvars; gvar; gvar = gvar->next)
        PrintStmt(gvar, depth + 1);

    for (const FuncDef *func = p->funcs; func; func = func->next)
        print_funcdef(func, depth + 1);
}
