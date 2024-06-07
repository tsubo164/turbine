#ifndef AST_H
#define AST_H

#include "token.h"
#include <stdint.h>

struct Type;
struct Symbol;
struct Var;
struct Func;
struct Field;
struct Struct;

struct Expr {
    int kind;
    const struct Type *type;
    struct Pos pos;

    struct Expr *l;
    struct Expr *r;
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
    struct Stmt* init;
    struct Expr* cond;
    struct Stmt* post;
    struct Stmt* body;
    // children
    struct Stmt *children;
    struct Stmt *next;
};

// Expr
struct Expr *NewNilLitExpr(void);
struct Expr *NewBoolLitExpr(bool b);
struct Expr *NewIntLitExpr(long l);
struct Expr *NewFloatLitExpr(double d);
struct Expr *NewStringLitExpr(const char *s);
struct Expr *NewFuncLitExpr(struct Func *func);
struct Expr *NewArrayLitExpr(struct Expr *elems, int len);
struct Expr *NewStructLitExpr(struct Struct *strct, struct Expr *fields);
struct Expr *NewConversionExpr(struct Expr *from, struct Type *to);
struct Expr *NewIdentExpr(struct Symbol *sym);
struct Expr *NewFieldExpr(struct Field *f);
struct Expr *NewSelectExpr(struct Expr *inst, struct Expr *fld);
struct Expr *NewIndexExpr(struct Expr *ary, struct Expr *idx);
struct Expr *NewCallExpr(struct Expr *callee, struct Pos p);
struct Expr *NewBinaryExpr(struct Expr *L, struct Expr *R, int k);
struct Expr *NewRelationalExpr(struct Expr *L, struct Expr *R, int k);
struct Expr *NewUnaryExpr(struct Expr *L, struct Type *t, int k);
struct Expr *NewElementExpr(struct Expr *key, struct Expr *val);

// Stmt
struct Stmt *NewNopStmt(void);
struct Stmt *NewBlockStmt(struct Stmt *children);
struct Stmt *NewOrStmt(struct Expr *cond, struct Stmt *body);
struct Stmt *NewIfStmt(struct Stmt *or_list);
struct Stmt *NewForStmt(struct Stmt *init, struct Expr *cond, struct Stmt *post,
        struct Stmt *body);
struct Stmt *NewJumpStmt(int k);
struct Stmt *NewCaseStmt(struct Expr *conds, struct Stmt *body, int kind);
struct Stmt *NewSwitchStmt(struct Expr *cond, struct Stmt *cases);
struct Stmt *NewReturnStmt(struct Expr *e);
struct Stmt *NewExprStmt(struct Expr *e);
struct Stmt *NewAssignStmt(struct Expr *l, struct Expr *r, int kind);
struct Stmt *NewInitStmt(struct Expr *l, struct Expr *r);
struct Stmt *NewIncDecStmt(struct Expr *l, int kind);

bool IsNull(const struct Expr *e);
bool IsGlobal(const struct Expr *e);
bool IsMutable(const struct Expr *e);

const struct Var *FindRootObject(const struct Expr *e);

int Addr(const struct Expr *e);
bool EvalExpr(const struct Expr *e, int64_t *result);
bool EvalAddr(const struct Expr *e, int *result);

#endif // _H
