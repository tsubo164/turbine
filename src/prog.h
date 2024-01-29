#ifndef PROG_H
#define PROG_H

#include <stdbool.h>
#include <stdint.h>
#include "hashmap.h"
#include "vec.h"

struct Stmt;
struct Type;
struct Scope;

struct Var {
    const char *name;
    const struct Type *type;
    int offset;
    bool is_global;
};

struct Func {
    const char *name;
    const struct Type *return_type;
    struct Vec params;
    int id;

    bool is_builtin;
    bool is_variadic;
    bool has_special_var;

    struct Scope *scope;
    struct Stmt *body;
};

struct Field {
    const char *name;
    const struct Type *type;
    int offset;
};

struct Struct {
    const char *name;
    struct Vec fields;
    int size;
};

struct Row {
    const char *name;
    union {
        int64_t ival;
        double fval;
        const char *sval;
    };
};

struct Table {
    const char *name;
    HashMap rows;
};

struct Module {
    const char *name;
    struct Scope *scope;
};

struct Prog {
    const struct Scope *scope;
    struct Vec vars;
    struct Vec funcs;
    struct Vec builtinfuncs;
    struct Stmt* gvars;

    // TODO remove this
    const struct Var *main_func;
};

// Var
struct Var *AddVar(struct Prog *prog, const char *Name, const struct Type *t,
        int offset, bool isglobal);

// Func
struct Func *AddFunc(struct Prog *prog, const char *name, struct Scope *parent);
struct Func *AddBuiltinFunc(struct Prog *prog, const char *name, struct Scope *parent);

void DeclareParam(struct Func *f, const char *name, const struct Type *type);
const struct Var *GetParam(const struct Func *f, int index);
int RequiredParamCount(const struct Func *f);

// Struct
struct Field *AddField(struct Struct *strct, const char *name, const struct Type *type);
struct Field *FindField(const struct Struct *strct, const char *name);

#endif // _H
