#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

class Scope;

struct Var {
    Var(SharedStr name_, int id_)
        : name(name_), id(id_) {}
    SharedStr name;
    int id;
};

struct FuncObj {
    FuncObj(SharedStr name_, int id_, Scope *parent_);
    SharedStr name;
    int id;

    std::unique_ptr<Scope> scope;
    int argc = 0;

    void DeclParam(SharedStr name);
};

class Scope {
public:
    Scope();
    Scope(Scope *parent);
    ~Scope();

    Scope *OpenChild();
    Scope *Close() const;
    Scope *GetParent() const;
    Scope *GetLastChild() const;

    Var *DefineVariable(const char *name);
    Var *FindVariable(const char *name) const;
    int GetVariableCount() const;

    FuncObj *DefineFunction(const char *name);
    FuncObj *FindFunction(const char *name) const;
    int GetFunctionCount() const;

    Var *DeclareParameter(SharedStr name);
    int GetParameterCount() const;

    void Print(int depth = 0) const;
private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;

    std::map<const char*,Var*> vars_;
    std::map<const char*,FuncObj*> funcs_;
    std::map<const char*,Var*> params_;
};

#endif // _H
