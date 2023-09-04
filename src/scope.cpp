#include "scope.h"
#include <iostream>

void Func::DeclareParam(SharedStr name)
{
    scope->DefineVar(name);
    nparams_++;
}

int Func::ParamCount() const
{
    return nparams_;
}

int Func::VarCount() const
{
    return scope->VarCount() - ParamCount();
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
    return Parent();
}

Scope *Scope::Parent() const
{
    return parent_;
}

bool Scope::HasParent() const
{
    return Parent();
}

bool Scope::IsGlobal() const
{
    return !Parent();
}

Var *Scope::DefineVar(const char *name)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return nullptr;
    }

    const int next_id = vars_.size();
    Var *var = new Var(name, next_id, IsGlobal());
    vars_.insert({name, var});
    return var;
}

Var *Scope::FindVar(const char *name) const
{
    const auto it = vars_.find(name);
    if (it != vars_.end()) {
        return it->second;
    }

    if (HasParent())
        return Parent()->FindVar(name);

    return nullptr;
}

int Scope::VarCount() const
{
    return vars_.size();
}

Func *Scope::DefineFunc(const char *name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end())
        return nullptr;

    Scope *func_scope = OpenChild();

    const int next_id = funcs_.size();
    Func *func = new Func(name, next_id, func_scope);
    funcs_.insert({name, func});

    func_scope->func_ = func;
    return func;
}

Func *Scope::FindFunc(const char *name) const
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return it->second;
    }

    if (HasParent())
        return Parent()->FindFunc(name);

    return nullptr;
}

void Scope::Print(int depth) const
{
    const std::string header =
        std::string(depth * 2, ' ') +
        std::to_string(depth) + ". ";

    for (auto it: vars_) {
        const Var *var = it.second;

        std::cout << header <<
            "[var] " << var->name <<
            " @" << var->id << std::endl;
    }

    for (auto scope: children_) {

        if (scope->func_)
            std::cout << header <<
                "[func] " << scope->func_->name << std::endl;

        scope->Print(depth + 1);
    }
}
