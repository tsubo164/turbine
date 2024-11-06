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
#include "data_strbuf.h"
static const char *read_file(const char *filename)
{
    FILE *fp = fopen(filename, "r");

    if (!fp)
        return NULL;

    char buf[1024] = {'\0'};
    struct data_strbuf sb = DATA_STRBUF_INIT;
    while (fgets(buf, 1024, fp)) {
        data_strbuf_cat(&sb, buf);
    }
    data_strbuf_cat(&sb, "\n");

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
static struct Type *type_spec(Parser *p);
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

    const struct FuncType *func_type = call->l->type->func_type;
    if (func_type->has_special_var) {
        tail = tail->next = NewIntLitExpr(call->pos.y);
        count++;
    }

    call->r = head.next;

    const int argc = count;
    const int paramc = RequiredParamCount(func_type);
    if (argc < paramc)
        error(p, tok_pos(p), "too few arguments");

    const struct Expr *arg = call->r;
    for (int i = 0; i < argc; i++, arg = arg->next) {
        const struct Type *param_type = GetParamType(func_type, i);

        if (!param_type)
            error(p, tok_pos(p), "too many arguments");

        // TODO arg needs to know its pos
        if (!MatchType(arg->type, param_type)) {
            error(p, tok_pos(p),
                    "type mismatch: parameter type '%s': argument type '%s'",
                    TypeString(param_type),
                    TypeString(arg->type));
        }
    }

    expect(p, T_RPAREN);

    return call;
}

static struct Expr *conv_expr(Parser *p, int kind)
{
    struct Type *to_type = type_spec(p);
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

static struct Expr *array_lit_expr(Parser *p)
{
    struct Expr *expr = expression(p);
    struct Expr *e = expr;
    const struct Type *elem_type = expr->type;
    int len = 1;

    while (consume(p, T_COMMA)) {
        e = e->next = expression(p);
        if (!MatchType(elem_type, e->type)) {
            error(p, tok_pos(p),
                    "type mismatch: first element type '%s': this element type '%s'",
                    TypeString(elem_type),
                    TypeString(e->type));
        }
        len++;
    }
    expect(p, T_RBRACK);

    return NewArrayLitExpr(expr, len);
}

static struct Expr *struct_lit_expr(struct Parser *p, struct Symbol *sym)
{
    struct Struct *strct = sym->strct;
    struct Expr *elems = NULL;
    struct Expr *e = NULL;

    expect(p, T_LBRACE);

    do {
        expect(p, T_IDENT);
        struct Field *field = FindField(strct, tok_str(p));
        if (!field) {
            error(p, tok_pos(p),
                    "struct '%s' has no field '%s'", strct->name, tok_str(p));
        }
        expect(p, T_ASSN);

        struct Expr *f = NewFieldExpr(field);
        struct Expr *elem = NewElementExpr(f, expression(p));

        //struct Expr *expr = expression(p);
        struct Expr *expr = elem;
        if (!e)
            e = elems = expr;
        else
            e = e->next = expr;
    }
    while (consume(p, T_COMMA));

    expect(p, T_RBRACE);
    return NewStructLitExpr(strct, elems);
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

    if (consume(p, T_LBRACK)) {
        return array_lit_expr(p);
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
            if (sym->kind == SYM_FUNC)
                expr = NewFuncLitExpr(sym->func);
            else if (sym->kind == SYM_STRUCT)
                expr = struct_lit_expr(p, sym);
            else
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
            else if (IsPtr(expr->type) && IsStruct(expr->type->underlying)) {
                expect(p, T_IDENT);
                struct Field *f = FindField(expr->type->underlying->strct, tok_str(p));
                expr = NewSelectExpr(expr, NewFieldExpr(f));
            }
            else if (IsTable(expr->type)) {
                expect(p, T_IDENT);
                struct data_hashmap_entry *ent =
                    data_hashmap_lookup(&expr->type->table->rows, tok_str(p));
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
            int64_t index = 0;
            if (EvalExpr(idx, &index)) {
                const int64_t len = expr->type->len;
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
        struct Type *type = NewPtrType(expr->type);
        return NewUnaryExpr(expr, type, kind);
    }
    if (kind == T_MUL) {
        struct Expr *expr = unary_expr(p);
        if (!IsPtr(expr->type)) {
            error(p, tok->pos,
                    "type mismatch: * must be used for pointer type");
        }
        struct Type *type = DuplicateType(expr->type->underlying);
        return NewUnaryExpr(expr, type, kind);
    }

    switch (kind) {
    case T_ADD:
    case T_SUB:
    case T_LNOT:
    case T_NOT:
        {
            struct Expr *e = unary_expr(p);
            return NewUnaryExpr(e, (struct Type*)(e->type), kind);
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

static struct Expr *expression(Parser *p)
{
    return logor_expr(p);
}

static void semantic_check_assign_stmt(Parser *p, struct Pos pos,
        const struct Expr *lval, const struct Expr *rval)
{
    if (!MatchType(lval->type, rval->type)) {
        error(p, pos, "type mismatch: l-value type '%s': r-value type '%s'",
                TypeString(lval->type), TypeString(rval->type));
    }
    // TODO make new_assign_stmt()
    if (IsFunc(rval->type) && rval->type->func_type->is_builtin) {
        assert(rval->kind == T_FUNCLIT);
        struct Func *func = rval->func;
        error(p, pos, "builtin function can not be assigned: '%s'",
                func->name);
    }
    if (!IsMutable(lval)) {
        const struct Var *var = FindRootObject(lval);
        assert(var);
        error(p, pos, "parameter object can not be modified: '%s'",
                var->name);
    }
}

static void semantic_check_incdec_stmt(Parser *p, struct Pos pos,
        const struct Expr *lval)
{
    if (!IsInt(lval->type)) {
        error(p, pos, "type mismatch: ++/-- must be used for int");
    }
}

// assign_stmt = logand_expr assing_op expression
//             | logand_expr incdec_op
// assign_op   = "=" | "+=" | "-=" | "*=" | "/=" | "%="
// incdec_op   = "++" | "--"
static struct Stmt *assign_stmt(Parser *p)
{
    struct Expr *lval = expression(p);
    struct Expr *rval = NULL;
    const struct Token *tok = gettok(p);
    struct Pos pos = tok->pos;

    switch (tok->kind) {

    case T_ASSN:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_ASSIGN);

    case T_AADD:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_ADDASSIGN);

    case T_ASUB:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_SUBASSIGN);

    case T_AMUL:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_MULASSIGN);

    case T_ADIV:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_DIVASSIGN);

    case T_AREM:
        rval = expression(p);
        semantic_check_assign_stmt(p, pos, lval, rval);
        return NewAssignStmt(lval, rval, NOD_EXPR_REMASSIGN);

    case T_INC:
        semantic_check_incdec_stmt(p, pos, lval);
        //return NewIncDecStmt(lval, NOD_EXPR_INC);
        // TODO testing new function
        return parser_new_inc_stmt(lval);

    case T_DEC:
        semantic_check_incdec_stmt(p, pos, lval);
        // TODO testing new function
        //return NewIncDecStmt(lval, NOD_EXPR_DEC);
        return parser_new_dec_stmt(lval);

    default:
        ungettok(p);
        return NewExprStmt(lval);
    }
}

static struct Scope *new_child_scope(struct Parser *p)
{
    struct Scope *parent = p->scope;
    struct Scope *child = NewScope(parent);
    struct Symbol *sym = NewSymbol(SYM_SCOPE, "_", NewNilType());

    sym->scope = child;
    VecPush(&parent->syms, sym);

    return child;
}

static struct Stmt *or_stmt(Parser *p)
{
    struct Expr *cond = NULL;

    if (consume(p, T_NEWLINE)) {
        // or (else)
        cond = NULL;
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

    struct Stmt *init = NULL;
    struct Expr *cond = NULL;
    struct Stmt *post = NULL;

    if (consume(p, T_NEWLINE)) {
        // infinite loop
        init = NULL;
        cond = NewIntLitExpr(1);
        post = NULL;
    }
    else {
        struct Stmt *stmt = assign_stmt(p);

        if (consume(p, T_SEM)) {
            // traditional for
            init = stmt;
            cond = expression(p);
            expect(p, T_SEM);
            post = assign_stmt(p);
            expect(p, T_NEWLINE);
        }
        else if (consume(p, T_NEWLINE)) {
            // while style
            init = NULL;
            cond = stmt->expr;
            post = NULL;
            // TODO need mem pool?
            free(stmt);
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
    int node_kind = 0;

    switch (tok->kind) {
    case T_BRK: node_kind = NOD_STMT_BREAK; break;
    case T_CNT: node_kind = NOD_STMT_CONTINUE; break;
    }

    expect(p, T_NEWLINE);

    return NewJumpStmt(node_kind);
}

static struct Stmt *case_stmt(Parser *p, int kind)
{
    struct Expr conds = {0};
    struct Expr *cond = &conds;
    int node_kind = 0;

    if (kind == T_CASE) {
        node_kind = NOD_STMT_CASE;
        do {
            struct Expr *expr = expression(p);
            // TODO const int check
            cond = cond->next = expr;
        }
        while (consume(p, T_COMMA));
    }
    else if (kind == T_DFLT) {
        node_kind = NOD_STMT_DEFAULT;
        cond = cond->next = NULL;
    }

    expect(p, T_NEWLINE);

    struct Stmt *body = block_stmt(p, new_child_scope(p));
    return NewCaseStmt(conds.next, body, node_kind);
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
        expr = NULL;
    }
    else {
        expr = expression(p);
        expect(p, T_NEWLINE);
    }

    assert(p->func_);

    if (expr && p->func_->return_type->kind != expr->type->kind) {
        error(p, exprpos,
                "type mismatch: function type '%s': expression type '%s'",
                TypeString(p->func_->return_type),
                TypeString(expr->type), "");
    }

    return NewReturnStmt(expr);
}

static struct Stmt *expr_stmt(Parser *p)
{
    struct Stmt *s = assign_stmt(p);
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

static struct Expr *default_value(const struct Type *type)
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
    struct Type *type = NULL;
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
    const struct Pos init_pos = tok_pos(p);

    expect(p, T_NEWLINE);

    struct Symbol *sym = DefineVar(p->scope, name, type, isglobal);
    if (!sym) {
        error(p, ident_pos,
                "re-defined identifier: '%s'", name);
    }
    struct Expr *ident = NewIdentExpr(sym);
    // TODO make new_assign_stmt()
    if (init && IsFunc(init->type) && init->type->func_type->is_builtin) {
        assert(init->kind == T_FUNCLIT);
        struct Func *func = init->func;
        error(p, init_pos,
                "builtin function can not be assigned: '%s'",
                func->name);
    }
    return NewInitStmt(ident, init);
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
            data_hashmap_insert(&tab->rows, tok_str(p), r);
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

static void param_list(Parser *p, struct Func *func)
{
    expect(p, T_LPAREN);

    if (consume(p, T_RPAREN))
        return;

    do {
        const struct Type *type = NULL;
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

static void ret_type(Parser *p, struct Func *func)
{
    const int next = peek(p);

    if (next == T_NEWLINE)
        func->return_type = NewNilType();
    else
        func->return_type = type_spec(p);
}

// type_spec = "bool" | "int" | "float" | "string" | identifier
// func_type = "#" param_list type_spec?
static struct Type *type_spec(Parser *p)
{
    struct Type *type = NULL;

    if (consume(p, T_MUL)) {
        return NewPtrType(type_spec(p));
    }

    if (consume(p, T_LBRACK)) {
        int64_t len = 0;
        if (peek(p) != T_RBRACK) {
            struct Expr *e = expression(p);
            if (!IsInt(e->type)) {
                error(p, tok_pos(p),
                        "array length expression must be integer type");
            }
            if (!EvalExpr(e, &len)) {
                error(p, tok_pos(p),
                        "array length expression must be compile time constant");
            }
        }
        expect(p, T_RBRACK);
        return NewArrayType(len, type_spec(p));
    }

    if (consume(p, T_HASH)) {
        struct Func *func = DeclareFunc(p->scope, "_", p->module->filename);
        // TODO check NULL func
        VecPush(&p->module->funcs, func);
        param_list(p, func);
        ret_type(p, func);
        // func type
        func->func_type = MakeFuncType(func);
        return NewFuncType(func->func_type);
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
static void func_def(struct Parser *p)
{
    expect(p, T_HASH);
    expect(p, T_IDENT);

    // func
    const char *name = tok_str(p);
    const struct Pos ident_pos = tok_pos(p);
    struct Func *func = DeclareFunc(p->scope, name, p->module->filename);
    if (!func) {
        error(p, ident_pos, "re-defined identifier: '%s'", name);
    }
    VecPush(&p->module->funcs, func);

    // params
    param_list(p, func);
    ret_type(p, func);
    expect(p, T_NEWLINE);

    // func type
    func->func_type = MakeFuncType(func);

    // func body
    p->func_ = func;
    struct Stmt *body = block_stmt(p, func->scope);
    // TODO control flow check to allow implicit return
    for (struct Stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = NewReturnStmt(NULL);
            break;
        }
    }
    func->body = body;
    p->func_ = NULL;

    // TODO remove this
    if (!strcmp(func->name, "main"))
        p->module->main_func = func;
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
    Parse(src, filename, modulename, tok, p->scope);

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
            func_def(p);
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
