#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>
#include "string_table.h"

class Scope;

struct Variable {
    Variable(SharedStr name_, int id_) : name(name_), id(id_) {}
    SharedStr name;
    int id;
};

struct Function {
    Function(const char *name_) : name(name_) {}
    const char *name;
    Scope *scope = nullptr;
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

    Variable *DefineVariable(const char *name);
    Variable *FindVariable(const char *name) const;
    int GetVariableCount() const;

    Function *DefineFunction(const char *name);
    Function *FindFunction(const char *name) const;
    int GetFunctionCount() const;

    void Print(int depth = 0) const;
private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;

    std::map<const char*,Variable*> vars_;
    std::map<const char*,Function*> funcs_;
};

#endif // _H
