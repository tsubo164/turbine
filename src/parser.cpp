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

FuncCallExpr *Parser::arg_list(FuncCallExpr *fcall)
{
    expect(TK::LeftParenthesis);

    if (consume(TK::RightParenthesis))
        return fcall;

    do {
        fcall->AddArgument(expression());
    }
    while (consume(TK::Comma));

    expect(TK::RightParenthesis);
    return fcall;
}

Expr *Parser::primary_expr()
{
    const Token *tok = gettok();

    if (tok->kind == TK::IntNum) {
        return new IntNumExpr(tok->ival);
    }

    if (tok->kind == TK::Ident) {
        if (peek() == TK::LeftParenthesis) {
            Function *func = scope_->FindFunction(tok->sval);
            if (!func) {
                std::cerr << "error: undefined identifier: '" << tok->sval << "'" << std::endl;
                exit(EXIT_FAILURE);
            }
            FuncCallExpr *fcall = new FuncCallExpr(func);
            return arg_list(fcall);
        }
        else {
            // TODO need to correct. arg may be found in the middle of var scope chain.
            Variable *var = scope_->FindVariable(tok->sval);
            if (var) {
                return new IdentExpr(var);
            }
            Argument *arg = func_->FindArgument(tok->sval);
            if (arg) {
                return new ArgExpr(arg);
            }

            std::cerr << "error: undefined identifier: '" << tok->sval << "'" << std::endl;
            exit(EXIT_FAILURE);
            return nullptr;
        }
    }

    std::cerr << "error: unexpected token: '" << tok->kind << "'" << std::endl;
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

Stmt *Parser::if_stmt()
{
    expect(TK::If);
    Expr *cond = expression();
    expect(TK::NewLine);

    BlockStmt *then = block_stmt();
    BlockStmt *els = nullptr;

    if (consume(TK::Else)) {
        expect(TK::NewLine);
        els = block_stmt();
    }

    return new IfStmt(cond, then, els);
}

Stmt *Parser::ret_stmt()
{
    expect(TK::Return);

    const int argc = func_->GetArgumentCount();
    ReturnStmt *stmt = nullptr;

    if (consume(TK::NewLine)) {
        stmt = new ReturnStmt(argc);
    }
    else {
        stmt = new ReturnStmt(expression(), argc);
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
Variable *Parser::var_decl()
{
    expect(TK::Minus);
    expect(TK::Ident);
    const Token *tok = curtok();
    Variable *var = scope_->DefineVariable(tok->sval);
    type();
    expect(TK::NewLine);

    return var;
}

BlockStmt *Parser::block_stmt()
{
    BlockStmt *block = new BlockStmt();
    scope_ = scope_->OpenChild();
    expect(TK::BlockBegin);

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::Minus) {
            var_decl();
            continue;
        }
        else if (next == TK::If) {
            block->AddStatement(if_stmt());
            continue;
        }
        else if (next == TK::Return) {
            block->AddStatement(ret_stmt());
            continue;
        }
        else if (next == TK::BlockEnd) {
            break;
        }
        else {
            block->AddStatement(expr_stmt());
            continue;
        }
    }

    expect(TK::BlockEnd);
    scope_ = scope_->Close();
    return block;
}

void Parser::type()
{
    if (consume(TK::Int))
        return;

    if (consume(TK::String))
        return;

    const Token *tok = gettok();

    std::cerr << "error: not a type name: '" << tok->kind << "'" << std::endl;
    std::exit(EXIT_FAILURE);
}

Function *Parser::param_list(Function *func)
{
    expect(TK::LeftParenthesis);

    if (consume(TK::RightParenthesis))
        return func;

    do {
        expect(TK::Ident);
        const Token *tok = curtok();
        func->DefineArgument(tok->sval);

        type();
    }
    while (consume(TK::Comma));

    expect(TK::RightParenthesis);
    return func;
}

FuncDef *Parser::func_def()
{
    expect(TK::Hash);
    expect(TK::Ident);

    // func name
    const Token *tok = curtok();
    Function *func = scope_->DefineFunction(tok->sval);
    if (!func) {
        std::cerr << "error: re-defined function: '" << tok->sval << "'" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    func = param_list(func);
    type();

    expect(TK::NewLine);

    // func body
    func_ = func;
    BlockStmt *block = block_stmt();
    func_ = nullptr;
    FuncDef *fdef = new FuncDef(func, block);

    // TODO TMP last child scope
    func->scope = scope_->GetLastChild();

    return fdef;
}

Prog *Parser::program()
{
    Prog *prog = new Prog;

    for (;;) {
        const TokenKind next = peek();

        if (next == TK::Hash) {
            prog->AddFuncDef(func_def());
            continue;
        }
        else if (next == TK::Eof) {
            break;
        }
        else {
            std::cerr << "error: unexpected token: '" << next << "'" << std::endl;
            exit(EXIT_FAILURE);
        }
    }

    return prog;
}
