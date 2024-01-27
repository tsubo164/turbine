#ifndef PROG_H
#define PROG_H

struct Func;
struct Var;
struct Stmt;

//--------------------------------
// FuncDef
typedef struct FuncDef {
    // TODO remove this
    const struct Func *func;
    struct Symbol *sym;
    const struct Var *var;
    struct Stmt* body;
    // TODO make FuncLitExpr and remove this
    int funclit_id;

    struct FuncDef *next;
} FuncDef;

FuncDef *NewFuncDef(struct Symbol *sym, struct Stmt *body);


//--------------------------------
// Prog
typedef struct Prog {
    const struct Scope *scope;
    // TODO use Vec
    FuncDef *funcs;
    FuncDef *funcs_tail;
    struct Stmt* gvars;
    // TODO remove this
    const struct Var *main_func;
    int funclit_id;
} Prog;

void PrintProg(const struct Prog *prog, int depth);

#endif // _H
