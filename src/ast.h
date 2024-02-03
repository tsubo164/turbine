#ifndef AST_H
#define AST_H

#include "token.h"

struct Type;
struct Symbol;
struct Field;
struct Func;
struct Var;

struct Expr {
    int kind;
    const struct Type *type;
    struct Pos pos;

    struct Expr *l;
    struct Expr *r;
    struct Expr *list;
    struct Expr *next;

    struct Symbol *sym;
    struct Var *var;
    struct Field *field;

    // literals
    union {
        long ival;
        double fval;
        const char *sval;
        struct Func *func;
    };
    const char *converted;
};

struct Stmt {
    int kind;

    struct Expr* expr;
    struct Expr* cond;
    struct Expr* post;
    struct Stmt* body;
    // children
    struct Stmt *children;
    struct Stmt *next;
};

// Expr
struct Expr *NewNullExpr(void);
struct Expr *NewNilLitExpr(void);
struct Expr *NewBoolLitExpr(bool b);
struct Expr *NewIntLitExpr(long l);
struct Expr *NewFloatLitExpr(double d);
struct Expr *NewStringLitExpr(const char *s);
struct Expr *NewFuncLitExpr(struct Func *func);
struct Expr *NewConversionExpr(struct Expr *from, struct Type *to);
struct Expr *NewIdentExpr(struct Symbol *sym);
struct Expr *NewFieldExpr(struct Field *f);
struct Expr *NewSelectExpr(struct Expr *inst, struct Expr *fld);
struct Expr *NewIndexExpr(struct Expr *ary, struct Expr *idx);
struct Expr *NewCallExpr(struct Expr *callee, struct Pos p);
struct Expr *NewBinaryExpr(struct Expr *L, struct Expr *R, int k);
struct Expr *NewRelationalExpr(struct Expr *L, struct Expr *R, int k);
struct Expr *NewUnaryExpr(struct Expr *L, struct Type *t, int k);
struct Expr *NewAssignExpr(struct Expr *l, struct Expr *r, int k);
struct Expr *NewIncDecExpr(struct Expr *l, int k);

// Stmt
struct Stmt *NewNopStmt(void);
struct Stmt *NewBlockStmt(struct Stmt *children);
struct Stmt *NewOrStmt(struct Expr *cond, struct Stmt *body);
struct Stmt *NewIfStmt(struct Stmt *or_list);
struct Stmt *NewForStmt(struct Expr *init, struct Expr *cond, struct Expr *post, struct Stmt *body);
struct Stmt *NewJumpStmt(int k);
struct Stmt *NewCaseStmt(struct Stmt *conds, struct Stmt *body, int k);
struct Stmt *NewSwitchStmt(struct Expr *cond, struct Stmt *cases);
struct Stmt *NewReturnStmt(struct Expr *e);
struct Stmt *NewExprStmt(struct Expr *e);

bool IsNull(const struct Expr *e);
bool IsGlobal(const struct Expr *e);

int Addr(const struct Expr *e);
bool EvalExpr(const struct Expr *e, long *result);
bool EvalAddr(const struct Expr *e, int *result);

#endif // _H
