#ifndef COMPILER_H
#define COMPILER_H

#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>


#include "bytecode.h"
#include "ast.h"

#define NALLOC(n,type) ((type*) calloc((n),sizeof(type)))
//#define CALLOC(type) NALLOC(1,type)
#define CALLOC(type) (new type())



//--------------------------------
// AST
struct Expr;
struct Stmt;
struct FuncDef;
struct Prog;

void SetOptimize(bool enable);

typedef union Val {
    long i;
    double f;
    const char *s;
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
Expr *NewStringLitExpr(const char *s);
Expr *NewConversionExpr(Expr *from, Type *to);
Expr *NewIdentExpr(Var *v);
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
Stmt *NewJumpStmt(int k);
Stmt *NewCaseStmt(Stmt *conds, Stmt *body, int k);
Stmt *NewSwitchStmt(Expr *cond, Stmt *cases);
Stmt *NewReturnStmt(Expr *e);
Stmt *NewExprStmt(Expr *e);


//--------------------------------
// FuncDef
typedef struct FuncDef {
    // TODO remove this
    const Func *func;
    const Var *var;
    Stmt* body;
    // TODO make FuncLitExpr and remove this
    int funclit_id;

    struct FuncDef *next;
} FuncDef;

FuncDef *NewFuncDef(Var *v, Stmt *body);


//--------------------------------
// Prog
typedef struct Prog {
    const Scope *scope;
    FuncDef *funcs;
    Stmt* gvars;
    // TODO remove this
    const Var *main_func;
} Prog;

Prog *NewProg(Scope *sc);

bool IsNull(const Expr *e);
bool IsGlobal(const Expr *e);

int Addr(const Expr *e);
bool EvalExpr(const Expr *e, long *result);
bool EvalAddr(const Expr *e, int *result);

// print
void PrintProg(const Prog *p, int depth);


// codegen
class Bytecode;
void GenerateCode(Bytecode *code, const Prog *prog);


// str
int ConvertEscSeq(const char *s, std::string &converted);

const char *intern(const char *str);

#endif // _H
