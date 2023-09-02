#include "scope.h"
#include <iostream>

Func::Func(SharedStr name_, int id_, Scope *parent_)
    : name(name_), id(id_), scope(new Scope(parent_))
{
}

void Func::DeclParam(SharedStr name)
{
    scope->DefineVariable(name);
    argc++;
}

Scope::Scope()
    : parent_(nullptr)
{
}

Scope::Scope(Scope *parent)
    : parent_(parent)
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

Var *Scope::DefineVariable(const char *name)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return nullptr;
    }

    const int next_id = vars_.size();
    Var *var = new Var(name, next_id);
    vars_.insert({name, var});
    return var;
}

Var *Scope::FindVariable(const char *name) const
{
    const auto it = vars_.find(name);
    if (it != vars_.end()) {
        return it->second;
    }

    if (GetParent())
        return GetParent()->FindVariable(name);

    return nullptr;
}

int Scope::GetVariableCount() const
{
    return vars_.size();
}

Func *Scope::DefineFunction(const char *name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return nullptr;
    }

    const int next_id = funcs_.size();
    Func *func = new Func(name, next_id, this);
    funcs_.insert({name, func});
    return func;
}

Func *Scope::FindFunction(const char *name) const
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return it->second;
    }

    if (GetParent())
        return GetParent()->FindFunction(name);

    return nullptr;
}

int Scope::GetFunctionCount() const
{
    return funcs_.size();
}

Var *Scope::DeclareParameter(SharedStr name)
{
    const auto found = params_.find(name);
    if (found != params_.end()) {
        return nullptr;
    }

    const int next_id = params_.size();
    Var *var = new Var(name, next_id);
    params_.insert({name, var});
    return var;
}

int Scope::GetParameterCount() const
{
    return params_.size();
}

void Scope::Print(int depth) const
{
    const std::string header =
        std::string(depth * 2, ' ') +
        std::to_string(depth) + ". ";

    for (auto it: funcs_) {
        const Func *func = it.second;

        std::cout << header <<
            "[func] " << func->name << std::endl;

        func->scope->Print(depth + 1);
    }

    for (auto it: vars_) {
        const Var *var = it.second;

        std::cout << header <<
            "[var] " << var->name <<
            " @" << var->id << std::endl;
    }
}
