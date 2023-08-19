#ifndef PARSER_H
#define PARSER_H

#include "string_table.h"
#include "tokenizer.h"
#include "scope.h"
#include "ast.h"
#include <array>

class Parser {
public:
    Parser(StringTable &string_table, Scope &scope)
        : tokenizer_(string_table), scope_(&scope) {}
    ~Parser() {}

    Node *ParseStream(std::istream &sstrm);
    void SetStringTable();

private:
    Tokenizer tokenizer_;
    Scope *scope_;

    // token buffer
    std::array<Token,8> tokbuf_;
    Token *curr_  = &tokbuf_[0];
    Token *head_  = &tokbuf_[0];
    Token *begin_ = &tokbuf_[0];
    Token *end_   = &tokbuf_[tokbuf_.size()-1];

    // token get
    Token *next() const;
    Token *prev() const;
    const Token *gettok();
    void ungettok();

    //
    TokenKind peek();
    void expect(TokenKind kind);
    void expect_one_of(TokenKind kind0, TokenKind kind1);

    // expressions
    Expr *primary_expr();
    Expr *add_expr();
    Expr *assign_expr();
    Expr *expression();

    Stmt *expr_stmt();
    Prog *program();
};

#endif // _H
