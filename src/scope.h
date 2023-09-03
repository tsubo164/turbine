#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

class Scope;

struct Var {
    Var(SharedStr name_, int ID, bool global)
        : name(name_), id(ID), is_global(global) {}
    SharedStr name;
    const int id;
    const bool is_global;
};

struct Func {
    Func(SharedStr name_, int id_, Scope *parent_);
    SharedStr name;
    const int id;

    std::unique_ptr<Scope> scope;

    void DeclareParam(SharedStr name);
    int ParamCount() const;
    int VarCount() const;

private:
    int nparams_ = 0;
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

    Func *DefineFunc(const char *name);
    Func *FindFunc(const char *name) const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;

    std::map<const char*,Var*> vars_;
    std::map<const char*,Func*> funcs_;
};

#endif // _H
