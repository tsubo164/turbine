#ifndef PARSER_H
#define PARSER_H

#include "lexer.h"
#include "scope.h"
#include "type.h"
#include "ast.h"
#include <cstdint>
#include <array>

class Parser {
public:
    Parser() {}
    ~Parser() {}

    Prog *Parse(const std::string &src, Scope *scope);

private:
    Lexer lexer_;
    Scope *scope_ = nullptr;
    const std::string *src_;
    Func *func_ = nullptr;

    // TODO remove this
    int funclit_id_ = 0;

    // token buffer
    std::array<Token,8> tokbuf_;
    Token *curr_  = &tokbuf_[0];
    Token *head_  = &tokbuf_[0];
    Token *begin_ = &tokbuf_[0];
    Token *end_   = &tokbuf_[tokbuf_.size()-1];

    // token get
    Token *next() const;
    Token *prev() const;
    const Token *curtok() const;
    const Token *gettok();
    void ungettok();

    // tokens
    Pos tok_pos() const;
    long tok_int() const;
    double tok_float() const;
    std::string_view tok_str() const;
    TokenKind peek();
    void expect(TokenKind kind);
    bool consume(TokenKind kind);

    // scope
    void enter_scope(Func *func = nullptr);
    void enter_scope(Class *clss);
    void leave_scope();

    // expression
    CallExpr *arg_list(CallExpr *fcall);
    Expr *conv_expr(TK kind);
    Expr *primary_expr();
    Expr *unary_expr();
    Expr *mul_expr();
    Expr *add_expr();
    Expr *rel_expr();
    Expr *logand_expr();
    Expr *logor_expr();
    Expr *assign_expr();
    Expr *expression();

    // statement
    OrStmt *or_stmt();
    Stmt *if_stmt();
    Stmt *for_stmt();
    Stmt *jump_stmt();
    Stmt *switch_stmt();
    CaseStmt *case_stmt(TK kind);
    Stmt *ret_stmt();
    Stmt *expr_stmt();
    Stmt *scope_stmt();
    Stmt *nop_stmt();
    BlockStmt *block_stmt(Func *func = nullptr);

    //
    Type *type_spec();
    Stmt *var_decl();
    Field *field_decl();
    Class *class_decl();
    void field_list(Class *clss);
    void param_list(Func *func);
    void ret_type(Func *func);
    FuncDef *func_def2();
    FuncDef *func_def();

    // error
    void error(Pos pos, std::string_view s0, std::string_view s1 = {},
            std::string_view s2 = {}, std::string_view s3 = {},
            std::string_view s4 = {}, std::string_view s5 = {}) const;
    Prog *program();
};

#endif // _H
