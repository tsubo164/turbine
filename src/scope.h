#ifndef SCOPE_H
#define SCOPE_H

#include <vector>

// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, classes.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

struct Type;
class Scope;

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

class Scope {
public:
    Scope();
    Scope(Scope *parent, int level, int var_offset);
    ~Scope();

    Scope *OpenChild();
    Scope *Close() const;
    bool IsGlobal() const;

    Var *DefineVar(const char *name, const Type *type);
    Var *FindVar(const char *name, bool find_in_parents = true) const;

    Field *DefineFild(const char *name);
    Field *FindField(const char *name) const;

    Func *DeclareFunc();
    const Var *FindFunc(const char *name) const;

    Class *DefineClass(const char *name);
    Class *FindClass(const char *name) const;

    int VarSize() const;
    int TotalVarSize() const;
    int FieldSize() const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    const int level_;
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

#endif // _H
