#ifndef PARSER_H
#define PARSER_H

#include "string_table.h"
#include "tokenizer.h"
#include "scope.h"
#include "type.h"
#include "ast.h"
#include <cstdint>
#include <array>

class Parser {
public:
    Parser(StringTable &string_table, Scope &scope)
        : tokenizer_(string_table), scope_(&scope) {}
    ~Parser() {}

    Node *ParseStream(std::istream &sstrm);

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
    const Token *curtok() const;
    TokenKind peek();
    void expect(TokenKind kind);
    bool consume(TokenKind kind);

    // scope
    void enter_scope(Func *func = nullptr);
    void enter_scope(Clss *clss);
    void leave_scope();

    // expression
    FuncCallExpr *arg_list(FuncCallExpr *fcall);
    Expr *primary_expr();
    Expr *add_expr();
    Expr *equal_expr();
    Expr *assign_expr();
    Expr *expression();

    // statement
    Stmt *if_stmt();
    Stmt *ret_stmt();
    Stmt *expr_stmt();
    BlockStmt *block_stmt();

    //
    Type *type();
    Var *var_decl();
    Fld *field_decl();
    Clss *class_decl();
    void field_list(Clss *clss);
    void param_list(Func *func);
    FuncDef *func_def();

    Prog *program();
};

#endif // _H
