#ifndef SCOPE_H
#define SCOPE_H

#include <map>

class Scope;

struct Variable {
    Variable(int name_id) : name(name_id) {}
    int name;
};

struct Function {
    Function(int name_id) : name(name_id) {}
    int name;
    Scope *scope = nullptr;
};

class Scope {
public:
    Scope() {}
    ~Scope() {}

    void DefineVariable(int name);
    Variable *FindVariable(int name) const;
    int GetVariableCount() const;

    Function *DefineFunction(int name);
    Function *FindFunction(int name) const;
    int GetFunctionCount() const;

private:
    std::map<int,Variable*> vars_;
    std::map<int,Function*> funcs_;
};

#endif // _H
