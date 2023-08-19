#ifndef SCOPE_H
#define SCOPE_H

#include <map>

struct Variable {
    Variable(int name_id) : name(name_id) {}
    int name;
};

class Scope {
public:
    Scope() {}
    ~Scope() {}

    void DefineVariable(int name);
    Variable *FindVarialbe(int name) const;
    int GetVariableCount() const;

private:
    std::map<int,Variable*> vars_;
};

#endif // _H
