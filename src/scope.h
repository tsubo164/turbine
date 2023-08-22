#ifndef SCOPE_H
#define SCOPE_H

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
    Scope() {}
    ~Scope() {}

    void DefineVariable(const char *name);
    Variable *FindVariable(const char *name) const;
    int GetVariableCount() const;

    Function *DefineFunction(const char *name);
    Function *FindFunction(const char *name) const;
    int GetFunctionCount() const;

private:
    std::map<const char*,Variable*> vars_;
    std::map<const char*,Function*> funcs_;
};

#endif // _H
