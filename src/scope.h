#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

class Scope;

struct Var {
    Var(SharedStr Name, int ID, bool global)
        : name(Name), id(ID), is_global(global) {}
    SharedStr name;
    const int id;
    const bool is_global;
};

struct Func {
    Func(SharedStr Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}
    SharedStr name;
    const int id;
    Scope *scope;

    void DeclareParam(SharedStr name);
    int ParamCount() const;
    int VarCount() const;

private:
    int nparams_ = 0;
};

struct Fld {
    Fld(SharedStr Name, int ID)
        : name(Name), id(ID) {}

    SharedStr name;
    const int id;
};

struct Clss {
    Clss(SharedStr Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    SharedStr name;
    const int id;
    Scope *scope;

    void DeclareFild(SharedStr name);
    int FildCount() const;

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

    Fld *DefineFild(const char *name);
    Fld *FindFild(const char *name) const;
    int FildCount() const;

    Func *DefineFunc(const char *name);
    Func *FindFunc(const char *name) const;

    Clss *DefineClss(const char *name);
    Clss *FindClss(const char *name) const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    const Func *func_;
    const Clss *clss_;

    std::map<const char*,Var*> vars_;
    std::map<const char*,Func*> funcs_;
    std::map<const char*,Fld*> flds_;
    std::map<const char*,Clss*> clsses_;
};

#endif // _H
