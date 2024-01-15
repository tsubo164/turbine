#ifndef SCOPE_H
#define SCOPE_H

#include <vector>

// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, classes.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

struct Type;
struct Scope;

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
int Size(const Class *c);
int FieldCount(const Class *c);

typedef struct Scope Scope;
struct Scope {

    Field *DefineFild(const char *name);
    Field *FindField(const char *name) const;

    Func *DeclareFunc();
    const Var *FindFunc(const char *name) const;

    Class *DefineClass(const char *name);
    Class *FindClass(const char *name) const;

    int VarSize() const;
    int TotalVarSize() const;
    int FieldSize() const;

    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    int level_;
    int var_offset_ = 0;
    int field_offset_ = 0;
    int class_offset_ = 0;

    const Class *clss_ = nullptr;

    Var *vars_ = nullptr;
    Var *vars_tail;
    Func *funcs_ = nullptr;
    Field *flds_ = nullptr;
    Field *fld_tail;
    Class *clsses_ = nullptr;
    Class *clsses_tail;

    int next_var_id() const;
    int max_var_id() const;
};

Scope *OpenChild(Scope *sc);
Scope *Close(const Scope *sc);
bool IsGlobal(const Scope *sc);

Var *DefineVar(Scope *sc, const char *name, const Type *type);
Var *FindVar(const Scope *sc, const char *name, bool find_in_parents);

void PrintScope(const Scope *sc, int depth);

#endif // _H
