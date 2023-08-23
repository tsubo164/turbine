#include "scope.h"

Scope *Scope::OpenChild()
{
    Scope *child = new Scope(this);
    children_.push_back(child);
    return child;
}

Scope *Scope::Close() const
{
    return GetParent();
}

Scope *Scope::GetParent() const
{
    return parent_;
}

void Scope::DefineVariable(const char *name)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return;
    }

    vars_.insert({name, new Variable(name)});
}

Variable *Scope::FindVariable(const char *name) const
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

Function *Scope::DefineFunction(const char *name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return nullptr;
    }

    Function *func = new Function(name);
    funcs_.insert({name, func});
    return func;
}

Function *Scope::FindFunction(const char *name) const
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return it->second;
    }

    return nullptr;
}

int Scope::GetFunctionCount() const
{
    return funcs_.size();
}
