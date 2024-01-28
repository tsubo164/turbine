#ifndef SCOPE_H
#define SCOPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hashmap.h"
#include "vec.h"
// XXX TMP
#include "prog.h"

// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, structs.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

typedef struct Type Type;
typedef struct Scope Scope;

struct Var {
    const char *name;
    const Type *type;
    int offset;
    bool is_global;
};


struct Func;

void DeclareParam(struct Func *f, const char *name, const Type *type);
const struct Var *GetParam(const struct Func *f, int index);
int RequiredParamCount(const struct Func *f);
int ParamCount(const struct Func *f);

bool HasSpecialVar(const struct Func *f);
bool IsVariadic(const struct Func *f);
bool IsBuiltin(const struct Func *f);

typedef struct Field {
    const char *name;
    const Type *type;
    int offset;
} Field;

struct Struct {
    const char *name;
    struct Vec fields;
    int size;
};

struct Field *AddField(struct Struct *strct, const char *name, const Type *type);
struct Field *FindField(const struct Struct *strct, const char *name);

//----------------
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
    Scope *scope;
};

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
        struct Var *var;
        Func *func;
        struct Struct *strct;
        struct Table *table;
        struct Module *module;
    };
} Symbol;


struct Scope {
    Scope *parent;
    Scope *children_;
    Scope *child_tail;
    Scope *next;

    int var_offset_;
    //Func *funcs_;

    HashMap symbols;
};

struct Scope *NewScope(struct Scope *parent, int var_offset);
Scope *OpenChild(Scope *sc);

struct Symbol *DefineVar(Scope *sc, const char *name, const Type *type, bool isglobal);

//struct Func *DeclareFunc(struct Scope *parent, bool isbuiltin);

struct Struct *DefineStruct(Scope *sc, const char *name);
struct Struct *FindStruct(const Scope *sc, const char *name);

struct Table *DefineTable(Scope *sc, const char *name);
struct Module *DefineModule(Scope *sc, const char *name);
Symbol *FindSymbol(const struct Scope *sc, const char *name);

int VarSize(const Scope *sc);
int TotalVarSize(const Scope *sc);
int FieldSize(const Scope *sc);

#endif // _H
