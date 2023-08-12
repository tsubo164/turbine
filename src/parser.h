#ifndef PARSER_H
#define PARSER_H

#include "tokenizer.h"
#include "ast.h"
#include <array>

class Parser {
public:
    Parser();
    ~Parser();

    Node *ParseStream(std::istream &sstrm);

private:
    Tokenizer tokenizer_;

    // token buffer
    static constexpr int BUFSIZE = 8;
    static constexpr int ENDTOK = BUFSIZE - 1;
    std::array<Token,BUFSIZE> token_buf_;
    int currtok_ = 0;
    int headtok_ = 0;

    const Token *gettok();
    void ungettok();

    // ast helpers
    Node *branch(Node *node, Node *l, Node *r);

    // expressions
    Node *primary_expr();
    Node *add_expr();
    Node *expression();
};

#endif // _H
