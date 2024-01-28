#ifndef PROG_H
#define PROG_H

#include <stdbool.h>
#include "vec.h"

struct Func;
struct Var;
struct Stmt;
struct Type;

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

typedef struct Func {
    // TODO Store func to prog
    const char *name;
    const struct Type *return_type;
    struct Vec params;

    struct Scope *scope;
    struct Stmt *body;

    bool is_builtin;
    bool has_special_var;
    int ellipsis_index;
} Func;

//--------------------------------
// Prog
typedef struct Prog {
    const struct Scope *scope;
    struct Vec funcdefs;
    //struct Vec funcs;
    struct Stmt* gvars;

    // TODO remove this
    const struct Var *main_func;
    int funclit_id;
} Prog;

struct Func *DeclareFunc(struct Scope *parent, bool isbuiltin);

#endif // _H
