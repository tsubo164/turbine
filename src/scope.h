#ifndef SCOPE_H
#define SCOPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hashmap.h"
// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, classes.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

typedef struct Type Type;
typedef struct Scope Scope;

typedef struct Var {
    const char *name;
    const Type *type;
    int id;
    bool is_global;

    struct Var *next;
} Var;

typedef struct Func {
    Scope *scope;
    const Type *return_type;
    Var *params;
    int param_count;

    bool is_builtin;
    bool has_special_var;
    int ellipsis_index;

    struct Func *next;
} Func;

void DeclareParam(Func *f, const char *name, const Type *type);
const Var *GetParam(const Func *f, int index);
int RequiredParamCount(const Func *f);
int ParamCount(const Func *f);

bool HasSpecialVar(const Func *f);
bool IsVariadic(const Func *f);
bool IsBuiltin(const Func *f);

typedef struct Field {
    const char *name;
    int id;
    const Type *type;
    struct Field *next;
} Field;

typedef struct Class {
    const char *name;
    int id;
    Scope *scope;
    int nflds_;

    struct Class *next;
} Class;

void DeclareField(Class *c, const char *name, const Type *type);
Field *FindField(const Class *c, const char *name);
int ClassSize(const Class *c);
int FieldCount(const Class *c);

//----------------
typedef struct Row {
    const char *name;
    union {
        int64_t ival;
        double fval;
        const char *sval;
    };
} Row;

typedef struct Table {
    const char *name;
    HashMap rows;
    Scope *scope;
} Table;


enum SymbolKind {
    SYM_VAR,
    SYM_FUNC,
    SYM_TABLE,
    SYM_STRUCT,
    SYM_MODULE,
};

typedef struct Symbol {
    int kind;
    int id;
    const char *name;
    const Type *type;

    union {
        Var *var;
        Func *func;
        Class *strct;
        Table *table;
    };
} Symbol;


struct Scope {
    Scope *parent_;
    Scope *children_;
    Scope *child_tail;
    Scope *next;

    int level_;
    int var_offset_;
    int field_offset_;
    int class_offset_;

    const Class *clss_;
    Var *vars_;
    Var *vars_tail;
    Func *funcs_;
    Field *flds_;
    Field *fld_tail;
    Class *clsses_;
    Class *clsses_tail;

    HashMap tables;
    HashMap symbols;
};

Scope *OpenChild(Scope *sc);
Scope *Close(const Scope *sc);
bool IsGlobalScope(const Scope *sc);

struct Symbol *DefineVar(Scope *sc, const char *name, const Type *type);
Var *FindVar(const Scope *sc, const char *name, bool find_in_parents);

Func *DeclareFunc(Scope *sc);
const Var *FindFunc(const Scope *sc, const char *name);

Field *DefineFild(Scope *sc, const char *name);
Field *FindClassField(const Scope *sc, const char *name);

Class *DefineClass(Scope *sc, const char *name);
Class *FindClass(const Scope *sc, const char *name);

Table *DefineTable(Scope *sc, const char *name);
Symbol *FindSymbol(Scope *sc, const char *name);

int VarSize(const Scope *sc);
int TotalVarSize(const Scope *sc);
int FieldSize(const Scope *sc);
void PrintScope(const Scope *sc, int depth);

#endif // _H
