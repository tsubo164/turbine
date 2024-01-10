#ifndef AST_H
#define AST_H

#include <memory>
#include <vector>
#include "bytecode.h"
#include "scope.h"
#include "lexer.h"
#include "type.h"
#include "escseq.h"

// XXX TEST ==============
#include "compiler.h"

void SetOptimize(bool enable);

typedef union Val {
    long i;
    double f;
    const char *s;
    std::string_view sv;
} Val;


//--------------------------------
// Expr
typedef struct Expr {
    int kind;
    const Type *type;

    Expr *l;
    Expr *r;
    Expr *list;
    Expr *next;

    Var *var;
    Field *field;

    // TODO remove init later
    Val val = {0};
    std::string converted;
    Pos pos;
} Expr;

Expr *NewNullExpr(void);
Expr *NewNilLitExpr(void);
Expr *NewBoolLitExpr(bool b);
Expr *NewIntLitExpr(long l);
Expr *NewFloatLitExpr(double d);
Expr *NewStringLitExpr(std::string_view s);
Expr *NewConversionExpr(Expr *from, Type *to);
Expr *NewIdentExpr(Var *v);
Expr *NewFieldExpr(Field *f);
Expr *NewSelectExpr(Expr *inst, Expr *fld);
Expr *NewIndexExpr(Expr *ary, Expr *idx);
Expr *NewCallExpr(Expr *callee, Pos p);
Expr *NewBinaryExpr(Expr *L, Expr *R, TK k);
Expr *NewRelationalExpr(Expr *L, Expr *R, TK k);
Expr *NewUnaryExpr(Expr *L, Type *t, TK k);
Expr *NewAssignExpr(Expr *l, Expr *r, TK k);
Expr *NewIncDecExpr(Expr *l, TK k);


//--------------------------------
// Stmt
typedef struct Stmt {
    int kind;

    Expr* expr;
    Expr* cond;
    Expr* post;
    Stmt* body;
    // children
    Stmt *children;
    Stmt *next;
} Stmt;

Stmt *NewNopStmt(void);
Stmt *NewBlockStmt(Stmt *children);
Stmt *NewOrStmt(Expr *cond, Stmt *body);
Stmt *NewIfStmt(Stmt *or_list);
Stmt *NewForStmt(Expr *init, Expr *cond, Expr *post, Stmt *body);
Stmt *NewJumpStmt(TK k);
Stmt *NewCaseStmt(Stmt *conds, Stmt *body, TK k);
Stmt *NewSwitchStmt(Expr *cond, Stmt *cases);
Stmt *NewReturnStmt(Expr *e);
Stmt *NewExprStmt(Expr *e);

typedef struct FuncDef {
    // TODO remove this
    const Func *func;
    const Var *var;
    Stmt* body;
    // TODO make FuncLitExpr and remove this
    int funclit_id;
} FuncDef;

FuncDef *NewFuncDef(Var *v, Stmt *body);

struct Prog {
    Prog(Scope *sc) : scope(sc) {}
    void AddFuncDef(FuncDef *func) { funcs.emplace_back(func); }
    void AddGlobalVar(Stmt *gvar) { gvars.emplace_back(gvar); }

    const Scope *scope;
    std::vector<std::unique_ptr<FuncDef>> funcs;
    std::vector<std::unique_ptr<Stmt>> gvars;
    // TODO remove this
    const Var *main_func = nullptr;
};

#endif // _H
