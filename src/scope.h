#ifndef SCOPE_H
#define SCOPE_H

#include <vector>
#include <map>

class Scope;

struct Variable {
    Variable(const char *name_) : name(name_) {}
    const char *name;
};

struct Function {
    Function(const char *name_) : name(name_) {}
    const char *name;
    Scope *scope = nullptr;
};

class Scope {
public:
    Scope() : parent_(nullptr) {}
    Scope(Scope *parent) : parent_(parent) {}
    ~Scope() {}

    Scope *OpenChild();
    Scope *Close() const;
    Scope *GetParent() const;
    Scope *GetLastChild() const;

    void DefineVariable(const char *name);
    Variable *FindVariable(const char *name) const;
    int GetVariableCount() const;

    Function *DefineFunction(const char *name);
    Function *FindFunction(const char *name) const;
    int GetFunctionCount() const;

private:
    Scope *parent_ = nullptr;
    std::vector<Scope*> children_;

    std::map<const char*,Variable*> vars_;
    std::map<const char*,Function*> funcs_;
};

#endif // _H
