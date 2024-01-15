#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>

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

struct Func {
    Func(Scope *sc, bool builtin = false)
        : scope(sc), is_builtin_(builtin) {}

    Scope *scope;
    const Type *return_type = nullptr;

    void DeclareParam(const char *name, const Type *type);
    const Var *GetParam(int index) const;
    int RequiredParamCount() const;
    int ParamCount() const;

    bool HasSpecialVar() const { return has_special_var_; }
    bool IsVariadic() const { return ellipsis_index_ >= 0; }
    bool IsBuiltin() const { return is_builtin_; }

private:
    std::vector<const Var*> params_;
    bool is_builtin_ = false;
    bool has_special_var_ = false;
    int ellipsis_index_ = -1;
};

typedef struct Field {
    Field(const char *Name, int ID)
        : name(Name), id(ID) {}

    const char *name;
    int id;
    const Type *type = nullptr;
} Field;

struct Class {
    Class(const char *Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    const char *name;
    const int id;
    Scope *scope;

    void DeclareField(const char *name, const Type *type);
    Field *FindField(const char *name) const;
    int FieldCount() const;

    int Size() const;

private:
    int nflds_ = 0;
};

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
    int FieldCount() const;

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

    const Class *clss_ = nullptr;

    Var *vars_ = nullptr;
    Var *vars_tail;

    std::map<const char *,Func*> funcs_;
    std::map<const char *,Field*> flds_;
    std::map<const char *,Class*> clsses_;

    int next_var_id() const;
    int max_var_id() const;
};

#endif // _H
