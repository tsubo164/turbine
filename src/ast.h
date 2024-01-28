#ifndef AST_H
#define AST_H

#include "scope.h"
#include "token.h"
#include "type.h"

//--------------------------------
// Expr
typedef struct Expr Expr;
struct Expr {
    int kind;
    const Type *type;

    Expr *l;
    Expr *r;
    Expr *list;
    Expr *next;

    Symbol *sym;
    struct Var *var;
    Field *field;

    union {
        long ival;
        double fval;
        const char *sval;
    };
    const char *converted;
    Pos pos;
};

Expr *NewNullExpr(void);
Expr *NewNilLitExpr(void);
Expr *NewBoolLitExpr(bool b);
Expr *NewIntLitExpr(long l);
Expr *NewFloatLitExpr(double d);
Expr *NewStringLitExpr(const char *s);
Expr *NewConversionExpr(Expr *from, Type *to);
Expr *NewIdentExpr(struct Symbol *sym);
Expr *NewFieldExpr(Field *f);
Expr *NewSelectExpr(Expr *inst, Expr *fld);
Expr *NewIndexExpr(Expr *ary, Expr *idx);
Expr *NewCallExpr(Expr *callee, Pos p);
Expr *NewBinaryExpr(Expr *L, Expr *R, int k);
Expr *NewRelationalExpr(Expr *L, Expr *R, int k);
Expr *NewUnaryExpr(Expr *L, Type *t, int k);
Expr *NewAssignExpr(Expr *l, Expr *r, int k);
Expr *NewIncDecExpr(Expr *l, int k);


//--------------------------------
// Stmt
typedef struct Stmt Stmt;
struct Stmt {
    int kind;

    Expr* expr;
    Expr* cond;
    Expr* post;
    Stmt* body;
    // children
    Stmt *children;
    Stmt *next;
};

Stmt *NewNopStmt(void);
Stmt *NewBlockStmt(Stmt *children);
Stmt *NewOrStmt(Expr *cond, Stmt *body);
Stmt *NewIfStmt(Stmt *or_list);
Stmt *NewForStmt(Expr *init, Expr *cond, Expr *post, Stmt *body);
Stmt *NewJumpStmt(int k);
Stmt *NewCaseStmt(Stmt *conds, Stmt *body, int k);
Stmt *NewSwitchStmt(Expr *cond, Stmt *cases);
Stmt *NewReturnStmt(Expr *e);
Stmt *NewExprStmt(Expr *e);

bool IsNull(const Expr *e);
bool IsGlobal(const Expr *e);

int Addr(const Expr *e);
bool EvalExpr(const Expr *e, long *result);
bool EvalAddr(const Expr *e, int *result);

#endif // _H
