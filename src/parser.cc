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

Node *Parser::branch(Node *node, Node *l, Node *r)
{
    node->lhs = l;
    node->rhs = r;
    return node;
}

Node *Parser::primary_expr()
{
    const Token *tok = gettok();
    Node *node = nullptr;

    switch (tok->kind) {

    case TOK_INTNUM:
        node = NewNode(NOD_INTNUM);
        node->ival = tok->ival;
        return node;

    default:
        // ??
        return nullptr;
    }
}

Node *Parser::add_expr()
{
    Node *tree = primary_expr();
    Node *expr = nullptr;

    for (;;) {
        const Token *tok = gettok();

        switch (tok->kind) {

        case TOK_PLUS:
            expr = NewNode(NOD_ADD);
            tree = branch(expr, tree, primary_expr());
            break;

        default:
            ungettok();
            return tree;
        }
    }
}

Node *Parser::expression()
{
    return add_expr();
}
