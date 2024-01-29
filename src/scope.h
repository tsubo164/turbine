#ifndef SCOPE_H
#define SCOPE_H

#include <stdbool.h>
#include <stdint.h>
#include "hashmap.h"
#include "vec.h"

struct Type;

struct Var;
struct Func;
struct Struct;
struct Table;
struct Module;

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
    const struct Type *type;

    union {
        struct Var *var;
        struct Func *func;
        struct Struct *strct;
        struct Table *table;
        struct Module *module;
    };
} Symbol;


struct Scope {
    struct Scope *parent;
    struct Scope *children_;
    struct Scope *child_tail;
    struct Scope *next;

    int var_offset_;

    HashMap symbols;
};

struct Scope *NewScope(struct Scope *parent, int var_offset);
struct Scope *OpenChild(struct Scope *sc);

struct Symbol *DefineVar(struct Scope *sc, const char *name,
        const struct Type *type, bool isglobal);

struct Struct *DefineStruct(struct Scope *sc, const char *name);
struct Struct *FindStruct(const struct Scope *sc, const char *name);

struct Table *DefineTable(struct Scope *sc, const char *name);
struct Module *DefineModule(struct Scope *sc, const char *name);
Symbol *FindSymbol(const struct Scope *sc, const char *name);

int VarSize(const struct Scope *sc);
int TotalVarSize(const struct Scope *sc);
int FieldSize(const struct Scope *sc);

#endif // _H
