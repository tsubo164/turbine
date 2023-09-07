#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, classes.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

struct Type;
class Scope;

struct Var {
    Var(const char *Name, int ID, bool global)
        : name(Name), id(ID), is_global(global) {}

    const char *name;
    const int id;
    const bool is_global;

    const Type *type = nullptr;
};

struct Func {
    Func(const char *Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    const char *name;
    const int id;
    Scope *scope;

    void DeclareParam(const char *name, const Type *type);
    int ParamCount() const;
    int VarCount() const;

private:
    int nparams_ = 0;
};

struct Field {
    Field(const char *Name, int ID)
        : name(Name), id(ID) {}

    const char *name;
    const int id;

    const Type *type = nullptr;
};

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
    Scope(Scope *parent);
    ~Scope();

    Scope *OpenChild();
    Scope *Close() const;
    Scope *Parent() const;
    bool HasParent() const;
    bool IsGlobal() const;

    Var *DefineVar(const char *name);
    Var *FindVar(const char *name) const;
    int VarCount() const;

    Field *DefineFild(const char *name);
    Field *FindField(const char *name) const;
    int FieldCount() const;

    Func *DefineFunc(const char *name);
    Func *FindFunc(const char *name) const;

    Class *DefineClass(const char *name);
    Class *FindClass(const char *name) const;

    int VarSize() const;
    int FieldSize() const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    const Func *func_ = nullptr;
    const Class *clss_ = nullptr;

    std::map<const char*,Var*> vars_;
    std::map<const char*,Func*> funcs_;
    std::map<const char*,Field*> flds_;
    std::map<const char*,Class*> clsses_;
};

#endif // _H
