#include "parser.h"
#include <iostream>

Node *Parser::ParseStream(std::istream &stream)
{
    tokenizer_.SetInput(stream);

    return program();
}

const Token *Parser::gettok()
{
    if (currtok_ != headtok_) {
        if (currtok_ == ENDTOK)
            currtok_ = 0;
        else
            currtok_++;
        return &token_buf_[currtok_];
    }
    else {
        if (currtok_ == ENDTOK)
            currtok_ = 0;
        else
            currtok_++;

        tokenizer_.Get(token_buf_[currtok_]);
        headtok_ = currtok_;

        return &token_buf_[currtok_];
    }
}

void Parser::ungettok()
{
    if (currtok_ == 0)
        currtok_ = ENDTOK;
    else
        currtok_--;
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

// assign = equality ("=" assign)?
Expr *Parser::assign_expr()
{
    Expr *tree = add_expr();
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

Prog *Parser::program()
{
    Prog *prog = new Prog;

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::If) {
            //prog->AddStmt(if_stmt());
            //continue;
        }

        if (next == TK::Eof)
            return prog;

        prog->AddStmt(expr_stmt());
    }
}
