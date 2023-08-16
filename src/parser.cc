#include "parser.h"

Parser::Parser()
{
}

Parser::~Parser()
{
}

Node *Parser::ParseStream(std::istream &stream)
{
    tokenizer_.SetInput(stream);

    return expression();
}

void Parser::SetStringTable(StringTable &string_table)
{
    tokenizer_.SetStringTable(string_table);
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

    case TOK_INTNUM:
        return new IntNumExpr(tok->ival);

    case TOK_IDENT:
        return new IdentExpr(tok->ival); // XXX TMP

    default:
        // ??
        return nullptr;
    }
}

Expr *Parser::add_expr()
{
    Expr *tree = primary_expr();

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TOK_PLUS:
            tree = new AddExpr(tree, primary_expr());
            break;

        default:
            ungettok();
            return tree;
        }
    }
}

Expr *Parser::expression()
{
    return add_expr();
}

Expr *Parser::assign_expr()
{
    Expr *tree = primary_expr();
    const Token *tok = gettok();

    switch (tok->kind) {

    case '=':
        return new AssignExpr(tree, expression());

    default:
        ungettok();
        return tree;
    }
}
