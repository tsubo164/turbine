#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include "ast.h"

class Parser {
public:
    Parser();
    ~Parser();

    Node *ParseStream(std::istream &sstrm);

private:
    Tokenizer tokenizer_;
    Token token_;

    const Token *gettok();
    Node *primary_expr();
};

#endif // _H
