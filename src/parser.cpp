#include "parser.h"
#include "error.h"
#include <iostream>

Node *Parser::Parse(const std::string &src)
{
    src_ = &src;
    lexer_.SetInput(src);

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

CallExpr *Parser::arg_list(CallExpr *call)
{
    expect(TK::LParen);

    if (consume(TK::RParen))
        return call;

    do {
        call->AddArg(expression());
    }
    while (consume(TK::Comma));

    expect(TK::RParen);
    return call;
}

// primary_expr =
//     IntNum |
//     FpNum |
//     StringLit |
//     primary_expr selector
Expr *Parser::primary_expr()
{
    if (consume(TK::IntNum))
        return new IntNumExpr(tok_int(), new Type(TY::Integer));

    if (consume(TK::FpNum))
        return new FpNumExpr(tok_float(), new Type(TY::Float));

    if (consume(TK::StringLit))
        return new StringLitExpr(tok_str(), new Type(TY::String));

    if (consume(TK::LParen)) {
        Expr *expr = expression();
        expect(TK::RParen);
        return expr;
    }

    Expr *expr = nullptr;

    for (;;) {
        const Token *tok = gettok();

        if (tok->kind == TK::Ident) {
            if (peek() == TK::LParen) {
                Func *func = FindBuiltinFunc(tok->sval);
                if (!func) {
                    func = scope_->FindFunc(tok->sval);
                    if (!func) {
                        std::cerr << "error: undefined identifier: '" <<
                            tok->sval << "'" << std::endl;
                        exit(EXIT_FAILURE);
                    }
                }

                CallExpr *call = new CallExpr(func);
                expr = arg_list(call);
            }
            else {
                Var *var = scope_->FindVar(tok->sval);
                if (!var) {
                    std::cerr << "error: undefined identifier: '" <<
                        tok->sval << "'" << std::endl;
                    exit(EXIT_FAILURE);
                }

                expr = new IdentExpr(var);
            }
            continue;
        }
        else if (tok->kind == TK::Period) {
            // TODO FIX
            if (!expr || !expr->type) {
                std::cerr << "error: no type" << std::endl;
                exit(EXIT_FAILURE);
            }
            if (expr->type->kind != TY::ClassType) {
                std::cerr << "error: not a class type" << std::endl;
                exit(EXIT_FAILURE);
            }

            expect(TK::Ident);

            Field *fld = expr->type->clss->FindField(tok_str());

            expr = new SelectExpr(expr, new FieldExpr(fld));
            continue;
        }
        else {
            if (!expr)
                Error("Unknown token", *src_, tok->pos);

            ungettok();
            return expr;
        }
    }

    return nullptr;
}

// unary_expr
//     primary_expr
//     "+" unary_expr
//     "-" unary_expr
//     "!" unary_expr
Expr *Parser::unary_expr()
{
    const Token *tok = gettok();

    switch (tok->kind) {
    case TK::Plus:
    case TK::Minus:
    case TK::EXCL:
        return new UnaryExpr(tok->kind, unary_expr());

    default:
        ungettok();
        return primary_expr();
    }
}

// mul_expr
//     primary_expr
//     mul_expr '*' primary_expr
//     mul_expr '/' primary_expr
//     mul_expr '%' primary_expr
Expr *Parser::mul_expr()
{
    Expr *expr = unary_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TK::STAR:
        case TK::Slash:
        case TK::PERCENT:
            expr = new BinaryExpr(tok->kind, expr, unary_expr());
            break;

        default:
            ungettok();
            return expr;
        }
    }
}

// add_expr
//     mul_expr
//     add_expr '+' mul_expr
//     add_expr '-' mul_expr
Expr *Parser::add_expr()
{
    Expr *tree = mul_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TK::Plus:
            tree = new AddExpr(tree, mul_expr());
            break;

        case TK::Minus:
            tree = new BinaryExpr(tok->kind, tree, mul_expr());
            break;

        default:
            ungettok();
            return tree;
        }
    }
}

// rel_expr = add_expr (rel_op add_expr)*
// rel_op   = "==" | "!=" | "<" | ">" | "<=" | ">="
Expr *Parser::rel_expr()
{
    Expr *expr = add_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::Equal2:
        case TK::ExclEqual:
        case TK::LT:
        case TK::GT:
        case TK::LTE:
        case TK::GTE:
            expr = new BinaryExpr(tok->kind, expr, add_expr());
            continue;

        default:
            ungettok();
            return expr;
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
            expr = new BinaryExpr(tok->kind, expr, rel_expr());
            continue;

        default:
            ungettok();
            return expr;
        }
    }
}

// logor_expr = logand_expr ("&&" logand_expr)*
Expr *Parser::logor_expr()
{
    Expr *tree = logand_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {
        case TK::BAR2:
            tree = new BinaryExpr(tok->kind, tree, logand_expr());
            continue;

        default:
            ungettok();
            return tree;
        }
    }
}

// assign = equality ("=" assign)?
Expr *Parser::assign_expr()
{
    Expr *tree = logor_expr();
    const Token *tok = gettok();

    switch (tok->kind) {
    case TK::Equal:
        return new AssignExpr(tree, expression());

    case TK::PLUS2:
        return new IncExpr(tree);

    default:
        ungettok();
        return tree;
    }
}

Expr *Parser::expression()
{
    return assign_expr();
}

Stmt *Parser::if_stmt()
{
    expect(TK::If);
    Expr *cond = expression();
    expect(TK::NewLine);

    enter_scope();
    BlockStmt *then = block_stmt();
    leave_scope();

    BlockStmt *els = nullptr;
    if (consume(TK::Else)) {
        expect(TK::NewLine);

        enter_scope();
        els = block_stmt();
        leave_scope();
    }

    return new IfStmt(cond, then, els);
}

Stmt *Parser::ret_stmt()
{
    expect(TK::Return);

    ReturnStmt *stmt = nullptr;

    if (consume(TK::NewLine)) {
        stmt = new ReturnStmt();
    }
    else {
        stmt = new ReturnStmt(expression());
        expect(TK::NewLine);
    }

    return stmt;
}

Stmt *Parser::expr_stmt()
{
    ExprStmt *stmt = new ExprStmt(expression());
    expect(TK::NewLine);

    return stmt;
}

// var_decl = - ident int newline
Var *Parser::var_decl()
{
    expect(TK::Minus);
    expect(TK::Ident);

    if (scope_->FindVar(tok_str())) {
        std::cerr
            << "error: re-defined variable: '"
            << tok_str() << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Var *var = scope_->DefineVar(tok_str());
    var->type = type();
    expect(TK::NewLine);

    return var;
}

Field *Parser::field_decl()
{
    expect(TK::Minus);
    expect(TK::Ident);

    if (scope_->FindField(tok_str())) {
        std::cerr
            << "error: re-defined variable: '"
            << tok_str() << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Field *fld = scope_->DefineFild(tok_str());
    fld->type = type();
    expect(TK::NewLine);

    return fld;
}

Class *Parser::class_decl()
{
    expect(TK::Hash2);
    expect(TK::Ident);

    // class name
    Class *clss = scope_->DefineClass(tok_str());
    if (!clss) {
        std::cerr << "error: re-defined class: '" << tok_str() << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    expect(TK::NewLine);
    enter_scope(clss);
    expect(TK::BlockBegin);
    field_list(clss);
    expect(TK::BlockEnd);
    leave_scope();

    return nullptr;
    //return new ClasDef(clss);
}

BlockStmt *Parser::block_stmt()
{
    BlockStmt *block = new BlockStmt();
    expect(TK::BlockBegin);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::Minus) {
            var_decl();
            continue;
        }
        else if (next == TK::If) {
            block->AddStmt(if_stmt());
            continue;
        }
        else if (next == TK::Return) {
            block->AddStmt(ret_stmt());
            continue;
        }
        else if (next == TK::BlockEnd) {
            break;
        }
        else {
            block->AddStmt(expr_stmt());
            continue;
        }
    }

    expect(TK::BlockEnd);
    return block;
}

Type *Parser::type()
{
    if (consume(TK::Int))
        return new Type(TY::Integer);

    if (consume(TK::Float))
        return new Type(TY::Float);

    if (consume(TK::String))
        return new Type(TY::String);

    if (consume(TK::Ident)) {
        Type *ty = new Type(TY::ClassType);
        ty->clss = scope_->FindClass(tok_str());
        return ty;
    }

    const Token *tok = gettok();

    std::cerr << "error: not a type name: '" << tok->kind << "'" << std::endl;
    std::exit(EXIT_FAILURE);

    return nullptr;
}

void Parser::field_list(Class *clss)
{
    expect(TK::Minus);

    do {
        expect(TK::Ident);
        std::string_view name = tok_str();

        clss->DeclareField(name, type());
        expect(TK::NewLine);
    }
    while (consume(TK::Minus));
}

void Parser::param_list(Func *func)
{
    expect(TK::LParen);

    if (consume(TK::RParen))
        return;

    do {
        expect(TK::Ident);
        std::string_view name = tok_str();

        func->DeclareParam(name, type());
    }
    while (consume(TK::Comma));

    expect(TK::RParen);
}

FuncDef *Parser::func_def()
{
    expect(TK::Hash);
    expect(TK::Ident);

    // func name
    Func *func = scope_->DefineFunc(tok_str());
    if (!func) {
        std::cerr << "error: re-defined function: '" << tok_str() << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    enter_scope(func);

    param_list(func);
    func->type = type();
    expect(TK::NewLine);

    // func body
    BlockStmt *block = block_stmt();

    leave_scope();

    return new FuncDef(func, block);
}

Prog *Parser::program()
{
    Prog *prog = new Prog(scope_);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::Hash) {
            FuncDef *fdef = func_def();

            if (fdef->func->name == "main")
                prog->main_func = fdef->func;

            prog->AddFuncDef(fdef);
            continue;
        }

        if (next == TK::Hash2) {
            class_decl();
            continue;
        }

        if (next == TK::Minus) {
            var_decl();
            continue;
        }

        if (next == TK::Eof) {
            break;
        }

        std::cerr << "error: unexpected token: '" << next << "'" << std::endl;
        exit(EXIT_FAILURE);
    }

    return prog;
}
