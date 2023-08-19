#include "scope.h"

void Scope::DefineVariable(int name)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return;
    }

    vars_.insert({name, new Variable(name)});
}

Variable *Scope::FindVarialbe(int name) const
{
    const auto it = vars_.find(name);
    if (it != vars_.end()) {
        return it->second;
    }

    return nullptr;
}

int Scope::GetVariableCount() const
{
    return vars_.size();
}
