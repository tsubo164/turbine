#ifndef SCOPE_H
#define SCOPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hashmap.h"
#include "vec.h"

struct Type;

struct Var {
    const char *name;
    const struct Type *type;
    int offset;
    int id;
    bool is_global;
};

struct Func {
    const char *name;
    const char *fullname;
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
    struct HashMap rows;
};

struct Module {
    const char *name;
    struct Scope *scope;
    struct Stmt* gvars;
    struct Vec funcs;

    const char *filename;
    const char *src;
    // TODO remove this
    const struct Var *main_func;
};

enum SymbolKind {
    SYM_VAR,
    SYM_FUNC,
    SYM_TABLE,
    SYM_STRUCT,
    SYM_MODULE,
    SYM_SCOPE,
};

struct Symbol {
    int kind;
    int id;
    const char *name;
    const struct Type *type;

    union {
        struct Var *var;
        struct Func *func;
        struct Struct *strct;
        struct Table *table;
        struct Module *module;
        struct Scope *scope;
    };
};

struct Scope {
    struct Scope *parent;
    struct Scope *children_;
    struct Scope *child_tail;
    struct Scope *next;

    int cur_offset;

    struct Vec syms;
    struct HashMap symbols;
};

struct Scope *NewScope(struct Scope *parent, int var_offset);

struct Symbol *DefineVar(struct Scope *sc, const char *name,
        const struct Type *type, bool isglobal);

struct Struct *DefineStruct(struct Scope *sc, const char *name);
struct Struct *FindStruct(const struct Scope *sc, const char *name);

struct Table *DefineTable(struct Scope *sc, const char *name);
struct Module *DefineModule(struct Scope *sc, const char *filename, const char *modulename);

struct Symbol *NewSymbol(int kind, const char *name, const struct Type *type);
struct Symbol *FindSymbol(const struct Scope *sc, const char *name);

int VarSize(const struct Scope *sc);
int TotalVarSize(const struct Scope *sc);
int FieldSize(const struct Scope *sc);

// Func
struct Func *AddFunc(struct Scope *parent, const char *modulefile, const char *name);
struct Func *AddBuiltinFunc(struct Scope *parent, const char *name);

void DeclareParam(struct Func *f, const char *name, const struct Type *type);
const struct Var *GetParam(const struct Func *f, int index);
int RequiredParamCount(const struct Func *f);

// Struct
struct Field *AddField(struct Struct *strct, const char *name, const struct Type *type);
struct Field *FindField(const struct Struct *strct, const char *name);

#endif // _H
