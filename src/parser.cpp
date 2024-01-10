#include "parser.h"
#include "error.h"
#include <iostream>

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

    Error(msg, *src_, pos);
}

Prog *Parser::Parse(const std::string &src, Scope *scope)
{
    src_ = &src;
    lexer_.SetInput(src);
    scope_ = scope;

    // global (file) scope
    enter_scope();

    return program();
}

Token *Parser::next() const
{
    if (curr_ == end_)
        return begin_;
    else
        return curr_ + 1;
}

Token *Parser::prev() const
{
    if (curr_ == begin_)
        return end_;
    else
        return curr_ - 1;
}

const Token *Parser::curtok() const
{
    return curr_;
}

const Token *Parser::gettok()
{
    if (curr_ != head_) {
        curr_ = next();
        return curr_;
    }
    else {
        curr_ = next();
        lexer_.Get(curr_);
        head_ = curr_;
        return curr_;
    }
}

void Parser::ungettok()
{
    curr_ = prev();
}

Pos Parser::tok_pos() const
{
    return curr_->pos;
}

long Parser::tok_int() const
{
    return curr_->ival;
}

double Parser::tok_float() const
{
    return curr_->fval;
}

std::string_view Parser::tok_str() const
{
    return curr_->sval;
}

TokenKind Parser::peek()
{
    const Token *tok = gettok();
    const TokenKind kind = tok->kind;
    ungettok();
    return kind;
}

void Parser::expect(TokenKind kind)
{
    const Token *tok = gettok();
    if (tok->kind != kind) {
        const std::string msg =
            std::string("expected '") + GetTokenKindString(kind) + "'";
        Error(msg, *src_, tok->pos);
    }
}

bool Parser::consume(TokenKind kind)
{
    if (peek() == kind) {
        gettok();
        return true;
    }
    else {
        return false;
    }
}

void Parser::enter_scope(Func *func)
{
    if (func) {
        scope_ = func->scope;
    }
    else {
        scope_ = scope_->OpenChild();
    }
}

void Parser::enter_scope(Class *clss)
{
    scope_ = clss->scope;
}

void Parser::leave_scope()
{
    scope_ = scope_->Close();
}

Expr *Parser::arg_list(Expr *call)
{
    if (peek() != TK::RPAREN) {
        do {
            AddArg(call, expression());
        }
        while (consume(TK::COMMA));
    }

    const Func *func = call->l->type->func;
    if (func->HasSpecialVar())
        AddArg(call, NewIntLitExpr(call->pos.y));

    const int argc = ArgCount(call);
    const int paramc = func->RequiredParamCount();
    if (argc < paramc)
        error(tok_pos(), "too few arguments");

    for (int i = 0; i < argc; i++) {
        const Expr *arg = GetArg(call, i);
        const Var *param = func->GetParam(i);

        if (!param)
            error(tok_pos(), "too many arguments");

        // TODO arg needs to know its pos
        if (!MatchType(arg->type, param->type)) {
            error(tok_pos(), "type mismatch: parameter type '",
                TypeString(param->type), "': argument type '",
                TypeString(arg->type), "'");
        }
    }

    expect(TK::RPAREN);

    return call;
}

Expr *Parser::conv_expr(TK kind)
{
    Type *to_type = type_spec();
    const Pos tokpos = tok_pos();

    expect(TK::LPAREN);
    Expr *expr = expression();
    expect(TK::RPAREN);

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
    if (consume(TK::NIL))
        return NewNilLitExpr();

    if (consume(TK::TRUE))
        return NewBoolLitExpr(true);

    if (consume(TK::FALSE))
        return NewBoolLitExpr(false);

    if (consume(TK::INTLIT))
        return NewIntLitExpr(tok_int());

    if (consume(TK::FLTLIT))
        return NewFloatLitExpr(tok_float());

    if (consume(TK::STRLIT)) {
        Expr *e = NewStringLitExpr(tok_str());
        const Token *tok = curtok();
        if (tok->has_escseq) {
            const int errpos = ConvertEscSeq(e->val.sv, e->converted);
            if (errpos != -1) {
                Pos pos = tok->pos;
                pos.x += errpos + 1;
                Error("unknown escape sequence", *src_, pos);
            }
        }
        return e;
    }

    if (consume(TK::LPAREN)) {
        Expr *e = expression();
        expect(TK::RPAREN);
        return e;
    }

    if (consume(TK::CALLER_LINE)) {
        Var *var = scope_->FindVar(tok_str());
        if (!var) {
            error(tok_pos(),
                    "special variable '", tok_str(),
                    "' not declared in parameters");
        }
        return NewIdentExpr(var);
    }

    const TK next = peek();
    switch (next) {
    case TK::BOOL:
    case TK::INT:
    case TK::FLOAT:
        return conv_expr(next);
    default:
        break;
    }

    Expr *expr = nullptr;

    for (;;) {
        const Token *tok = gettok();

        if (tok->kind == TK::IDENT) {
            Var *var = scope_->FindVar(tok->sval);
            if (!var) {
                error(tok_pos(),
                        "undefined identifier: '", tok_str(), "'");
            }

            expr = NewIdentExpr(var);
            continue;
        }
        else if (tok->kind == TK::LPAREN) {
            if (!expr || !expr->type->IsFunc()) {
                error(tok_pos(),
                        "call operator must be used for function type");
            }
            // TODO func signature check
            Expr *call = NewCallExpr(expr, tok->pos);
            expr = arg_list(call);
            continue;
        }
        else if (tok->kind == TK::PERIOD) {
            // TODO FIX
            if (!expr || !expr->type) {
                std::cerr << "error: no type" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (!expr->type->IsClass()) {
                std::cerr << "error: not a class type" << std::endl;
                exit(EXIT_FAILURE);
            }

            expect(TK::IDENT);

            Field *fld = expr->type->clss->FindField(tok_str());

            expr = NewSelectExpr(expr, NewFieldExpr(fld));
            continue;
        }
        else if (tok->kind == TK::LBRACK) {
            if (!expr->type->IsArray()) {
                error(tok_pos(),
                        "index operator must be used for array type");
            }
            Expr *idx = expression();
            if (!idx->type->IsInt()) {
                error(tok_pos(),
                        "index expression must be integer type");
            }
            long index = 0;
            if (EvalExpr(idx, &index)) {
                const long len = expr->type->len;
                if (index >= len) {
                    error(tok_pos(), "index out of range[", std::to_string(index),
                            "] with length ", std::to_string(len));
                }
            }
            expect(TK::RBRACK);
            return NewIndexExpr(expr, idx);
        }
        else {
            if (!expr) {
                const std::string msg = "unknown token: \"" +
                    std::string(GetTokenKindString(tok->kind)) + "\"";
                Error(msg, *src_, tok->pos);
            }

            ungettok();
            return expr;
        }
    }

    return nullptr;
}

// unary_expr = primary_expr (unary_op primary_expr)*
// unary_op   = "+" | "-" | "!" | "~"
Expr *Parser::unary_expr()
{
    const Token *tok = gettok();
    const TK kind = tok->kind;

    if (kind == TK::AMP) {
        Expr *expr = unary_expr();
        Type *type = NewPtrType(expr->type);
        return NewUnaryExpr(expr, type, kind);
    }
    if (kind == TK::STAR) {
        Expr *expr = unary_expr();
        if (!expr->type->IsPtr()) {
            error(tok->pos,
                    "type mismatch: * must be used for pointer type");
        }
        Type *type = DuplicateType(expr->type->underlying);
        return NewUnaryExpr(expr, type, kind);
    }

    switch (kind) {
    case TK::PLUS:
    case TK::MINUS:
    case TK::EXCL:
    case TK::TILDA:
        {
            Expr *e = unary_expr();
            return NewUnaryExpr(e, const_cast<Type*>(e->type), kind);
        }

    default:
        ungettok();
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
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::STAR:
        case TK::SLASH:
        case TK::PERCENT:
        case TK::AMP:
        case TK::LT2:
        case TK::GT2:
            y = unary_expr();
            if (!MatchType(x->type, y->type)) {
                const std::string msg = std::string("type mismatch: ") +
                    TypeString(x->type) + " and " + TypeString(y->type);
                Error(msg, *src_, tok->pos);
            }
            x = NewBinaryExpr(x, y, tok->kind);
            break;

        default:
            ungettok();
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
        const Token *tok = gettok();
        Expr *l;

        switch (tok->kind) {
        case TK::PLUS:
        case TK::MINUS:
        case TK::BAR:
        case TK::CARET:
            l = mul_expr();
            if (!MatchType(expr->type, l->type)) {
                const std::string msg = std::string("type mismatch: ") +
                    TypeString(expr->type) + " and " + TypeString(l->type);
                Error(msg, *src_, tok->pos);
            }
            expr = NewBinaryExpr(expr, l, tok->kind);
            break;

        default:
            ungettok();
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
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::EQ2:
        case TK::EXCLEQ:
        case TK::LT:
        case TK::GT:
        case TK::LTE:
        case TK::GTE:
            r = add_expr();
            if (!MatchType(l->type, r->type)) {
                error(tok->pos, "type mismatch: ",
                        TypeString(l->type), " and ",
                        TypeString(r->type));
            }
            l = NewRelationalExpr(l, r, tok->kind);
            continue;

        default:
            ungettok();
            return l;
        }
    }
}

// logand_expr = rel_expr ("&&" rel_expr)*
Expr *Parser::logand_expr()
{
    Expr *expr = rel_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::AMP2:
            expr = NewBinaryExpr(expr, rel_expr(), tok->kind);
            continue;

        default:
            ungettok();
            return expr;
        }
    }
}

// logor_expr = logand_expr ("||" logand_expr)*
Expr *Parser::logor_expr()
{
    Expr *expr = logand_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::BAR2:
            expr = NewBinaryExpr(expr, logand_expr(), tok->kind);
            continue;

        default:
            ungettok();
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
    const Token *tok = gettok();
    const TK kind = tok->kind;

    switch (kind) {
    case TK::EQ:
    case TK::PLUSEQ:
    case TK::MINUSEQ:
    case TK::STAREQ:
    case TK::SLASHEQ:
    case TK::PERCENTEQ:
        rval = expression();
        if (!MatchType(lval->type, rval->type)) {
            error(tok->pos, "type mismatch: l-value type '",
                TypeString(lval->type), "': r-value type '",
                TypeString(rval->type), "'");
        }
        return NewAssignExpr(lval, rval, kind);

    case TK::PLUS2:
    case TK::MINUS2:
        if (!lval->type->IsInt()) {
            error(tok->pos,
                    "type mismatch: ++/-- must be used for int");
        }
        return NewIncDecExpr(lval, kind);

    default:
        ungettok();
        return lval;
    }
}

Expr *Parser::expression()
{
    return assign_expr();
}

OrStmt *Parser::or_stmt()
{
    Expr *cond = nullptr;

    if (consume(TK::NEWLINE)) {
        // or (else)
        cond = NewNullExpr();
    }
    else {
        // or if (else if)
        cond = expression();
        expect(TK::NEWLINE);
    }

    Stmt *body = block_stmt();

    return new OrStmt(cond, body);
}

Stmt *Parser::if_stmt()
{
    expect(TK::IF);
    Expr *cond = expression();
    expect(TK::NEWLINE);

    Stmt *body = block_stmt();
    IfStmt *ifs = new IfStmt(cond, body);

    bool endor = false;

    while (!endor) {
        if (consume(TK::OR)) {
            if (peek() == TK::NEWLINE) {
                // last 'or' (else)
                endor = true;
            }

            ifs->AddOr(or_stmt());
        }
        else {
            endor = true;
        }
    }

    return ifs;
}

Stmt *Parser::for_stmt()
{
    expect(TK::FOR);

    Expr *init = nullptr;
    Expr *cond = nullptr;
    Expr *post = nullptr;

    if (consume(TK::NEWLINE)) {
        // infinite loop
        init = NewNullExpr();
        cond = NewIntLitExpr(1);
        post = NewNullExpr();
    }
    else {
        Expr *e = expression();

        if (consume(TK::SEMICOLON)) {
            // traditional for
            init = e;
            cond = expression();
            expect(TK::SEMICOLON);
            post = expression();
            expect(TK::NEWLINE);
        }
        else if (consume(TK::NEWLINE)) {
            // while style
            init = NewNullExpr();
            cond = e;
            post = NewNullExpr();
        }
        else {
            const Token *tok = gettok();
            Error("unknown token", *src_, tok->pos);
        }
    }

    // body
    Stmt *body = block_stmt();
    return new ForStmt(init, cond, post, body);
}

Stmt *Parser::jump_stmt()
{
    const Token *tok = gettok();
    const TK kind = tok->kind;

    if (kind == TK::BREAK) {
    }
    else if (kind == TK::CONTINUE) {
    }

    expect(TK::NEWLINE);

    return new JumpStmt(kind);
}

Stmt *Parser::switch_stmt()
{
    expect(TK::SWITCH);

    Expr *expr = expression();
    // TODO int check
    expect(TK::NEWLINE);

    SwitchStmt *swtch = new SwitchStmt(expr);

    int default_count = 0;

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::CASE:
            if (default_count > 0) {
                error(tok->pos, "No 'case' should come after 'default'");
            }
            swtch->AddCase(case_stmt(tok->kind));
            continue;

        case TK::DEFAULT:
            swtch->AddCase(case_stmt(tok->kind));
            default_count++;
            continue;

        default:
            ungettok();
            return swtch;
        }
    }
}

CaseStmt *Parser::case_stmt(TK kind)
{
    // TODO make this last by adding ListExpr
    CaseStmt *cases = new CaseStmt(kind);

    if (kind == TK::CASE) {
        do {
            Expr *expr = expression();
            // TODO const int check
            cases->AddCond(expr);
        }
        while (consume(TK::COMMA));
    }
    else if (kind == TK::DEFAULT) {
        cases->AddCond(NewNullExpr());
    }

    expect(TK::NEWLINE);

    Stmt *body = block_stmt();
    cases->AddBody(body);

    return cases;
}

Stmt *Parser::ret_stmt()
{
    expect(TK::RETURN);

    const Pos exprpos = tok_pos();
    Expr *expr = nullptr;

    if (consume(TK::NEWLINE)) {
        expr = NewNullExpr();
    }
    else {
        expr = expression();
        expect(TK::NEWLINE);
    }

    assert(func_);

    if (func_->return_type->kind != expr->type->kind) {
        error(exprpos, "type mismatch: function type '",
            TypeString(func_->return_type), "': expression type '",
            TypeString(expr->type), "'");
    }

    return new ReturnStmt(expr);
}

Stmt *Parser::expr_stmt()
{
    ExprStmt *stmt = new ExprStmt(expression());
    expect(TK::NEWLINE);

    return stmt;
}

Stmt *Parser::scope_stmt()
{
    expect(TK::MINUS3);
    expect(TK::NEWLINE);

    return block_stmt();
}

Stmt *Parser::nop_stmt()
{
    expect(TK::NOP);

    NopStmt *stmt = new NopStmt();
    expect(TK::NEWLINE);

    return stmt;
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
    expect(TK::MINUS);
    expect(TK::IDENT);

    if (scope_->FindVar(tok_str(), false)) {
        const std::string msg = "re-defined variable: '" +
            std::string(tok_str()) + "'";
        Error(msg, *src_, tok_pos());
    }

    // var anme
    std::string_view name = tok_str();
    Type *type = nullptr;
    Expr *init = nullptr;

    // type and init
    if (consume(TK::EQ)) {
        // "- x = 42"
        init = expression();
        type = DuplicateType(init->type);
    }
    else {
        type = type_spec();

        if (consume(TK::EQ)) {
            // "- x int = 42"
            init = expression();
        }
        else {
            // "- x int"
            init = default_value(type);
        }
    }

    expect(TK::NEWLINE);

    Var *var = scope_->DefineVar(name, type);
    Expr *ident = NewIdentExpr(var);
    return new ExprStmt(NewAssignExpr(ident, init, TK::EQ));
}

Field *Parser::field_decl()
{
    expect(TK::MINUS);
    expect(TK::IDENT);

    if (scope_->FindField(tok_str())) {
        std::cerr
            << "error: re-defined variable: '"
            << tok_str() << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Field *fld = scope_->DefineFild(tok_str());
    fld->type = type_spec();
    expect(TK::NEWLINE);

    return fld;
}

Class *Parser::class_decl()
{
    expect(TK::HASH2);
    expect(TK::IDENT);

    // class name
    Class *clss = scope_->DefineClass(tok_str());
    if (!clss) {
        std::cerr << "error: re-defined class: '" << tok_str() << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    expect(TK::NEWLINE);
    enter_scope(clss);
    expect(TK::BLOCKBEGIN);
    field_list(clss);
    expect(TK::BLOCKEND);
    leave_scope();

    return nullptr;
    //return new ClasDef(clss);
}

struct StmtList {
    Stmt head;
    Stmt *tail;
};
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

Stmt *Parser::block_stmt(Func *func)
{
    StmtList list;
    init_list(&list);

    enter_scope(func);
    expect(TK::BLOCKBEGIN);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::MINUS) {
            append(&list, var_decl());
            continue;
        }
        else if (next == TK::IF) {
            append(&list, if_stmt());
            continue;
        }
        else if (next == TK::FOR) {
            append(&list, for_stmt());
            continue;
        }
        else if (next == TK::BREAK || next == TK::CONTINUE) {
            append(&list, jump_stmt());
            continue;
        }
        else if (next == TK::SWITCH) {
            append(&list, switch_stmt());
            continue;
        }
        else if (next == TK::RETURN) {
            append(&list, ret_stmt());
            continue;
        }
        else if (next == TK::MINUS3) {
            append(&list, scope_stmt());
            continue;
        }
        else if (next == TK::NOP) {
            append(&list, nop_stmt());
            continue;
        }
        else if (next == TK::NEWLINE) {
            gettok();
            continue;
        }
        else if (next == TK::BLOCKEND) {
            break;
        }
        else {
            append(&list, expr_stmt());
            continue;
        }
    }

    expect(TK::BLOCKEND);
    leave_scope();

    return NewBlockStmt(list.head.next);
}

// type_spec = "bool" | "int" | "float" | "string" | identifier
// func_type = "#" param_list type_spec?
Type *Parser::type_spec()
{
    Type *parent = nullptr;
    Type *type = nullptr;

    if (consume(TK::STAR)) {
        return NewPtrType(type_spec());
    }

    if (consume(TK::LBRACK)) {
        parent = new Type(TY::ARRAY);
        Expr *e = expression();
        if (!e->type->IsInt()) {
            error(tok_pos(),
                    "array length expression must be integer type");
        }
        long len = 0;
        if (!EvalExpr(e, &len)) {
            error(tok_pos(),
                    "array length expression must be compile time constant");
        }
        expect(TK::RBRACK);
        return NewArrayType(len, type_spec());
    }

    if (consume(TK::HASH)) {
        Func *func = scope_->DeclareFunc();
        param_list(func);
        ret_type(func);
        return NewFuncType(func);
    }

    if (consume(TK::BOOL)) {
        type = NewBoolType();
    }
    else if (consume(TK::INT)) {
        type = new Type(TY::INT);
    }
    else if (consume(TK::FLOAT)) {
        type = new Type(TY::FLOAT);
    }
    else if (consume(TK::STRING)) {
        type = new Type(TY::STRING);
    }
    else if (consume(TK::IDENT)) {
        type = new Type(TY::CLASS);
        type->clss = scope_->FindClass(tok_str());
    }
    else {
        const Token *tok = gettok();
        error(tok->pos, "not a type name: '",
                GetTokenKindString(tok->kind), "'");
    }

    return type;
}

void Parser::field_list(Class *clss)
{
    expect(TK::MINUS);

    do {
        expect(TK::IDENT);
        std::string_view name = tok_str();

        clss->DeclareField(name, type_spec());
        expect(TK::NEWLINE);
    }
    while (consume(TK::MINUS));
}

void Parser::param_list(Func *func)
{
    expect(TK::LPAREN);

    if (consume(TK::RPAREN))
        return;

    do {
        const Type *type = nullptr;
        std::string_view name;

        if (consume(TK::CALLER_LINE)) {
            name = tok_str();
            type = new Type(TY::INT);
        }
        else {
            expect(TK::IDENT);
            name = tok_str();
            type = type_spec();
        }

        func->DeclareParam(name, type);
    }
    while (consume(TK::COMMA));

    expect(TK::RPAREN);
}

void Parser::ret_type(Func *func)
{
    const TK next = peek();

    if (next == TK::NEWLINE)
        func->return_type = new Type(TY::NIL);
    else
        func->return_type = type_spec();
}

// func_def = "#" identifier param_list type_spec? newline block_stmt
FuncDef *Parser::func_def()
{
    expect(TK::HASH);
    expect(TK::IDENT);

    // signature
    std::string_view name = tok_str();
    Func *func = scope_->DeclareFunc();

    // params
    param_list(func);
    ret_type(func);
    expect(TK::NEWLINE);

    // func var
    if (scope_->FindVar(name)) {
        // error
        return nullptr;
    }
    Var *var = scope_->DefineVar(name, NewFuncType(func));

    // func body
    func_ = func;

    enter_scope(func);
    Stmt *body = block_stmt();
    // TODO control flow check to allow implicit return
    for (Stmt *s = body->children; s; s = s->next) {
        if (!s->next) {
            s->next = new ReturnStmt(NewNullExpr());
            break;
        }
    }
    //body->AddStmt(new ReturnStmt(NewNullExpr()));
    leave_scope();

    func_ = nullptr;

    // XXX temp
    FuncDef *fdef = new FuncDef(var, body);
    fdef->funclit_id = funclit_id_++;

    return fdef;
}

Prog *Parser::program()
{
    Prog *prog = new Prog(scope_);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::HASH) {
            FuncDef *fdef = func_def();

            // TODO remove this
            if (fdef->var->name == "main")
                prog->main_func = fdef->var;

            {
                // TODO clean up
                Expr *ident = NewIdentExpr(const_cast<Var *>(fdef->var));
                Expr *init = NewIntLitExpr(fdef->funclit_id);
                prog->AddGlobalVar(new ExprStmt(NewAssignExpr(ident, init, TK::EQ)));
            }

            prog->AddFuncDef(fdef);
            continue;
        }

        if (next == TK::HASH2) {
            class_decl();
            continue;
        }

        if (next == TK::MINUS) {
            prog->AddGlobalVar(var_decl());
            continue;
        }

        if (next == TK::NEWLINE) {
            gettok();
            continue;
        }

        if (next == TK::EOF_) {
            break;
        }

        const Token *tok = gettok();
        const std::string msg = std::string("error: unexpected token: '") +
            GetTokenKindString(next) + "'";
        Error(msg, *src_, tok->pos);
    }

    return prog;
}
