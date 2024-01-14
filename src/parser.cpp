#include "parser.h"
#include "escseq.h"
#include "error.h"
#include <iostream>


typedef struct Parser {
    Scope *scope_ = nullptr;
    Func *func_ = nullptr;
    const Token *curr_;
    const char *src_;

    // TODO remove this
    int funclit_id_ = 0;

    // expression
    Expr *arg_list(Expr *call);
    Expr *conv_expr(int kind);
    Expr *primary_expr();
    Expr *unary_expr();
    Expr *mul_expr();
    Expr *add_expr();
    Expr *rel_expr();
    Expr *logand_expr();
    Expr *logor_expr();
    Expr *assign_expr();
    Expr *expression();

    // statement
    Stmt *or_stmt();
    Stmt *if_stmt();
    Stmt *for_stmt();
    Stmt *jump_stmt();
    Stmt *switch_stmt();
    Stmt *case_stmt(int kind);
    Stmt *ret_stmt();
    Stmt *expr_stmt();
    Stmt *scope_stmt();
    Stmt *nop_stmt();
    Stmt *block_stmt(Func *func = nullptr);

    //
    Type *type_spec();
    Stmt *var_decl();
    Field *field_decl();
    Class *class_decl();
    void field_list(Class *clss);
    void param_list(Func *func);
    void ret_type(Func *func);
    FuncDef *func_def();

    // error
    void error(Pos pos, std::string_view s0, std::string_view s1 = {},
            std::string_view s2 = {}, std::string_view s3 = {},
            std::string_view s4 = {}, std::string_view s5 = {}) const;
    Prog *program();
} Parser;

typedef struct ExprList {
    Expr head;
    Expr *tail;
    int count;
} ExprList;

static void init_expr_list(ExprList *list)
{
    list->head.next = NULL;
    list->tail = &list->head;
    list->count = 0;
}

static void append_expr(ExprList *list, Expr *e)
{
    list->tail->next = e;
    list->tail = e;
    list->count++;
}

typedef struct StmtList {
    Stmt head;
    Stmt *tail;
} StmtList;

static void init_list(StmtList *list)
{
    list->head.next = NULL;
    list->tail = &list->head;
}

static void append(StmtList *list, Stmt *s)
{
    list->tail->next = s;
    list->tail = s;
}

void Parser::error(Pos pos, std::string_view s0, std::string_view s1,
        std::string_view s2, std::string_view s3,
        std::string_view s4, std::string_view s5) const
{
    std::string msg(s0);

    if (!s1.empty()) msg += s1;
    if (!s2.empty()) msg += s2;
    if (!s3.empty()) msg += s3;
    if (!s4.empty()) msg += s4;
    if (!s5.empty()) msg += s5;

    Error(msg, src_, pos);
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

static int peek(Parser *p)
{
    /*
    if (p->curr_->kind == T_EOF)
        return T_EOF;
    else
        return p->curr_->next->kind;
    */
    const Token *tok = gettok(p);
    const int kind = tok->kind;
    ungettok(p);
    return kind;
}

static void expect(Parser *p, int kind)
{
    const Token *tok = gettok(p);
    if (tok->kind != kind) {
        const std::string msg =
            std::string("expected '") + TokenKindString(kind) + "'";
        Error(msg, p->src_, tok->pos);
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
        p->scope_ = p->scope_->OpenChild();
    }
}

static void enter_scope(Parser *p, Class *clss)
{
    p->scope_ = clss->scope;
}

static void leave_scope(Parser *p)
{
    p->scope_ = p->scope_->Close();
}

Expr *Parser::arg_list(Expr *call)
{
    ExprList list;
    init_expr_list(&list);

    if (peek(this) != T_RPAREN) {
        do {
            append_expr(&list, expression());
        }
        while (consume(this, T_COMMA));
    }

    const Func *func = call->l->type->func;
    if (func->HasSpecialVar())
        append_expr(&list, NewIntLitExpr(call->pos.y));

    call->list = list.head.next;

    const int argc = list.count;
    const int paramc = func->RequiredParamCount();
    if (argc < paramc)
        error(tok_pos(this), "too few arguments");

    const Expr *arg = call->list;
    for (int i = 0; i < argc; i++, arg = arg->next) {
        const Var *param = func->GetParam(i);

        if (!param)
            error(tok_pos(this), "too many arguments");

        // TODO arg needs to know its pos
        if (!MatchType(arg->type, param->type)) {
            error(tok_pos(this), "type mismatch: parameter type '",
                TypeString(param->type), "': argument type '",
                TypeString(arg->type), "'");
        }
    }

    expect(this, T_RPAREN);

    return call;
}

Expr *Parser::conv_expr(int kind)
{
    Type *to_type = type_spec();
    const Pos tokpos = tok_pos(this);

    expect(this, T_LPAREN);
    Expr *expr = expression();
    expect(this, T_RPAREN);

    switch (expr->type->kind) {
    case TY::BOOL:
    case TY::INT:
    case TY::FLOAT:
        break;
    default:
        error(tokpos, "unable to convert type from '",
                TypeString(expr->type), "' to '",
                TypeString(to_type), "'");
        break;
    }

    return NewConversionExpr(expr, to_type);
}

// primary_expr =
//     IntNum |
//     FpNum |
//     StringLit |
//     primary_expr selector
Expr *Parser::primary_expr()
{
    if (consume(this, T_NIL))
        return NewNilLitExpr();

    if (consume(this, T_TRU))
        return NewBoolLitExpr(true);

    if (consume(this, T_FLS))
        return NewBoolLitExpr(false);

    if (consume(this, T_INTLIT))
        return NewIntLitExpr(tok_int(this));

    if (consume(this, T_FLTLIT))
        return NewFloatLitExpr(tok_float(this));

    if (consume(this, T_STRLIT)) {
        Expr *e = NewStringLitExpr(tok_str(this));
        const Token *tok = curtok(this);
        if (tok->has_escseq) {
            const int errpos = ConvertEscapeSequence(e->sval, &e->converted);
            if (errpos != -1) {
                printf("!! e->val.s [%s] errpos %d\n", e->sval, errpos);
                Pos pos = tok->pos;
                pos.x += errpos + 1;
                Error("unknown escape sequence", src_, pos);
            }
        }
        return e;
    }

    if (consume(this, T_LPAREN)) {
        Expr *e = expression();
        expect(this, T_RPAREN);
        return e;
    }

    if (consume(this, T_CALLER_LINE)) {
        Var *var = scope_->FindVar(tok_str(this));
        if (!var) {
            error(tok_pos(this),
                    "special variable '", tok_str(this),
                    "' not declared in parameters");
        }
        return NewIdentExpr(var);
    }

    const int next = peek(this);
    switch (next) {
    case T_BOL:
    case T_INT:
    case T_FLT:
        return conv_expr(next);
    default:
        break;
    }

    Expr *expr = nullptr;

    for (;;) {
        const Token *tok = gettok(this);

        if (tok->kind == T_IDENT) {
            Var *var = scope_->FindVar(tok->sval);
            if (!var) {
                error(tok_pos(this),
                        "undefined identifier: '", tok_str(this), "'");
            }

            expr = NewIdentExpr(var);
            continue;
        }
        else if (tok->kind == T_LPAREN) {
            if (!expr || !expr->type->IsFunc()) {
                error(tok_pos(this),
                        "call operator must be used for function type");
            }
            // TODO func signature check
            Expr *call = NewCallExpr(expr, tok->pos);
            expr = arg_list(call);
            continue;
        }
        else if (tok->kind == T_DOT) {
            // TODO FIX
            if (!expr || !expr->type) {
                std::cerr << "error: no type" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (!expr->type->IsClass()) {
                std::cerr << "error: not a class type" << std::endl;
                exit(EXIT_FAILURE);
            }

            expect(this, T_IDENT);

            Field *fld = expr->type->clss->FindField(tok_str(this));

            expr = NewSelectExpr(expr, NewFieldExpr(fld));
            continue;
        }
        else if (tok->kind == T_LBRACK) {
            if (!expr->type->IsArray()) {
                error(tok_pos(this),
                        "index operator must be used for array type");
            }
            Expr *idx = expression();
            if (!idx->type->IsInt()) {
                error(tok_pos(this),
                        "index expression must be integer type");
            }
            long index = 0;
            if (EvalExpr(idx, &index)) {
                const long len = expr->type->len;
                if (index >= len) {
                    error(tok_pos(this), "index out of range[", std::to_string(index),
                            "] with length ", std::to_string(len));
                }
            }
            expect(this, T_RBRACK);
            return NewIndexExpr(expr, idx);
        }
        else {
            if (!expr) {
                const std::string msg = "unknown token: \"" +
                    std::string(TokenKindString(tok->kind)) + "\"";
                Error(msg, src_, tok->pos);
            }

            ungettok(this);
            return expr;
        }
    }

    return nullptr;
}

// unary_expr = primary_expr (unary_op primary_expr)*
// unary_op   = "+" | "-" | "!" | "~"
Expr *Parser::unary_expr()
{
    const Token *tok = gettok(this);
    const int kind = tok->kind;

    if (kind == T_AND) {
        Expr *expr = unary_expr();
        Type *type = NewPtrType(expr->type);
        return NewUnaryExpr(expr, type, kind);
    }
    if (kind == T_MUL) {
        Expr *expr = unary_expr();
        if (!expr->type->IsPtr()) {
            error(tok->pos,
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
            Expr *e = unary_expr();
            return NewUnaryExpr(e, const_cast<Type*>(e->type), kind);
        }

    default:
        ungettok(this);
        return primary_expr();
    }
}

// mul_expr = unary_expr (mul_op unary_expr)*
// mul_op   = "*" | "/" | "%" | "&" | "<<" | ">>"
Expr *Parser::mul_expr()
{
    Expr *x = unary_expr();
    Expr *y;

    for (;;) {
        const Token *tok = gettok(this);

        switch (tok->kind) {
        case T_MUL:
        case T_DIV:
        case T_REM:
        case T_AND:
        case T_SHL:
        case T_SHR:
            y = unary_expr();
            if (!MatchType(x->type, y->type)) {
                const std::string msg = std::string("type mismatch: ") +
                    TypeString(x->type) + " and " + TypeString(y->type);
                Error(msg, src_, tok->pos);
            }
            x = NewBinaryExpr(x, y, tok->kind);
            break;

        default:
            ungettok(this);
            return x;
        }
    }
}

// add_expr = mul_expr (add_op mul_expr)*
// add_op   = "+" | "-" | "|" | "^"
Expr *Parser::add_expr()
{
    Expr *expr = mul_expr();

    for (;;) {
        const Token *tok = gettok(this);
        Expr *l;

        switch (tok->kind) {
        case T_ADD:
        case T_SUB:
        case T_OR:
        case T_XOR:
            l = mul_expr();
            if (!MatchType(expr->type, l->type)) {
                const std::string msg = std::string("type mismatch: ") +
                    TypeString(expr->type) + " and " + TypeString(l->type);
                Error(msg, src_, tok->pos);
            }
            expr = NewBinaryExpr(expr, l, tok->kind);
            break;

        default:
            ungettok(this);
            return expr;
        }
    }
}

// rel_expr = add_expr (rel_op add_expr)*
// rel_op   = "==" | "!=" | "<" | ">" | "<=" | ">="
Expr *Parser::rel_expr()
{
    Expr *l = add_expr();
    Expr *r = nullptr;

    for (;;) {
        const Token *tok = gettok(this);

        switch (tok->kind) {
        case T_EQ:
        case T_NEQ:
        case T_LT:
        case T_GT:
        case T_LTE:
        case T_GTE:
            r = add_expr();
            if (!MatchType(l->type, r->type)) {
                error(tok->pos, "type mismatch: ",
                        TypeString(l->type), " and ",
                        TypeString(r->type));
            }
            l = NewRelationalExpr(l, r, tok->kind);
            continue;

        default:
            ungettok(this);
            return l;
        }
    }
}

// logand_expr = rel_expr ("&&" rel_expr)*
Expr *Parser::logand_expr()
{
    Expr *expr = rel_expr();

    for (;;) {
        const Token *tok = gettok(this);

        switch (tok->kind) {
        case T_LAND:
            expr = NewBinaryExpr(expr, rel_expr(), tok->kind);
            continue;

        default:
            ungettok(this);
            return expr;
        }
    }
}

// logor_expr = logand_expr ("||" logand_expr)*
Expr *Parser::logor_expr()
{
    Expr *expr = logand_expr();

    for (;;) {
        const Token *tok = gettok(this);

        switch (tok->kind) {
        case T_LOR:
            expr = NewBinaryExpr(expr, logand_expr(), tok->kind);
            continue;

        default:
            ungettok(this);
            return expr;
        }
    }
}

// assign_expr = logand_expr assing_op expression
//             | logand_expr incdec_op
// assign_op   = "=" | "+=" | "-=" | "*=" | "/=" | "%="
// incdec_op   = "++" | "--"
Expr *Parser::assign_expr()
{
    Expr *lval = logor_expr();
    Expr *rval = nullptr;
    const Token *tok = gettok(this);
    const int kind = tok->kind;

    switch (kind) {
    case T_ASSN:
    case T_AADD:
    case T_ASUB:
    case T_AMUL:
    case T_ADIV:
    case T_AREM:
        rval = expression();
        if (!MatchType(lval->type, rval->type)) {
            error(tok->pos, "type mismatch: l-value type '",
                TypeString(lval->type), "': r-value type '",
                TypeString(rval->type), "'");
        }
        return NewAssignExpr(lval, rval, kind);

    case T_INC:
    case T_DEC:
        if (!lval->type->IsInt()) {
            error(tok->pos,
                    "type mismatch: ++/-- must be used for int");
        }
        return NewIncDecExpr(lval, kind);

    default:
        ungettok(this);
        return lval;
    }
}

Expr *Parser::expression()
{
    return assign_expr();
}

Stmt *Parser::or_stmt()
{
    Expr *cond = nullptr;

    if (consume(this, T_NEWLINE)) {
        // or (else)
        cond = NewNullExpr();
    }
    else {
        // or if (else if)
        cond = expression();
        expect(this, T_NEWLINE);
    }

    Stmt *body = block_stmt();

    return NewOrStmt(cond, body);
}

Stmt *Parser::if_stmt()
{
    expect(this, T_IF);
    Expr *cond = expression();
    expect(this, T_NEWLINE);

    StmtList list;
    init_list(&list);

    Stmt *body = block_stmt();
    append(&list, NewOrStmt(cond, body));

    bool endor = false;

    while (!endor) {
        if (consume(this, T_ELS)) {
            if (peek(this) == T_NEWLINE) {
                // last 'or' (else)
                endor = true;
            }

            append(&list, or_stmt());
        }
        else {
            endor = true;
        }
    }

    return NewIfStmt(list.head.next);
}

Stmt *Parser::for_stmt()
{
    expect(this, T_FOR);

    Expr *init = nullptr;
    Expr *cond = nullptr;
    Expr *post = nullptr;

    if (consume(this, T_NEWLINE)) {
        // infinite loop
        init = NewNullExpr();
        cond = NewIntLitExpr(1);
        post = NewNullExpr();
    }
    else {
        Expr *e = expression();

        if (consume(this, T_SEM)) {
            // traditional for
            init = e;
            cond = expression();
            expect(this, T_SEM);
            post = expression();
            expect(this, T_NEWLINE);
        }
        else if (consume(this, T_NEWLINE)) {
            // while style
            init = NewNullExpr();
            cond = e;
            post = NewNullExpr();
        }
        else {
            const Token *tok = gettok(this);
            Error("unknown token", src_, tok->pos);
        }
    }

    // body
    Stmt *body = block_stmt();
    return NewForStmt(init, cond, post, body);
}

Stmt *Parser::jump_stmt()
{
    const Token *tok = gettok(this);
    const int kind = tok->kind;

    if (kind == T_BRK) {
    }
    else if (kind == T_CNT) {
    }

    expect(this, T_NEWLINE);

    return NewJumpStmt(kind);
}

Stmt *Parser::switch_stmt()
{
    expect(this, T_SWT);

    Expr *expr = expression();
    // TODO int check
    expect(this, T_NEWLINE);

    StmtList list;
    init_list(&list);

    int default_count = 0;

    for (;;) {
        const Token *tok = gettok(this);

        switch (tok->kind) {
        case T_CASE:
            if (default_count > 0) {
                error(tok->pos, "No 'case' should come after 'default'");
            }
            append(&list, case_stmt(tok->kind));
            continue;

        case T_DFLT:
            append(&list, case_stmt(tok->kind));
            default_count++;
            continue;

        default:
            ungettok(this);
            return NewSwitchStmt(expr, list.head.next);
        }
    }
}

Stmt *Parser::case_stmt(int kind)
{
    // TODO make this last by adding ListExpr
    StmtList list;
    init_list(&list);

    if (kind == T_CASE) {
        do {
            Expr *expr = expression();
            // TODO const int check
            append(&list, NewExprStmt(expr));
        }
        while (consume(this, T_COMMA));
    }
    else if (kind == T_DFLT) {
        append(&list, NewExprStmt(NewNullExpr()));
    }

    expect(this, T_NEWLINE);

    Stmt *body = block_stmt();
    return NewCaseStmt(list.head.next, body, kind);
}

Stmt *Parser::ret_stmt()
{
    expect(this, T_RET);

    const Pos exprpos = tok_pos(this);
    Expr *expr = nullptr;

    if (consume(this, T_NEWLINE)) {
        expr = NewNullExpr();
    }
    else {
        expr = expression();
        expect(this, T_NEWLINE);
    }

    assert(func_);

    if (func_->return_type->kind != expr->type->kind) {
        error(exprpos, "type mismatch: function type '",
            TypeString(func_->return_type), "': expression type '",
            TypeString(expr->type), "'");
    }

    return NewReturnStmt(expr);
}

Stmt *Parser::expr_stmt()
{
    Stmt *s = NewExprStmt(expression());
    expect(this, T_NEWLINE);

    return s;
}

Stmt *Parser::scope_stmt()
{
    expect(this, T_DASH3);
    expect(this, T_NEWLINE);

    return block_stmt();
}

Stmt *Parser::nop_stmt()
{
    expect(this, T_NOP);

    Stmt *s = NewNopStmt();
    expect(this, T_NEWLINE);

    return s;
}

static Expr *default_value(const Type *type)
{
    switch (type->kind) {
    case TY::BOOL:
        return NewBoolLitExpr(false);
    case TY::INT:
        return NewIntLitExpr(0);
    case TY::FLOAT:
        return NewFloatLitExpr(0.0);
    case TY::STRING:
        return NewStringLitExpr("");

    case TY::PTR:
        return NewNilLitExpr();

    case TY::ARRAY:
        // TODO fill with zero values
        // put len at base addr
        return NewIntLitExpr(type->len);

    case TY::CLASS:
    case TY::FUNC:
        // TODO
        return NewNilLitExpr();

    case TY::NIL:
    case TY::ANY:
        ERROR_NO_CASE(type->kind);
        return nullptr;
    }
}

// var_decl = "-" identifier type newline
//          | "-" identifier type = expression newline
Stmt *Parser::var_decl()
{
    expect(this, T_SUB);
    expect(this, T_IDENT);

    if (scope_->FindVar(tok_str(this), false)) {
        const std::string msg = "re-defined variable: '" +
            std::string(tok_str(this)) + "'";
        Error(msg, src_, tok_pos(this));
    }

    // var anme
    const char *name = tok_str(this);
    Type *type = nullptr;
    Expr *init = nullptr;

    // type and init
    if (consume(this, T_ASSN)) {
        // "- x = 42"
        init = expression();
        type = DuplicateType(init->type);
    }
    else {
        type = type_spec();

        if (consume(this, T_ASSN)) {
            // "- x int = 42"
            init = expression();
        }
        else {
            // "- x int"
            init = default_value(type);
        }
    }

    expect(this, T_NEWLINE);

    Var *var = scope_->DefineVar(name, type);
    Expr *ident = NewIdentExpr(var);
    return NewExprStmt(NewAssignExpr(ident, init, T_ASSN));
}

Field *Parser::field_decl()
{
    expect(this, T_SUB);
    expect(this, T_IDENT);

    if (scope_->FindField(tok_str(this))) {
        std::cerr
            << "error: re-defined variable: '"
            << tok_str(this) << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Field *fld = scope_->DefineFild(tok_str(this));
    fld->type = type_spec();
    expect(this, T_NEWLINE);

    return fld;
}

Class *Parser::class_decl()
{
    expect(this, T_HASH2);
    expect(this, T_IDENT);

    // class name
    Class *clss = scope_->DefineClass(tok_str(this));
    if (!clss) {
        std::cerr << "error: re-defined class: '" << tok_str(this) << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    expect(this, T_NEWLINE);
    enter_scope(this, clss);
    expect(this, T_BLOCKBEGIN);
    field_list(clss);
    expect(this, T_BLOCKEND);
    leave_scope(this);

    return nullptr;
    //return new ClasDef(clss);
}

Stmt *Parser::block_stmt(Func *func)
{
    StmtList list;
    init_list(&list);

    enter_scope(this, func);
    expect(this, T_BLOCKBEGIN);

    for (;;) {
        const int next = peek(this);

        if (next == T_SUB) {
            append(&list, var_decl());
            continue;
        }
        else if (next == T_IF) {
            append(&list, if_stmt());
            continue;
        }
        else if (next == T_FOR) {
            append(&list, for_stmt());
            continue;
        }
        else if (next == T_BRK || next == T_CNT) {
            append(&list, jump_stmt());
            continue;
        }
        else if (next == T_SWT) {
            append(&list, switch_stmt());
            continue;
        }
        else if (next == T_RET) {
            append(&list, ret_stmt());
            continue;
        }
        else if (next == T_DASH3) {
            append(&list, scope_stmt());
            continue;
        }
        else if (next == T_NOP) {
            append(&list, nop_stmt());
            continue;
        }
        else if (next == T_NEWLINE) {
            gettok(this);
            continue;
        }
        else if (next == T_BLOCKEND) {
            break;
        }
        else {
            append(&list, expr_stmt());
            continue;
        }
    }

    expect(this, T_BLOCKEND);
    leave_scope(this);

    return NewBlockStmt(list.head.next);
}

// type_spec = "bool" | "int" | "float" | "string" | identifier
// func_type = "#" param_list type_spec?
Type *Parser::type_spec()
{
    Type *parent = nullptr;
    Type *type = nullptr;

    if (consume(this, T_MUL)) {
        return NewPtrType(type_spec());
    }

    if (consume(this, T_LBRACK)) {
        parent = new Type(TY::ARRAY);
        Expr *e = expression();
        if (!e->type->IsInt()) {
            error(tok_pos(this),
                    "array length expression must be integer type");
        }
        long len = 0;
        if (!EvalExpr(e, &len)) {
            error(tok_pos(this),
                    "array length expression must be compile time constant");
        }
        expect(this, T_RBRACK);
        return NewArrayType(len, type_spec());
    }

    if (consume(this, T_HASH)) {
        Func *func = scope_->DeclareFunc();
        param_list(func);
        ret_type(func);
        return NewFuncType(func);
    }

    if (consume(this, T_BOL)) {
        type = NewBoolType();
    }
    else if (consume(this, T_INT)) {
        type = new Type(TY::INT);
    }
    else if (consume(this, T_FLT)) {
        type = new Type(TY::FLOAT);
    }
    else if (consume(this, T_STR)) {
        type = new Type(TY::STRING);
    }
    else if (consume(this, T_IDENT)) {
        type = new Type(TY::CLASS);
        type->clss = scope_->FindClass(tok_str(this));
    }
    else {
        const Token *tok = gettok(this);
        error(tok->pos, "not a type name: '",
                TokenKindString(tok->kind), "'");
    }

    return type;
}

void Parser::field_list(Class *clss)
{
    expect(this, T_SUB);

    do {
        expect(this, T_IDENT);
        const char *name = tok_str(this);

        clss->DeclareField(name, type_spec());
        expect(this, T_NEWLINE);
    }
    while (consume(this, T_SUB));
}

void Parser::param_list(Func *func)
{
    expect(this, T_LPAREN);

    if (consume(this, T_RPAREN))
        return;

    do {
        const Type *type = nullptr;
        const char *name;

        if (consume(this, T_CALLER_LINE)) {
            name = tok_str(this);
            type = new Type(TY::INT);
        }
        else {
            expect(this, T_IDENT);
            name = tok_str(this);
            type = type_spec();
        }

        func->DeclareParam(name, type);
    }
    while (consume(this, T_COMMA));

    expect(this, T_RPAREN);
}

void Parser::ret_type(Func *func)
{
    const int next = peek(this);

    if (next == T_NEWLINE)
        func->return_type = new Type(TY::NIL);
    else
        func->return_type = type_spec();
}

// func_def = "#" identifier param_list type_spec? newline block_stmt
FuncDef *Parser::func_def()
{
    expect(this, T_HASH);
    expect(this, T_IDENT);

    // signature
    const char *name = tok_str(this);
    Func *func = scope_->DeclareFunc();

    // params
    param_list(func);
    ret_type(func);
    expect(this, T_NEWLINE);

    // func var
    if (scope_->FindVar(name)) {
        // error
        return nullptr;
    }
    Var *var = scope_->DefineVar(name, NewFuncType(func));

    // func body
    func_ = func;

    enter_scope(this, func);
    Stmt *body = block_stmt();
    // TODO control flow check to allow implicit return
    for (Stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = NewReturnStmt(NewNullExpr());
            break;
        }
    }
    leave_scope(this);

    func_ = nullptr;

    // XXX temp
    FuncDef *fdef = NewFuncDef(var, body);
    fdef->funclit_id = funclit_id_++;

    return fdef;
}

Prog *Parser::program()
{
    Prog *prog = NewProg(scope_);
    FuncDef *tail = NULL;

    StmtList list;
    init_list(&list);

    for (;;) {
        const int next = peek(this);

        if (next == T_HASH) {
            FuncDef *fdef = func_def();

            // TODO remove this
            //if (fdef->var->name == "main")
            if (!strcmp(fdef->var->name, "main"))
                prog->main_func = fdef->var;

            {
                // TODO clean up
                Expr *ident = NewIdentExpr(const_cast<Var *>(fdef->var));
                Expr *init = NewIntLitExpr(fdef->funclit_id);
                append(&list, NewExprStmt(NewAssignExpr(ident, init, T_ASSN)));
            }

            if (!prog->funcs) {
                prog->funcs = fdef;
                tail = prog->funcs;
            }
            else {
                tail->next = fdef;
                tail = fdef;
            }
            continue;
        }

        if (next == T_HASH2) {
            class_decl();
            continue;
        }

        if (next == T_SUB) {
            append(&list, var_decl());
            continue;
        }

        if (next == T_NEWLINE) {
            gettok(this);
            continue;
        }

        if (next == T_EOF) {
            break;
        }

        const Token *tok = gettok(this);
        const std::string msg = std::string("error: unexpected token: '") +
            TokenKindString(next) + "'";
        Error(msg, src_, tok->pos);
    }

    prog->gvars = list.head.next;
    return prog;
}

static Prog *parse(Parser *p, const char *src, const Token *tok, Scope *scope)
{
    p->src_ = src;
    p->curr_ = tok;
    p->scope_ = scope;

    // global (file) scope
    enter_scope(p, (Func *)NULL);

    return p->program();
}

Prog *Parse(const char *src, const Token *tok, Scope *scope)
{
    Parser parser;
    return parse(&parser, src, tok, scope);
}
