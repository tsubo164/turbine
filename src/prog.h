#ifndef PROG_H
#define PROG_H

#include <stdbool.h>
#include "vec.h"

struct Func;
struct Var;
struct Stmt;
struct Type;

struct Func {
    const char *name;
    const struct Type *return_type;
    struct Vec params;
    int id;

    struct Scope *scope;
    struct Stmt *body;

    bool is_builtin;
    bool has_special_var;
    // TODO remove this
    int ellipsis_index;
};

struct Prog {
    const struct Scope *scope;
    struct Vec funcs;
    struct Vec builtinfuncs;
    struct Stmt* gvars;

    // TODO remove this
    const struct Var *main_func;
};

struct Func *AddFunc(struct Prog *prog, const char *name, struct Scope *parent);
struct Func *AddBuiltinFunc(struct Prog *prog, const char *name, struct Scope *parent);

#endif // _H
