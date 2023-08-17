#include "parser.h"

Node *Parser::ParseStream(std::istream &stream)
{
    tokenizer_.SetInput(stream);

    return program();
}

const Token *Parser::gettok()
{
    if (currtok_ != headtok_) {
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

Expr *Parser::primary_expr()
{
    const Token *tok = gettok();

    switch (tok->kind) {

    case TK::IntNum:
        return new IntNumExpr(tok->ival);

    case TK::Ident:
        return new IdentExpr(tok->str_id);

    default:
        printf("error: unexpected token: %d\n", tok->kind);
        exit(1);
        return nullptr;
    }
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

    // expect ==============
    const Token *tok = gettok();
    if (tok->kind != TK::NewLine && tok->kind != TK::Eof) {
        printf("expect new line or eof%d\n", tok->kind);
        exit(1);
    }

    return stmt;
}

Prog *Parser::program()
{
    Prog *prog = new Prog;

    for (;;) {
        // peek ==============
        const Token *tok = gettok();
        if (tok->kind == TK::Eof)
            return prog;
        ungettok();

        Stmt *stmt = expr_stmt();
        prog->stmts.push_back(stmt);
    }
}
