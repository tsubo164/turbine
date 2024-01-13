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

    Prog *Parse(const std::string &src, const Token *tok, Scope *scope);

private:
    Scope *scope_ = nullptr;
    const std::string *src_;
    Func *func_ = nullptr;

    // TODO remove this
    int funclit_id_ = 0;

    // current token
    const Token *curr_;

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
    const char *tok_str() const;
    int peek();
    void expect(int kind);
    bool consume(int kind);

    // scope
    void enter_scope(Func *func = nullptr);
    void enter_scope(Class *clss);
    void leave_scope();

    // expression
    Expr *arg_list(Expr *call);
    Expr *conv_expr(int kind);
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
    Stmt *or_stmt();
    Stmt *if_stmt();
    Stmt *for_stmt();
    Stmt *jump_stmt();
    Stmt *switch_stmt();
    Stmt *case_stmt(int kind);
    Stmt *ret_stmt();
    Stmt *expr_stmt();
    Stmt *scope_stmt();
    Stmt *nop_stmt();
    Stmt *block_stmt(Func *func = nullptr);

    //
    Type *type_spec();
    Stmt *var_decl();
    Field *field_decl();
    Class *class_decl();
    void field_list(Class *clss);
    void param_list(Func *func);
    void ret_type(Func *func);
    FuncDef *func_def();

    // error
    void error(Pos pos, std::string_view s0, std::string_view s1 = {},
            std::string_view s2 = {}, std::string_view s3 = {},
            std::string_view s4 = {}, std::string_view s5 = {}) const;
    Prog *program();
};

Prog *Parse(const std::string &src, const Token *tok, Scope *scope);

#endif // _H
