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
        if (!scope_->FindVariable(tok->str_id)) {
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

Stmt *Parser::ret_stmt()
{
    expect(TK::Return);

    ReturnStmt *stmt = nullptr;
    const TokenKind next = peek();

    if (next == TK::NewLine || next == TK::Eof) {
        stmt = new ReturnStmt();
        // TODO cosume()?
        gettok();
    }
    else {
        stmt = new ReturnStmt(expression());
        expect_one_of(TK::NewLine, TK::Eof);
    }

    return stmt;
}

Stmt *Parser::expr_stmt()
{
    ExprStmt *stmt = new ExprStmt(expression());
    expect_one_of(TK::NewLine, TK::Eof);

    return stmt;
}

Stmt *Parser::statement()
{
    const TokenKind next = peek();

    if (next == TK::Return) {
        expect(TK::Return);
        return new ReturnStmt(expression());
    }
    else {
        return expr_stmt();
    }
}

FuncDef *Parser::func_def()
{
    expect(TK::Hash1);
    expect(TK::Ident);

    const Token *tok = curtok();
    Function *func = scope_->DefineFunction(tok->str_id);
    if (!func) {
        std::cerr << "error: re-defined function" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    func->scope = scope_;

    expect_one_of(TK::NewLine, TK::Eof);

    FuncDef *fdef = new FuncDef();
    fdef->func = func;

    // stmt_list
    for (;;) {
        const TokenKind next = peek();

        if (next == TK::If) {
            //fdef->AddStmt(if_stmt());
            //continue;
        }
        else if (next == TK::Return) {
            fdef->AddStmt(ret_stmt());
            continue;
        }
        else if (next == TK::Eof) {
            return fdef;
        }
        else {
            fdef->AddStmt(expr_stmt());
            continue;
        }
    }

    return fdef;
}

Prog *Parser::program()
{
    Prog *prog = new Prog;

    FuncDef *func = func_def();
    prog->AddFuncDef(func);

    return prog;
}
