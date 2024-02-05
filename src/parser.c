#include "parser.h"
#include "scope.h"
#include "token.h"
#include "type.h"
#include "ast.h"
#include "mem.h"
#include "escseq.h"
#include "error.h"

#include <assert.h>
#include <string.h>
#include <stdarg.h>
#include <stdlib.h>
#include <stdio.h>

// TODO
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
    struct Scope *scope; // current scope
    struct Func *func_;
    const struct Token *curr_;
    const char *src_;
    const char *filename;

    struct Module *module;
} Parser;

static void error(const Parser *p, struct Pos pos, const char *fmt, ...)
{
    va_list args; 
    va_start(args, fmt);
    VError(p->src_, p->filename, pos, fmt, args);
    va_end(args);
}

static const struct Token *curtok(const Parser *p)
{
    return p->curr_;
}

static const struct Token *gettok(Parser *p)
{
    p->curr_ = p->curr_->next;
    return p->curr_;
}

static void ungettok(Parser *p)
{
    p->curr_ = p->curr_->prev;
}

static struct Pos tok_pos(const Parser *p)
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
    const struct Token *tok = gettok(p);
    if (tok->kind != kind) {
        error(p, tok->pos, "expected '%s'", TokenString(kind));
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

// forward decls
static Type *type_spec(Parser *p);
static struct Expr *expression(Parser *p);
static struct Stmt *block_stmt(Parser *p, struct Scope *block_scope);

static struct Expr *arg_list(Parser *p, struct Expr *call)
{
    struct Expr head = {0};
    struct Expr *tail = &head;
    int count = 0;

    if (peek(p) != T_RPAREN) {
        do {
            tail = tail->next = expression(p);
            count++;
        }
        while (consume(p, T_COMMA));
    }

    const Func *func = call->l->type->func;
    if (func->has_special_var) {
        tail = tail->next = NewIntLitExpr(call->pos.y);
        count++;
    }

    call->list = head.next;

    const int argc = count;
    const int paramc = RequiredParamCount(func);
    if (argc < paramc)
        error(p, tok_pos(p), "too few arguments");

    const struct Expr *arg = call->list;
    for (int i = 0; i < argc; i++, arg = arg->next) {
        const struct Var *param = GetParam(func, i);

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

static struct Expr *conv_expr(Parser *p, int kind)
{
    Type *to_type = type_spec(p);
    const struct Pos tokpos = tok_pos(p);

    expect(p, T_LPAREN);
    struct Expr *expr = expression(p);
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
static struct Expr *primary_expr(Parser *p)
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
        struct Expr *e = NewStringLitExpr(tok_str(p));
        const struct Token *tok = curtok(p);
        if (tok->has_escseq) {
            const int errpos = ConvertEscapeSequence(e->sval, &e->converted);
            if (errpos != -1) {
                printf("!! e->val.s [%s] errpos %d\n", e->sval, errpos);
                struct Pos pos = tok->pos;
                pos.x += errpos + 1;
                error(p, pos, "unknown escape sequence");
            }
        }
        return e;
    }

    if (consume(p, T_LPAREN)) {
        struct Expr *e = expression(p);
        expect(p, T_RPAREN);
        return e;
    }

    if (consume(p, T_CALLER_LINE)) {
        struct Symbol *sym = FindSymbol(p->scope, tok_str(p));
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

    struct Expr *expr = NULL;

    for (;;) {
        const struct Token *tok = gettok(p);

        if (tok->kind == T_IDENT) {
            struct Symbol *sym = FindSymbol(p->scope, tok->sval);
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
            struct Expr *call = NewCallExpr(expr, tok->pos);
            expr = arg_list(p, call);
            continue;
        }
        else if (tok->kind == T_DOT) {
            if (IsStruct(expr->type)) {
                expect(p, T_IDENT);
                struct Field *f = FindField(expr->type->strct, tok_str(p));
                expr = NewSelectExpr(expr, NewFieldExpr(f));
            }
            else if (IsTable(expr->type)) {
                expect(p, T_IDENT);
                struct MapEntry *ent = HashMapLookup(&expr->type->table->rows, tok_str(p));
                struct Row *r = ent->val;
                struct Expr *tmp = expr;
                expr = NewIntLitExpr(r->ival);
                expr->l = tmp;
            }
            else if (IsModule(expr->type)) {
                struct Scope *cur = p->scope;
                p->scope = expr->type->module->scope;
                struct Expr *r = primary_expr(p);
                p->scope = cur;

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
            struct Expr *idx = expression(p);
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
                        TokenString(tok->kind));
            }

            ungettok(p);
            return expr;
        }
    }

    return NULL;
}

// unary_expr = primary_expr (unary_op primary_expr)*
// unary_op   = "+" | "-" | "!" | "~"
static struct Expr *unary_expr(Parser *p)
{
    const struct Token *tok = gettok(p);
    const int kind = tok->kind;

    if (kind == T_AND) {
        struct Expr *expr = unary_expr(p);
        Type *type = NewPtrType(expr->type);
        return NewUnaryExpr(expr, type, kind);
    }
    if (kind == T_MUL) {
        struct Expr *expr = unary_expr(p);
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
            struct Expr *e = unary_expr(p);
            return NewUnaryExpr(e, (Type*)(e->type), kind);
        }

    default:
        ungettok(p);
        return primary_expr(p);
    }
}

// mul_expr = unary_expr (mul_op unary_expr)*
// mul_op   = "*" | "/" | "%" | "&" | "<<" | ">>"
static struct Expr *mul_expr(Parser *p)
{
    struct Expr *L = unary_expr(p);
    struct Expr *R = NULL;

    for (;;) {
        const struct Token *tok = gettok(p);

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
static struct Expr *add_expr(Parser *p)
{
    struct Expr *L = mul_expr(p);
    struct Expr *R = NULL;

    for (;;) {
        const struct Token *tok = gettok(p);

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
static struct Expr *rel_expr(Parser *p)
{
    struct Expr *L = add_expr(p);
    struct Expr *R = NULL;

    for (;;) {
        const struct Token *tok = gettok(p);

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
static struct Expr *logand_expr(Parser *p)
{
    struct Expr *expr = rel_expr(p);

    for (;;) {
        const struct Token *tok = gettok(p);

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
static struct Expr *logor_expr(Parser *p)
{
    struct Expr *expr = logand_expr(p);

    for (;;) {
        const struct Token *tok = gettok(p);

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
static struct Expr *assign_expr(Parser *p)
{
    struct Expr *lval = logor_expr(p);
    struct Expr *rval = NULL;
    const struct Token *tok = gettok(p);
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

static struct Expr *expression(Parser *p)
{
    return assign_expr(p);
}

static struct Scope *new_child_scope(struct Parser *p)
{
    struct Scope *parent = p->scope;
    struct Scope *child = NewScope(parent, parent->cur_offset);

    if (!parent->children_)
        parent->child_tail = parent->children_ = child;
    else
        parent->child_tail = parent->child_tail->next = child;

    return child;
}

static struct Stmt *or_stmt(Parser *p)
{
    struct Expr *cond = NULL;

    if (consume(p, T_NEWLINE)) {
        // or (else)
        cond = NewNullExpr();
    }
    else {
        // or if (else if)
        cond = expression(p);
        expect(p, T_NEWLINE);
    }

    struct Stmt *body = block_stmt(p, new_child_scope(p));

    return NewOrStmt(cond, body);
}

static struct Stmt *if_stmt(Parser *p)
{
    expect(p, T_IF);
    struct Expr *cond = expression(p);
    expect(p, T_NEWLINE);

    struct Stmt head = {0};
    struct Stmt *tail = &head;

    struct Stmt *body = block_stmt(p, new_child_scope(p));
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

static struct Stmt *for_stmt(Parser *p)
{
    expect(p, T_FOR);

    struct Expr *init = NULL;
    struct Expr *cond = NULL;
    struct Expr *post = NULL;

    if (consume(p, T_NEWLINE)) {
        // infinite loop
        init = NewNullExpr();
        cond = NewIntLitExpr(1);
        post = NewNullExpr();
    }
    else {
        struct Expr *e = expression(p);

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
            const struct Token *tok = gettok(p);
            error(p, tok->pos, "unknown token");
        }
    }

    // body
    struct Stmt *body = block_stmt(p, new_child_scope(p));
    return NewForStmt(init, cond, post, body);
}

static struct Stmt *jump_stmt(Parser *p)
{
    const struct Token *tok = gettok(p);
    const int kind = tok->kind;

    if (kind == T_BRK) {
    }
    else if (kind == T_CNT) {
    }

    expect(p, T_NEWLINE);

    return NewJumpStmt(kind);
}

static struct Stmt *case_stmt(Parser *p, int kind)
{
    struct Stmt head = {0};
    struct Stmt *tail = &head;

    if (kind == T_CASE) {
        do {
            struct Expr *expr = expression(p);
            // TODO const int check
            tail = tail->next = NewExprStmt(expr);
        }
        while (consume(p, T_COMMA));
    }
    else if (kind == T_DFLT) {
        tail = tail->next = NewExprStmt(NewNullExpr());
    }

    expect(p, T_NEWLINE);

    struct Stmt *body = block_stmt(p, new_child_scope(p));
    return NewCaseStmt(head.next, body, kind);
}

static struct Stmt *switch_stmt(Parser *p)
{
    expect(p, T_SWT);

    struct Expr *expr = expression(p);
    // TODO int check
    expect(p, T_NEWLINE);

    struct Stmt head = {0};
    struct Stmt *tail = &head;

    int default_count = 0;

    for (;;) {
        const struct Token *tok = gettok(p);

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

static struct Stmt *ret_stmt(Parser *p)
{
    expect(p, T_RET);

    const struct Pos exprpos = tok_pos(p);
    struct Expr *expr = NULL;

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

static struct Stmt *expr_stmt(Parser *p)
{
    struct Stmt *s = NewExprStmt(expression(p));
    expect(p, T_NEWLINE);

    return s;
}

static struct Stmt *scope_stmt(Parser *p)
{
    expect(p, T_DASH3);
    expect(p, T_NEWLINE);

    return block_stmt(p, new_child_scope(p));
}

static struct Stmt *nop_stmt(Parser *p)
{
    expect(p, T_NOP);

    struct Stmt *s = NewNopStmt();
    expect(p, T_NEWLINE);

    return s;
}

static struct Expr *default_value(const Type *type)
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
static struct Stmt *var_decl(Parser *p, bool isglobal)
{
    expect(p, T_SUB);
    expect(p, T_IDENT);

    // var anme
    const char *name = tok_str(p);
    const struct Pos ident_pos = tok_pos(p);
    Type *type = NULL;
    struct Expr *init = NULL;

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

    struct Symbol *sym = DefineVar(p->scope, name, type, isglobal);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }
    struct Expr *ident = NewIdentExpr(sym);
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
    struct Struct *strct = DefineStruct(p->scope, tok_str(p));
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

static struct Table *table_def(Parser *p)
{
    expect(p, T_COLON2);
    expect(p, T_IDENT);

    struct Table *tab = DefineTable(p->scope, tok_str(p));
    if (!tab) {
        error(p, tok_pos(p), "re-defined table: '%s'", tok_str(p));
    }
    expect(p, T_NEWLINE);

    expect(p, T_BLOCKBEGIN);
    int id = 0;
    for (;;) {
        if (consume(p, T_OR)) {
            expect(p, T_IDENT);
            struct Row *r = CALLOC(struct Row);
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

static struct Stmt *block_stmt(Parser *p, struct Scope *block_scope)
{
    struct Stmt head = {0};
    struct Stmt *tail = &head;

    // enter scope
    p->scope = block_scope;
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

    // leave scope
    p->scope = p->scope->parent;
    expect(p, T_BLOCKEND);

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
            type = NewIntType();
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
        func->return_type = NewNilType();
    else
        func->return_type = type_spec(p);
}

// type_spec = "bool" | "int" | "float" | "string" | identifier
// func_type = "#" param_list type_spec?
static Type *type_spec(Parser *p)
{
    Type *type = NULL;

    if (consume(p, T_MUL)) {
        return NewPtrType(type_spec(p));
    }

    if (consume(p, T_LBRACK)) {
        struct Expr *e = expression(p);
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
        return NewArrayType(len, type_spec(p));
    }

    if (consume(p, T_HASH)) {
        Func *func = AddFunc(p->scope, p->module->filename, "_");
        func->id = p->module->funcs.len;
        VecPush(&p->module->funcs, func);
        param_list(p, func);
        ret_type(p, func);
        return NewFuncType(func);
    }

    if (consume(p, T_BOL)) {
        type = NewBoolType();
    }
    else if (consume(p, T_INT)) {
        type = NewIntType();
    }
    else if (consume(p, T_FLT)) {
        type = NewFloatType();
    }
    else if (consume(p, T_STR)) {
        type = NewStringType();
    }
    else if (consume(p, T_IDENT)) {
        type = NewStructType(FindStruct(p->scope, tok_str(p)));
    }
    else {
        const struct Token *tok = gettok(p);
        error(p, tok->pos,
                "not a type name: '%s'",
                TokenString(tok->kind));
    }

    return type;
}

// func_def = "#" identifier param_list type_spec? newline block_stmt
static struct Stmt *func_def(struct Parser *p)
{
    expect(p, T_HASH);
    expect(p, T_IDENT);

    // func
    const char *name = tok_str(p);
    const struct Pos ident_pos = tok_pos(p);
    Func *func = AddFunc(p->scope, p->module->filename, name);
    func->id = p->module->funcs.len;
    VecPush(&p->module->funcs, func);

    // params
    param_list(p, func);
    ret_type(p, func);
    expect(p, T_NEWLINE);

    // func var
    struct Symbol *sym = DefineVar(p->scope, name, NewFuncType(func), true);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }

    // func body
    p->func_ = func;
    struct Stmt *body = block_stmt(p, func->scope);
    // TODO control flow check to allow implicit return
    for (struct Stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = NewReturnStmt(NewNullExpr());
            break;
        }
    }
    func->body = body;
    p->func_ = NULL;

    // TODO remove this
    if (!strcmp(sym->name, "main"))
        p->module->main_func = sym->var;

    struct Expr *ident = NewIdentExpr(sym);
    struct Expr *init = NewFuncLitExpr(func);
    return NewExprStmt(NewAssignExpr(ident, init, T_ASSN));
}

static void module_import(struct Parser *p)
{
    expect(p, T_LBRACK);
    expect(p, T_IDENT);
    const char *modulename = tok_str(p);

    char filename[512] = {'\0'};
    if (strlen(modulename) > 500) {
        error(p, tok_pos(p),
                "error: too long module name: '%s'", modulename);
    }

    // TODO have search paths
    const char *src = NULL;
    if (!src) {
        sprintf(filename, "src/%s.ro", modulename);
        src = read_file(filename);
    }
    if (!src) {
        sprintf(filename, "%s.ro", modulename);
        src = read_file(filename);
    }
    if (!src) {
        error(p, tok_pos(p),
                "module %s.ro not found", modulename);
    }

    const struct Token *tok = Tokenize(src);
    // TODO use this style => enter_scope(p, mod->scope);
    struct Module *mod = Parse(src, filename, modulename, tok, p->scope);
    // TODO come up with better way to take over var id from child
    // Maybe need one more pass to fill id
    p->scope->cur_offset = mod->scope->cur_offset;

    expect(p, T_RBRACK);
    expect(p, T_NEWLINE);
}

static void program(Parser *p)
{
    struct Stmt head = {0};
    struct Stmt *tail = &head;

    for (;;) {
        const int next = peek(p);

        if (next == T_HASH) {
            tail = tail->next = func_def(p);
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

        const struct Token *tok = gettok(p);
        error(p, tok->pos,
                "error: unexpected token for global object: '%s'",
                TokenString(next));
    }

    p->module->gvars = head.next;
}

struct Module *Parse(const char *src, const char *filename, const char *modulename,
        const struct Token *tok, struct Scope *scope)
{
    struct Module *mod = DefineModule(scope, filename, modulename);

    Parser p = {0};

    p.src_ = src;
    p.curr_ = tok;
    p.scope = mod->scope;
    p.func_ = NULL;
    p.filename = filename;
    p.module = mod;

    program(&p);
    return mod;
}
