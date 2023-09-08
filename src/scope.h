#ifndef SCOPE_H
#define SCOPE_H

#include <string_view>
#include <vector>
#include <map>

// Scope and objects that are managed by scope.
// Objects are variables, functions, fields, classes.
// Objects have ownership of their contents like type, children, etc.
// and they are responsible for memory management.

struct Type;
class Scope;

struct Var {
    Var(std::string_view Name, int ID, bool global)
        : name(Name), id(ID), is_global(global) {}

    std::string_view name;
    const int id;
    const bool is_global;

    const Type *type = nullptr;
};

struct Func {
    Func(std::string_view Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    std::string_view name;
    const int id;
    Scope *scope;

    void DeclareParam(std::string_view name, const Type *type);
    int ParamCount() const;
    int VarCount() const;

private:
    int nparams_ = 0;
};

struct Field {
    Field(std::string_view Name, int ID)
        : name(Name), id(ID) {}

    std::string_view name;
    const int id;

    const Type *type = nullptr;
};

struct Class {
    Class(std::string_view Name, int ID, Scope *sc)
        : name(Name), id(ID), scope(sc) {}

    std::string_view name;
    const int id;
    Scope *scope;

    void DeclareField(std::string_view name, const Type *type);
    Field *FindField(std::string_view name) const;
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

    Var *DefineVar(std::string_view name);
    Var *FindVar(std::string_view name) const;
    int VarCount() const;

    Field *DefineFild(std::string_view name);
    Field *FindField(std::string_view name) const;
    int FieldCount() const;

    Func *DefineFunc(std::string_view name);
    Func *FindFunc(std::string_view name) const;

    Class *DefineClass(std::string_view name);
    Class *FindClass(std::string_view name) const;

    int VarSize() const;
    int FieldSize() const;

    void Print(int depth = 0) const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;
    const Func *func_ = nullptr;
    const Class *clss_ = nullptr;

    std::map<std::string_view,Var*> vars_;
    std::map<std::string_view,Func*> funcs_;
    std::map<std::string_view,Field*> flds_;
    std::map<std::string_view,Class*> clsses_;
};

#endif // _H
