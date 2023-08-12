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

    return primary_expr();
}

const Token *Parser::gettok()
{
    tokenizer_.Get(token_);
    return &token_;
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

    return nullptr;
}
