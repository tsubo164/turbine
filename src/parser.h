#ifndef PARSER_H
#define PARSER_H

#include "string_table.h"
#include "tokenizer.h"
#include "ast.h"
#include <array>

class Parser {
public:
    Parser(StringTable &string_table) : tokenizer_(string_table) {}
    ~Parser() {}

    Node *ParseStream(std::istream &sstrm);
    void SetStringTable();

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

    // expressions
    Expr *primary_expr();
    Expr *add_expr();
    Expr *assign_expr();
    Expr *expression();

    Stmt *expr_stmt();
    Prog *program();
};

#endif // _H
