#include "parser.h"
#include <iostream>

Node *Parser::ParseStream(std::istream &stream)
{
    tokenizer_.SetInput(stream);

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
        tokenizer_.Get(curr_);
        head_ = curr_;
        return curr_;
    }
}

void Parser::ungettok()
{
    curr_ = prev();
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
        std::cout << "unexpected token: " << static_cast<int>(tok->kind) << std::endl;
        exit(EXIT_FAILURE);
    }
}

void Parser::expect_one_of(TokenKind kind0, TokenKind kind1)
{
    const Token *tok = gettok();
    if (tok->kind != kind0 && tok->kind != kind1) {
        std::cerr << "unexpected token: " << static_cast<int>(tok->kind) << std::endl;
        exit(EXIT_FAILURE);
    }
}

Expr *Parser::primary_expr()
{
    const Token *tok = gettok();

    if (tok->kind == TK::IntNum) {
        return new IntNumExpr(tok->ival);
    }

    if (tok->kind == TK::Ident) {
        if (!scope_->FindVarialbe(tok->str_id)) {
            scope_->DefineVariable(tok->str_id);
        }
        return new IdentExpr(tok->str_id);
    }

    std::cerr << "unexpected token: " << static_cast<int>(tok->kind) << std::endl;
    exit(EXIT_FAILURE);
    return nullptr;
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

Stmt *Parser::expr_stmt()
{
    ExprStmt *stmt = new ExprStmt(expression());

    expect_one_of(TK::NewLine, TK::Eof);

    return stmt;
}

FuncDef *Parser::func_def()
{
    expect(TK::Hash1);
    expect(TK::Ident);
    expect_one_of(TK::NewLine, TK::Eof);

    FuncDef *func = new FuncDef();

    // stmt list
    for (;;) {
        const TokenKind next = peek();

        if (next == TK::If) {
            //prog->AddStmt(if_stmt());
            //continue;
        }

        if (next == TK::Eof)
            return func;

        func->AddStmt(expr_stmt());
    }

    return func;
}

Prog *Parser::program()
{
    Prog *prog = new Prog;

    FuncDef *func = func_def();
    prog->AddFuncDef(func);

    return prog;
}
