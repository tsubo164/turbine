#include "parser.h"
#include "scope.h"
#include "token.h"
#include "type.h"
#include "prog.h"
#include "ast.h"
#include "mem.h"
#include "escseq.h"
#include "error.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>


#include "strbuf.h"
static const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    Strbuf sb = {0};
    while (fgets(buf, 1024, fp)) {
        StrbufCat(&sb, buf);
    }
    StrbufCat(&sb, "\n");

    fclose(fp);

    return sb.data;
}

typedef struct Parser {
    Scope *scope_;
    Prog *prog;
    Func *func_;
    const Token *curr_;
    const char *src_;
    const char *filename;
} Parser;

static void error(const Parser *p, Pos pos, const char *fmt, ...)
{
    va_list args; 
    va_start(args, fmt);
    VError(p->src_, p->filename, pos, fmt, args);
    va_end(args);
}

static const Token *curtok(const Parser *p)
{
    return p->curr_;
}

static const Token *gettok(Parser *p)
{
    p->curr_ = p->curr_->next;
    return p->curr_;
}

static void ungettok(Parser *p)
{
    p->curr_ = p->curr_->prev;
}

static Pos tok_pos(const Parser *p)
{
    return p->curr_->pos;
}

static long tok_int(const Parser *p)
{
    return p->curr_->ival;
}

static double tok_float(const Parser *p)
{
    return p->curr_->fval;
}

static const char *tok_str(const Parser *p)
{
    return p->curr_->sval;
}

static int peek(const Parser *p)
{
    if (p->curr_->kind == T_EOF)
        return T_EOF;
    else
        return p->curr_->next->kind;
}

static void expect(Parser *p, int kind)
{
    const Token *tok = gettok(p);
    if (tok->kind != kind) {
        error(p, tok->pos, "expected '%s'", TokenKindString(kind));
    }
}

static bool consume(Parser *p, int kind)
{
    if (peek(p) == kind) {
        gettok(p);
        return true;
    }
    else {
        return false;
    }
}

static void enter_scope(Parser *p, Func *func)
{
    if (func) {
        p->scope_ = func->scope;
    }
    else {
        p->scope_ = OpenChild(p->scope_);
    }
}

static void leave_scope(Parser *p)
{
    p->scope_ = p->scope_->parent;
}

// forward decls
static Type *type_spec(Parser *p);
static Expr *expression(Parser *p);
static Stmt *block_stmt(Parser *p, Func *func);

static Expr *arg_list(Parser *p, Expr *call)
{
    Expr head = {0};
    Expr *tail = &head;
    int count = 0;

    if (peek(p) != T_RPAREN) {
        do {
            tail = tail->next = expression(p);
            count++;
        }
        while (consume(p, T_COMMA));
    }

    const Func *func = call->l->type->func;
    if (HasSpecialVar(func)) {
        tail = tail->next = NewIntLitExpr(call->pos.y);
        count++;
    }

    call->list = head.next;

    const int argc = count;
    const int paramc = RequiredParamCount(func);
    if (argc < paramc)
        error(p, tok_pos(p), "too few arguments");

    const Expr *arg = call->list;
    for (int i = 0; i < argc; i++, arg = arg->next) {
        const Var *param = GetParam(func, i);

        if (!param)
            error(p, tok_pos(p), "too many arguments");

        // TODO arg needs to know its pos
        if (!MatchType(arg->type, param->type)) {
            error(p, tok_pos(p),
                    "type mismatch: parameter type '%s': argument type '%s'",
                    TypeString(param->type),
                    TypeString(arg->type));
        }
    }

    expect(p, T_RPAREN);

    return call;
}

static Expr *conv_expr(Parser *p, int kind)
{
    Type *to_type = type_spec(p);
    const Pos tokpos = tok_pos(p);

    expect(p, T_LPAREN);
    Expr *expr = expression(p);
    expect(p, T_RPAREN);

    switch (expr->type->kind) {
    case TY_BOOL:
    case TY_INT:
    case TY_FLOAT:
        break;
    default:
        error(p, tokpos,
                "unable to convert type from '%s' to '%s'",
                TypeString(expr->type),
                TypeString(to_type));
        break;
    }

    return NewConversionExpr(expr, to_type);
}

// primary_expr =
//     IntNum |
//     FpNum |
//     StringLit |
//     primary_expr selector
static Expr *primary_expr(Parser *p)
{
    if (consume(p, T_NIL))
        return NewNilLitExpr();

    if (consume(p, T_TRU))
        return NewBoolLitExpr(true);

    if (consume(p, T_FLS))
        return NewBoolLitExpr(false);

    if (consume(p, T_INTLIT))
        return NewIntLitExpr(tok_int(p));

    if (consume(p, T_FLTLIT))
        return NewFloatLitExpr(tok_float(p));

    if (consume(p, T_STRLIT)) {
        Expr *e = NewStringLitExpr(tok_str(p));
        const Token *tok = curtok(p);
        if (tok->has_escseq) {
            const int errpos = ConvertEscapeSequence(e->sval, &e->converted);
            if (errpos != -1) {
                printf("!! e->val.s [%s] errpos %d\n", e->sval, errpos);
                Pos pos = tok->pos;
                pos.x += errpos + 1;
                error(p, pos, "unknown escape sequence");
            }
        }
        return e;
    }

    if (consume(p, T_LPAREN)) {
        Expr *e = expression(p);
        expect(p, T_RPAREN);
        return e;
    }

    if (consume(p, T_CALLER_LINE)) {
        struct Symbol *sym = FindSymbol(p->scope_, tok_str(p));
        if (!sym) {
            error(p, tok_pos(p),
                    "special variable '%s' not declared in parameters",
                    tok_str(p));
        }
        return NewIdentExpr(sym);
    }

    const int next = peek(p);
    switch (next) {
    case T_BOL:
    case T_INT:
    case T_FLT:
        return conv_expr(p, next);
    default:
        break;
    }

    Expr *expr = NULL;

    for (;;) {
        const Token *tok = gettok(p);

        if (tok->kind == T_IDENT) {
            struct Symbol *sym = FindSymbol(p->scope_, tok->sval);
            if (!sym) {
                error(p, tok_pos(p),
                        "undefined identifier: '%s'",
                        tok_str(p));
            }
            expr = NewIdentExpr(sym);
            continue;
        }
        else if (tok->kind == T_LPAREN) {
            if (!expr || !IsFunc(expr->type)) {
                error(p, tok_pos(p),
                        "call operator must be used for function type");
            }
            // TODO func signature check
            Expr *call = NewCallExpr(expr, tok->pos);
            expr = arg_list(p, call);
            continue;
        }
        else if (tok->kind == T_DOT) {
            if (IsStruct(expr->type)) {
                expect(p, T_IDENT);
                Field *f = FindField(expr->type->strct, tok_str(p));
                expr = NewSelectExpr(expr, NewFieldExpr(f));
            }
            else if (IsTable(expr->type)) {
                expect(p, T_IDENT);
                Row *r = HashMapLookup(&expr->type->table->rows, tok_str(p));
                Expr *tmp = expr;
                expr = NewIntLitExpr(r->ival);
                expr->l = tmp;
            }
            else if (IsModule(expr->type)) {
                struct Scope *cur = p->scope_;
                p->scope_ = expr->type->module->scope;
                Expr *r = primary_expr(p);
                p->scope_ = cur;

                // TODO keep module expr somewhere
                expr = r;
            }
            else {
                error(p, tok_pos(p),
                        "dot operator must be used for struct or table type");
            }
            continue;
        }
        else if (tok->kind == T_LBRACK) {
            if (!IsArray(expr->type)) {
                error(p, tok_pos(p),
                        "index operator must be used for array type");
            }
            Expr *idx = expression(p);
            if (!IsInt(idx->type)) {
                error(p, tok_pos(p),
                        "index expression must be integer type");
            }
            long index = 0;
            if (EvalExpr(idx, &index)) {
                const long len = expr->type->len;
                if (index >= len) {
                    error(p, tok_pos(p),
                            "index out of range[%d] with length %d",
                            index, len);
                }
            }
            expect(p, T_RBRACK);
            return NewIndexExpr(expr, idx);
        }
        else {
            if (!expr) {
                error(p, tok->pos,
                        "unknown token for primary expression: \"%s\"",
                        TokenKindString(tok->kind));
            }

            ungettok(p);
            return expr;
        }
    }

    return NULL;
}

// unary_expr = primary_expr (unary_op primary_expr)*
// unary_op   = "+" | "-" | "!" | "~"
static Expr *unary_expr(Parser *p)
{
    const Token *tok = gettok(p);
    const int kind = tok->kind;

    if (kind == T_AND) {
        Expr *expr = unary_expr(p);
        Type *type = NewTypePtr(expr->type);
        return NewUnaryExpr(expr, type, kind);
    }
    if (kind == T_MUL) {
        Expr *expr = unary_expr(p);
        if (!IsPtr(expr->type)) {
            error(p, tok->pos,
                    "type mismatch: * must be used for pointer type");
        }
        Type *type = DuplicateType(expr->type->underlying);
        return NewUnaryExpr(expr, type, kind);
    }

    switch (kind) {
    case T_ADD:
    case T_SUB:
    case T_LNOT:
    case T_NOT:
        {
            Expr *e = unary_expr(p);
            return NewUnaryExpr(e, (Type*)(e->type), kind);
        }

    default:
        ungettok(p);
        return primary_expr(p);
    }
}

// mul_expr = unary_expr (mul_op unary_expr)*
// mul_op   = "*" | "/" | "%" | "&" | "<<" | ">>"
static Expr *mul_expr(Parser *p)
{
    Expr *L = unary_expr(p);
    Expr *R = NULL;

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_MUL:
        case T_DIV:
        case T_REM:
        case T_AND:
        case T_SHL:
        case T_SHR:
            R = unary_expr(p);
            if (!MatchType(L->type, R->type)) {
                error(p, tok->pos,
                        "type mismatch: %s and %s",
                        TypeString(L->type),
                        TypeString(R->type));
            }
            L = NewBinaryExpr(L, R, tok->kind);
            break;

        default:
            ungettok(p);
            return L;
        }
    }
}

// add_expr = mul_expr (add_op mul_expr)*
// add_op   = "+" | "-" | "|" | "^"
static Expr *add_expr(Parser *p)
{
    Expr *L = mul_expr(p);
    Expr *R = NULL;

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_ADD:
        case T_SUB:
        case T_OR:
        case T_XOR:
            R = mul_expr(p);
            if (!MatchType(L->type, R->type)) {
                error(p, tok->pos,
                        "type mismatch: %s and %s",
                        TypeString(L->type),
                        TypeString(R->type));
            }
            L = NewBinaryExpr(L, R, tok->kind);
            break;

        default:
            ungettok(p);
            return L;
        }
    }
}

// rel_expr = add_expr (rel_op add_expr)*
// rel_op   = "==" | "!=" | "<" | ">" | "<=" | ">="
static Expr *rel_expr(Parser *p)
{
    Expr *L = add_expr(p);
    Expr *R = NULL;

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_EQ:
        case T_NEQ:
        case T_LT:
        case T_GT:
        case T_LTE:
        case T_GTE:
            R = add_expr(p);
            if (!MatchType(L->type, R->type)) {
                error(p, tok->pos,
                        "type mismatch: %s and %s",
                        TypeString(L->type),
                        TypeString(R->type));
            }
            L = NewRelationalExpr(L, R, tok->kind);
            continue;

        default:
            ungettok(p);
            return L;
        }
    }
}

// logand_expr = rel_expr ("&&" rel_expr)*
static Expr *logand_expr(Parser *p)
{
    Expr *expr = rel_expr(p);

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_LAND:
            expr = NewBinaryExpr(expr, rel_expr(p), tok->kind);
            continue;

        default:
            ungettok(p);
            return expr;
        }
    }
}

// logor_expr = logand_expr ("||" logand_expr)*
static Expr *logor_expr(Parser *p)
{
    Expr *expr = logand_expr(p);

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_LOR:
            expr = NewBinaryExpr(expr, logand_expr(p), tok->kind);
            continue;

        default:
            ungettok(p);
            return expr;
        }
    }
}

// assign_expr = logand_expr assing_op expression
//             | logand_expr incdec_op
// assign_op   = "=" | "+=" | "-=" | "*=" | "/=" | "%="
// incdec_op   = "++" | "--"
static Expr *assign_expr(Parser *p)
{
    Expr *lval = logor_expr(p);
    Expr *rval = NULL;
    const Token *tok = gettok(p);
    const int kind = tok->kind;

    switch (kind) {
    case T_ASSN:
    case T_AADD:
    case T_ASUB:
    case T_AMUL:
    case T_ADIV:
    case T_AREM:
        rval = expression(p);
        if (!MatchType(lval->type, rval->type)) {
            error(p, tok->pos,
                    "type mismatch: l-value type '%s': r-value type '%s'",
                    TypeString(lval->type),
                    TypeString(rval->type));
        }
        return NewAssignExpr(lval, rval, kind);

    case T_INC:
    case T_DEC:
        if (!IsInt(lval->type)) {
            error(p, tok->pos,
                    "type mismatch: ++/-- must be used for int");
        }
        return NewIncDecExpr(lval, kind);

    default:
        ungettok(p);
        return lval;
    }
}

static Expr *expression(Parser *p)
{
    return assign_expr(p);
}

static Stmt *or_stmt(Parser *p)
{
    Expr *cond = NULL;

    if (consume(p, T_NEWLINE)) {
        // or (else)
        cond = NewNullExpr();
    }
    else {
        // or if (else if)
        cond = expression(p);
        expect(p, T_NEWLINE);
    }

    Stmt *body = block_stmt(p, (Func *)NULL);

    return NewOrStmt(cond, body);
}

static Stmt *if_stmt(Parser *p)
{
    expect(p, T_IF);
    Expr *cond = expression(p);
    expect(p, T_NEWLINE);

    Stmt head = {0};
    Stmt *tail = &head;

    Stmt *body = block_stmt(p, (Func *)NULL);
    tail = tail->next = NewOrStmt(cond, body);

    bool endor = false;

    while (!endor) {
        if (consume(p, T_ELS)) {
            if (peek(p) == T_NEWLINE) {
                // last 'or' (else)
                endor = true;
            }

            tail = tail->next = or_stmt(p);
        }
        else {
            endor = true;
        }
    }

    return NewIfStmt(head.next);
}

static Stmt *for_stmt(Parser *p)
{
    expect(p, T_FOR);

    Expr *init = NULL;
    Expr *cond = NULL;
    Expr *post = NULL;

    if (consume(p, T_NEWLINE)) {
        // infinite loop
        init = NewNullExpr();
        cond = NewIntLitExpr(1);
        post = NewNullExpr();
    }
    else {
        Expr *e = expression(p);

        if (consume(p, T_SEM)) {
            // traditional for
            init = e;
            cond = expression(p);
            expect(p, T_SEM);
            post = expression(p);
            expect(p, T_NEWLINE);
        }
        else if (consume(p, T_NEWLINE)) {
            // while style
            init = NewNullExpr();
            cond = e;
            post = NewNullExpr();
        }
        else {
            const Token *tok = gettok(p);
            error(p, tok->pos, "unknown token");
        }
    }

    // body
    Stmt *body = block_stmt(p, (Func *)NULL);
    return NewForStmt(init, cond, post, body);
}

static Stmt *jump_stmt(Parser *p)
{
    const Token *tok = gettok(p);
    const int kind = tok->kind;

    if (kind == T_BRK) {
    }
    else if (kind == T_CNT) {
    }

    expect(p, T_NEWLINE);

    return NewJumpStmt(kind);
}

static Stmt *case_stmt(Parser *p, int kind)
{
    Stmt head = {0};
    Stmt *tail = &head;

    if (kind == T_CASE) {
        do {
            Expr *expr = expression(p);
            // TODO const int check
            tail = tail->next = NewExprStmt(expr);
        }
        while (consume(p, T_COMMA));
    }
    else if (kind == T_DFLT) {
        tail = tail->next = NewExprStmt(NewNullExpr());
    }

    expect(p, T_NEWLINE);

    Stmt *body = block_stmt(p, (Func *)NULL);
    return NewCaseStmt(head.next, body, kind);
}

static Stmt *switch_stmt(Parser *p)
{
    expect(p, T_SWT);

    Expr *expr = expression(p);
    // TODO int check
    expect(p, T_NEWLINE);

    Stmt head = {0};
    Stmt *tail = &head;

    int default_count = 0;

    for (;;) {
        const Token *tok = gettok(p);

        switch (tok->kind) {
        case T_CASE:
            if (default_count > 0) {
                error(p, tok->pos, "No 'case' should come after 'default'");
            }
            tail = tail->next = case_stmt(p, tok->kind);
            continue;

        case T_DFLT:
            tail = tail->next = case_stmt(p, tok->kind);
            default_count++;
            continue;

        default:
            ungettok(p);
            return NewSwitchStmt(expr, head.next);
        }
    }
}

static Stmt *ret_stmt(Parser *p)
{
    expect(p, T_RET);

    const Pos exprpos = tok_pos(p);
    Expr *expr = NULL;

    if (consume(p, T_NEWLINE)) {
        expr = NewNullExpr();
    }
    else {
        expr = expression(p);
        expect(p, T_NEWLINE);
    }

    assert(p->func_);

    if (p->func_->return_type->kind != expr->type->kind) {
        error(p, exprpos,
                "type mismatch: function type '%s': expression type '%s'",
                TypeString(p->func_->return_type),
                TypeString(expr->type), "");
    }

    return NewReturnStmt(expr);
}

static Stmt *expr_stmt(Parser *p)
{
    Stmt *s = NewExprStmt(expression(p));
    expect(p, T_NEWLINE);

    return s;
}

static Stmt *scope_stmt(Parser *p)
{
    expect(p, T_DASH3);
    expect(p, T_NEWLINE);

    return block_stmt(p, (Func *)NULL);
}

static Stmt *nop_stmt(Parser *p)
{
    expect(p, T_NOP);

    Stmt *s = NewNopStmt();
    expect(p, T_NEWLINE);

    return s;
}

static Expr *default_value(const Type *type)
{
    switch (type->kind) {
    case TY_BOOL:
        return NewBoolLitExpr(false);
    case TY_INT:
        return NewIntLitExpr(0);
    case TY_FLOAT:
        return NewFloatLitExpr(0.0);
    case TY_STRING:
        return NewStringLitExpr("");

    case TY_PTR:
        return NewNilLitExpr();

    case TY_ARRAY:
        // TODO fill with zero values
        // put len at base addr
        return NewIntLitExpr(type->len);

    case TY_FUNC:
    case TY_STRUCT:
    case TY_TABLE:
    case TY_MODULE:
        // TODO
        return NewNilLitExpr();

    case TY_NIL:
    case TY_ANY:
        UNREACHABLE;
        return NULL;
    }
}

// var_decl = "-" identifier type newline
//          | "-" identifier type = expression newline
static Stmt *var_decl(Parser *p, bool isglobal)
{
    expect(p, T_SUB);
    expect(p, T_IDENT);

    // var anme
    const char *name = tok_str(p);
    const struct Pos ident_pos = tok_pos(p);
    Type *type = NULL;
    Expr *init = NULL;

    // type and init
    if (consume(p, T_ASSN)) {
        // "- x = 42"
        init = expression(p);
        type = DuplicateType(init->type);
    }
    else {
        type = type_spec(p);

        if (consume(p, T_ASSN)) {
            // "- x int = 42"
            init = expression(p);
        }
        else {
            // "- x int"
            init = default_value(type);
        }
    }

    expect(p, T_NEWLINE);

    struct Symbol *sym = DefineVar(p->scope_, name, type, isglobal);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }
    Expr *ident = NewIdentExpr(sym);
    return NewExprStmt(NewAssignExpr(ident, init, T_ASSN));
}

static void field_list(Parser *p, struct Struct *strct)
{
    expect(p, T_SUB);

    do {
        expect(p, T_IDENT);
        const char *name = tok_str(p);

        AddField(strct, name, type_spec(p));
        expect(p, T_NEWLINE);
    }
    while (consume(p, T_SUB));
}

static struct Struct *struct_decl(Parser *p)
{
    expect(p, T_HASH2);
    expect(p, T_IDENT);

    // struct name
    struct Struct *strct = DefineStruct(p->scope_, tok_str(p));
    if (!strct) {
        fprintf(stderr, "error: re-defined struct: '%s'\n", tok_str(p));
        exit(EXIT_FAILURE);
    }

    expect(p, T_NEWLINE);
    expect(p, T_BLOCKBEGIN);
    field_list(p, strct);
    expect(p, T_BLOCKEND);

    return strct;
}

static Table *table_def(Parser *p)
{
    expect(p, T_COLON2);
    expect(p, T_IDENT);

    Table *tab = DefineTable(p->scope_, tok_str(p));
    if (!tab) {
        error(p, tok_pos(p), "re-defined table: '%s'", tok_str(p));
    }
    expect(p, T_NEWLINE);

    expect(p, T_BLOCKBEGIN);
    int id = 0;
    for (;;) {
        if (consume(p, T_OR)) {
            expect(p, T_IDENT);
            Row *r = CALLOC(Row);
            r->name = tok_str(p);
            r->ival = id++;
            HashMapInsert(&tab->rows, tok_str(p), r);
            expect(p, T_NEWLINE);
        }
        else {
            break;
        }
    }
    expect(p, T_BLOCKEND);

    return tab;
}

static Stmt *block_stmt(Parser *p, Func *func)
{
    Stmt head = {0};
    Stmt *tail = &head;

    enter_scope(p, func);
    expect(p, T_BLOCKBEGIN);

    for (;;) {
        const int next = peek(p);

        if (next == T_SUB) {
            tail = tail->next = var_decl(p, false);
            continue;
        }
        else if (next == T_IF) {
            tail = tail->next = if_stmt(p);
            continue;
        }
        else if (next == T_FOR) {
            tail = tail->next = for_stmt(p);
            continue;
        }
        else if (next == T_BRK || next == T_CNT) {
            tail = tail->next = jump_stmt(p);
            continue;
        }
        else if (next == T_SWT) {
            tail = tail->next = switch_stmt(p);
            continue;
        }
        else if (next == T_RET) {
            tail = tail->next = ret_stmt(p);
            continue;
        }
        else if (next == T_DASH3) {
            tail = tail->next = scope_stmt(p);
            continue;
        }
        else if (next == T_NOP) {
            tail = tail->next = nop_stmt(p);
            continue;
        }
        else if (next == T_NEWLINE) {
            gettok(p);
            continue;
        }
        else if (next == T_BLOCKEND) {
            break;
        }
        else {
            tail = tail->next = expr_stmt(p);
            continue;
        }
    }

    expect(p, T_BLOCKEND);
    leave_scope(p);

    return NewBlockStmt(head.next);
}

static void param_list(Parser *p, Func *func)
{
    expect(p, T_LPAREN);

    if (consume(p, T_RPAREN))
        return;

    do {
        const Type *type = NULL;
        const char *name;

        if (consume(p, T_CALLER_LINE)) {
            name = tok_str(p);
            type = NewTypeInt();
        }
        else {
            expect(p, T_IDENT);
            name = tok_str(p);
            type = type_spec(p);
        }

        DeclareParam(func, name, type);
    }
    while (consume(p, T_COMMA));

    expect(p, T_RPAREN);
}

static void ret_type(Parser *p, Func *func)
{
    const int next = peek(p);

    if (next == T_NEWLINE)
        func->return_type = NewTypeNil();
    else
        func->return_type = type_spec(p);
}

// type_spec = "bool" | "int" | "float" | "string" | identifier
// func_type = "#" param_list type_spec?
static Type *type_spec(Parser *p)
{
    Type *type = NULL;

    if (consume(p, T_MUL)) {
        return NewTypePtr(type_spec(p));
    }

    if (consume(p, T_LBRACK)) {
        Expr *e = expression(p);
        if (!IsInt(e->type)) {
            error(p, tok_pos(p),
                    "array length expression must be integer type");
        }
        long len = 0;
        if (!EvalExpr(e, &len)) {
            error(p, tok_pos(p),
                    "array length expression must be compile time constant");
        }
        expect(p, T_RBRACK);
        return NewTypeArray(len, type_spec(p));
    }

    if (consume(p, T_HASH)) {
        Func *func = DeclareFunc(p->scope_, false);
        param_list(p, func);
        ret_type(p, func);
        return NewTypeFunc(func);
    }

    if (consume(p, T_BOL)) {
        type = NewTypeBool();
    }
    else if (consume(p, T_INT)) {
        type = NewTypeInt();
    }
    else if (consume(p, T_FLT)) {
        type = NewTypeFloat();
    }
    else if (consume(p, T_STR)) {
        type = NewTypeString();
    }
    else if (consume(p, T_IDENT)) {
        type = NewTypeStruct(FindStruct(p->scope_, tok_str(p)));
    }
    else {
        const Token *tok = gettok(p);
        error(p, tok->pos,
                "not a type name: '%s'",
                TokenKindString(tok->kind));
    }

    return type;
}

// func_def = "#" identifier param_list type_spec? newline block_stmt
static FuncDef *func_def(Parser *p)
{
    expect(p, T_HASH);
    expect(p, T_IDENT);

    // signature
    const char *name = tok_str(p);
    const struct Pos ident_pos = tok_pos(p);
    Func *func = DeclareFunc(p->scope_, false);

    // params
    param_list(p, func);
    ret_type(p, func);
    expect(p, T_NEWLINE);

    // func var
    struct Symbol *sym = DefineVar(p->scope_, name, NewTypeFunc(func), false);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }

    // func body
    p->func_ = func;

    Stmt *body = block_stmt(p, func);
    // TODO control flow check to allow implicit return
    for (Stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = NewReturnStmt(NewNullExpr());
            break;
        }
    }

    p->func_ = NULL;

    // XXX temp
    FuncDef *fdef = NewFuncDef(sym, body);
    fdef->funclit_id = p->prog->funclit_id++;

    return fdef;
}

static void module_import(struct Parser *p)
{
    expect(p, T_LBRACK);
    expect(p, T_IDENT);
    const char *name = tok_str(p);

    char buf[128] = {'\0'};
    if (strlen(name) > 120) {
        error(p, tok_pos(p),
                "error: too long module name: '%s'", name);
    }

    // TODO have search paths
    const char *src = NULL;
    if (!src) {
        sprintf(buf, "src/%s.ro", name);
        src = read_file(buf);
    }
    if (!src) {
        sprintf(buf, "%s.ro", name);
        src = read_file(buf);
    }
    if (!src) {
        error(p, tok_pos(p),
                "module %s.ro not found", name);
    }

    struct Module *mod = DefineModule(p->scope_, name);
    const struct Token *tok = Tokenize(src);
    // TODO use this style => enter_scope(p, mod->scope);
    Parse(src, tok, mod->scope, p->prog);
    // TODO come up with better way to take over var id from child
    // Maybe need one more pass to fill id
    p->scope_->var_offset_ = mod->scope->var_offset_;

    expect(p, T_RBRACK);
    expect(p, T_NEWLINE);
}

static void program(Parser *p)
{
    Prog *prog = p->prog;

    Stmt head = {0};
    Stmt *tail = &head;

    for (;;) {
        const int next = peek(p);

        if (next == T_HASH) {
            FuncDef *fdef = func_def(p);

            // TODO remove this
            if (!strcmp(fdef->var->name, "main"))
                prog->main_func = fdef->var;

            {
                // TODO clean up
                Expr *ident = NewIdentExpr(fdef->sym);
                Expr *init = NewIntLitExpr(fdef->funclit_id);
                tail = tail->next = NewExprStmt(NewAssignExpr(ident, init, T_ASSN));
            }

            VecPush(&prog->funcv, fdef);
            continue;
        }

        if (next == T_HASH2) {
            struct_decl(p);
            continue;
        }

        if (next == T_COLON2) {
            table_def(p);
            continue;
        }

        if (next == T_SUB) {
            tail = tail->next = var_decl(p, true);
            continue;
        }

        if (next == T_LBRACK) {
            module_import(p);
            continue;
        }

        if (next == T_NEWLINE) {
            gettok(p);
            continue;
        }

        if (next == T_EOF) {
            break;
        }

        const Token *tok = gettok(p);
        error(p, tok->pos,
                "error: unexpected token for global object: '%s'",
                TokenKindString(next));
    }

    prog->gvars = head.next;
}

void Parse(const char *src, const Token *tok, Scope *scope, Prog *prog)
{
    static const char filename[] = "fixme.ro";

    Parser p = {0};

    p.src_ = src;
    p.curr_ = tok;
    p.scope_ = scope;
    p.prog = prog;
    p.func_ = NULL;
    p.filename = filename;

    program(&p);
}
