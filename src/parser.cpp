#include "parser.h"
#include <iostream>

Node *Parser::Parse(const std::string &src)
{
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

const Token *Parser::curtok() const
{
    return curr_;
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
        std::cerr << "error: unexpected token: '" << tok->kind << "'" << std::endl;
        std::cerr << "         expected token: '" << kind << "'" << std::endl;
        exit(EXIT_FAILURE);
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
//     primary_expr selector
Expr *Parser::primary_expr()
{
    if (consume(TK::IntNum)) {
        const Token *tok = curtok();
        return new IntNumExpr(tok->ival);
    }

    Expr *expr = nullptr;

    for (;;) {
        const Token *tok = gettok();

        if (tok->kind == TK::Ident) {
            if (peek() == TK::LParen) {
                Func *func = scope_->FindFunc(tok->sval);
                if (!func) {
                    std::cerr << "error: undefined identifier: '" <<
                        tok->sval << "'" << std::endl;
                    exit(EXIT_FAILURE);
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
            tok = curtok();

            Field *fld = expr->type->clss->FindField(tok->sval);

            expr = new SelectExpr(expr, new FieldExpr(fld));
            continue;
        }
        else {
            ungettok();
            return expr;
        }
    }

    return expr;
}

Expr *Parser::add_expr()
{
    Expr *tree = primary_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TK::Plus:
            tree = new AddExpr(tree, primary_expr());
            break;

        default:
            ungettok();
            return tree;
        }
    }
}

Expr *Parser::equal_expr()
{
    Expr *tree = add_expr();

    for (;;) {
        const Token *tok = gettok();

        if (tok->kind == TK::Equal2) {
            tree = new EqualExpr(tree, add_expr());
            continue;
        }
        else {
            ungettok();
            return tree;
        }
    }
}

// assign = equality ("=" assign)?
Expr *Parser::assign_expr()
{
    Expr *tree = equal_expr();
    const Token *tok = gettok();

    switch (tok->kind) {

    case TK::Equal:
        return new AssignExpr(tree, expression());

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

    const Token *tok = curtok();
    if (scope_->FindVar(tok->sval)) {
        std::cerr
            << "error: re-defined variable: '"
            << tok->sval << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Var *var = scope_->DefineVar(tok->sval);
    var->type = type();
    expect(TK::NewLine);

    return var;
}

Field *Parser::field_decl()
{
    expect(TK::Minus);
    expect(TK::Ident);

    const Token *tok = curtok();
    if (scope_->FindField(tok->sval)) {
        std::cerr
            << "error: re-defined variable: '"
            << tok->sval << "'"
            << std::endl;
        std::exit(EXIT_FAILURE);
    }

    Field *fld = scope_->DefineFild(tok->sval);
    fld->type = type();
    expect(TK::NewLine);

    return fld;
}

Class *Parser::class_decl()
{
    expect(TK::Hash2);
    expect(TK::Ident);

    // class name
    const Token *tok = curtok();
    Class *clss = scope_->DefineClass(tok->sval);
    if (!clss) {
        std::cerr << "error: re-defined class: '" << tok->sval << "'" << std::endl;
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

    if (consume(TK::String))
        return new Type(TY::String);

    if (consume(TK::Ident)) {
        const Token *tok = curtok();
        Type *ty = new Type(TY::ClassType);
        ty->clss = scope_->FindClass(tok->sval);
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
        const Token *tok = curtok();
        const char *name = tok->sval;

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
        const Token *tok = curtok();
        const char *name = tok->sval;

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
    const Token *tok = curtok();
    Func *func = scope_->DefineFunc(tok->sval);
    if (!func) {
        std::cerr << "error: re-defined function: '" << tok->sval << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    enter_scope(func);

    param_list(func);
    type();
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

            if (!strcmp(fdef->func->name, "main"))
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
