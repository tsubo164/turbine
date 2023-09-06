#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

struct Type;
class Scope;

struct Var {
    Var(SharedStr Name, int ID, bool global)
        : name(Name), id(ID), is_global(global) {}
    SharedStr name;
    const int id;
    const bool is_global;

    Type *type = nullptr;
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

    Type *type = nullptr;
};
typedef Fld Field;

struct Clss {
    Clss(SharedStr Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    SharedStr name;
    const int id;
    Scope *scope;

    void DeclareField(SharedStr name);
    Field *FindField(const char *name) const;
    int FieldCount() const;

    int Size() const;

private:
    int nflds_ = 0;
};
typedef Clss Class;

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
    Fld *FindField(const char *name) const;
    int FieldCount() const;

    Func *DefineFunc(const char *name);
    Func *FindFunc(const char *name) const;

    Clss *DefineClss(const char *name);
    Clss *FindClss(const char *name) const;

    int VarSize() const;
    int FieldSize() const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    const Func *func_ = nullptr;
    const Clss *clss_ = nullptr;

    std::map<const char*,Var*> vars_;
    std::map<const char*,Func*> funcs_;
    std::map<const char*,Fld*> flds_;
    std::map<const char*,Clss*> clsses_;
};

#endif // _H
