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

    case TOK_IDENT:
        node = NewNode(NOD_IDENT);
        node->ival = tok->str_id; // XXX TMP
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

Node *Parser::assign_expr()
{
    Node *tree = primary_expr();
    Node *assg = nullptr;
    const Token *tok = gettok();

    switch (tok->kind) {

    case '=':
        assg = NewNode(NOD_ASSIGN);
        assg->lhs = tree;
        assg->rhs = expression();
        return assg;

    default:
        ungettok();
        return tree;
    }
}
