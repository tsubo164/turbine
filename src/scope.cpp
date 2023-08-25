#include "scope.h"
#include <iostream>

Scope::Scope() : parent_(nullptr)
{
}

Scope::Scope(Scope *parent) : parent_(parent)
{
}

Scope::~Scope()
{
    for (auto child: children_)
        delete child;
}

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

Scope *Scope::GetLastChild() const
{
    const int last = children_.size() - 1;
    return children_[last];
}

Variable *Scope::DefineVariable(const char *name)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return nullptr;
    }

    const int next_id = vars_.size();
    Variable *var = new Variable(name, next_id);
    vars_.insert({name, var});
    return var;
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

void Scope::Print(int depth) const
{
    const std::string header = std::string(depth * 2, ' ') + std::to_string(depth) + ". ";

    for (auto it: funcs_) {
        const Function *func = it.second;
        std::cout << header << "[func] " << func->name << std::endl;
        func->scope->Print(depth + 1);
    }

    for (auto it: vars_) {
        const Variable *var = it.second;
        std::cout << header << "[var] " << var->name << " @" << var->id << std::endl;
    }
}
