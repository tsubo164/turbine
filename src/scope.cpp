#include "scope.h"
#include <iostream>

template<typename T>
T *define_symbol(std::map<SharedStr,T*> &map, SharedStr name)
{
    const auto found = map.find(name);
    if (found != map.end()) {
        return nullptr;
    }
    const int next_id = map.size();
    T *ent = new T(name, next_id);
    map.insert({name, ent});
    return ent;
}

template<typename T>
T *find_symbol(const std::map<SharedStr,T*> &map, SharedStr name)
{
    const auto it = map.find(name);
    if (it != map.end()) {
        return it->second;
    }
    return nullptr;
}

template<typename T>
int symbol_count(const std::map<SharedStr,T*> &map)
{
    return map.size();
}

Argument *Function::DefineArgument(SharedStr name)
{
    return define_symbol(args_, name);
}

Argument *Function::FindArgument(SharedStr name) const
{
    return find_symbol(args_, name);
}

int Function::GetArgumentCount() const
{
    return symbol_count(args_);
}

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

    if (!params_.empty()) {
        // move params into function body scope
        std::swap(params_, child->vars_);
    }

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

Function *Scope::DefineFunction(const char *name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return nullptr;
    }

    const int next_id = funcs_.size();
    Function *func = new Function(name, next_id);
    funcs_.insert({name, func});
    return func;
}

Function *Scope::FindFunction(const char *name) const
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

void Function::Print(int depth) const
{
    const std::string header = std::string(depth * 2, ' ') + std::to_string(depth) + ". ";

    // args
    for (auto it: args_) {
        const Argument *arg = it.second;
        std::cout << header << "[arg] " << arg->name << " @" << arg->id << std::endl;
    }
}

void Scope::Print(int depth) const
{
    const std::string header = std::string(depth * 2, ' ') + std::to_string(depth) + ". ";

    for (auto it: funcs_) {
        const Function *func = it.second;
        std::cout << header << "[func] " << func->name << std::endl;
        func->Print(depth + 1);
    }

    for (auto it: vars_) {
        const Variable *var = it.second;
        std::cout << header << "[var] " << var->name << " @" << var->id << std::endl;
    }

    for (auto child: children_) {
        child->Print(depth + 1);
    }
}
