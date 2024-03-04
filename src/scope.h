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
    bool is_global;
    bool is_param;
};

struct FuncType {
    const struct Type *return_type;
    struct Vec param_types;
    bool is_builtin;
    bool is_variadic;
    bool has_special_var;
};

struct Func {
    const char *name;
    const char *fullname;
    const struct Type *return_type;
    struct Vec params;
    int size;
    int id;

    bool is_builtin;
    bool is_variadic;
    bool has_special_var;

    struct Scope *scope;
    struct Stmt *body;
    struct FuncType *func_type;
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
    const struct Func *main_func;
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
    int size;

    struct Vec syms;
    struct HashMap symbols;
};

// Scope
struct Scope *NewScope(struct Scope *parent);

// Symbol
struct Symbol *NewSymbol(int kind, const char *name, const struct Type *type);
struct Symbol *FindSymbol(const struct Scope *sc, const char *name);

// Var
struct Symbol *DefineVar(struct Scope *sc, const char *name,
        const struct Type *type, bool isglobal);

// Func
struct Func *DeclareFunc(struct Scope *parent, const char *name, const char *modulefile);
struct Func *DeclareBuiltinFunc(struct Scope *parent, const char *name);
struct FuncType *MakeFuncType(struct Func *func);
void DeclareParam(struct Func *f, const char *name, const struct Type *type);
const struct Var *GetParam(const struct Func *f, int index);
const struct Type *GetParamType(const struct FuncType *func_type, int index);
int RequiredParamCount(const struct FuncType *func_type);

// Struct
struct Struct *DefineStruct(struct Scope *sc, const char *name);
struct Struct *FindStruct(const struct Scope *sc, const char *name);
struct Field *AddField(struct Struct *strct, const char *name, const struct Type *type);
struct Field *FindField(const struct Struct *strct, const char *name);

// Table
struct Table *DefineTable(struct Scope *sc, const char *name);

// Module
struct Module *DefineModule(struct Scope *sc, const char *filename, const char *modulename);

#endif // _H
