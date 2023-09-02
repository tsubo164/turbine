#include "scope.h"
#include <iostream>

FuncObj::FuncObj(SharedStr name_, int id_, Scope *parent_)
    : name(name_), id(id_), scope(new Scope(parent_))
{
}

void FuncObj::DeclParam(SharedStr name)
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

    if (GetParent())
        return GetParent()->FindVariable(name);

    return nullptr;
}

int Scope::GetVariableCount() const
{
    return vars_.size();
}

FuncObj *Scope::DefineFunction(const char *name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return nullptr;
    }

    const int next_id = funcs_.size();
    FuncObj *func = new FuncObj(name, next_id, this);
    funcs_.insert({name, func});
    return func;
}

FuncObj *Scope::FindFunction(const char *name) const
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

Variable *Scope::DeclareParameter(SharedStr name)
{
    const auto found = params_.find(name);
    if (found != params_.end()) {
        return nullptr;
    }

    const int next_id = params_.size();
    Variable *var = new Variable(name, next_id);
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
        const FuncObj *func = it.second;

        std::cout << header <<
            "[func] " << func->name << std::endl;

        func->scope->Print(depth + 1);
    }

    for (auto it: vars_) {
        const Variable *var = it.second;

        std::cout << header <<
            "[var] " << var->name <<
            " @" << var->id << std::endl;
    }
}
