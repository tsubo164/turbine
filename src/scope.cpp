#include "scope.h"
#include "type.h"
#include <algorithm>
#include <iostream>

// Func
void Func::DeclareParam(std::string_view name, const Type *type)
{
    scope->DefineVar(name, type);
    nparams_++;
}

int Func::ParamCount() const
{
    return nparams_;
}

int Func::VarCount() const
{
    const int var_count = scope->MaxVarID() + 1;
    return var_count - ParamCount();
}

// Class
void Class::DeclareField(std::string_view name, const Type *type)
{
    Field *f = scope->DefineFild(name);
    f->type = type;
    nflds_++;
}

Field *Class::FindField(std::string_view name) const
{
    return scope->FindField(name);
}

int Class::FieldCount() const
{
    return nflds_;
}

int Class::Size() const
{
    return scope->FieldSize();
}

// Scope
Scope::Scope()
    : parent_(nullptr), var_id_offset_(0)
{
}

Scope::Scope(Scope *parent, int var_id_offset)
    : parent_(parent), var_id_offset_(var_id_offset)
{
}

Scope::~Scope()
{
    for (auto child: children_)
        delete child;
}

Scope *Scope::OpenChild()
{
    const int next_var_id = IsGlobal() ? 0 : NextVarID();

    Scope *child = new Scope(this, next_var_id);
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

Var *Scope::DefineVar(std::string_view name, const Type *type)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return nullptr;
    }

    const int next_id = NextVarID();
    Var *var = new Var(name, type, next_id, IsGlobal());
    vars_.insert({name, var});
    return var;
}

Var *Scope::FindVar(std::string_view name, bool find_in_parents) const
{
    const auto it = vars_.find(name);
    if (it != vars_.end()) {
        return it->second;
    }

    if (!find_in_parents)
        return nullptr;

    if (HasParent())
        return Parent()->FindVar(name);

    return nullptr;
}

int Scope::VarCount() const
{
    return vars_.size();
}

int Scope::MaxVarID() const
{
    int max = NextVarID() - 1;

    for (auto child: children_)
        max = std::max(max, child->MaxVarID());

    return max;
}

Field *Scope::DefineFild(std::string_view name)
{
    const auto found = flds_.find(name);
    if (found != flds_.end()) {
        return nullptr;
    }

    const int next_id = flds_.size();
    Field *fld = new Field(name, next_id);
    flds_.insert({name, fld});
    return fld;
}

Field *Scope::FindField(std::string_view name) const
{
    const auto it = flds_.find(name);
    if (it != flds_.end()) {
        return it->second;
    }

    if (HasParent())
        return Parent()->FindField(name);

    return nullptr;
}

int Scope::FieldCount() const
{
    return flds_.size();
}

Func *Scope::DefineFunc(std::string_view name)
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

Func *Scope::FindFunc(std::string_view name) const
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end()) {
        return it->second;
    }

    if (HasParent())
        return Parent()->FindFunc(name);

    return nullptr;
}

Class *Scope::DefineClass(std::string_view name)
{
    const auto it = funcs_.find(name);
    if (it != funcs_.end())
        return nullptr;

    Scope *clss_scope = OpenChild();

    const int next_id = clsses_.size();
    Class *clss = new Class(name, next_id, clss_scope);
    clsses_.insert({name, clss});

    clss_scope->clss_ = clss;
    return clss;
}

Class *Scope::FindClass(std::string_view name) const
{
    const auto it = clsses_.find(name);
    if (it != clsses_.end())
        return it->second;

    if (HasParent())
        return Parent()->FindClass(name);

    return nullptr;
}

int Scope::NextVarID() const
{
    return VarCount() + var_id_offset_;
}

int Scope::VarSize() const
{
    int size = 0;

    for (auto it: vars_) {
        const Var *var = it.second;
        size += var->type->Size();
    }

    return size;
}

int Scope::FieldSize() const
{
    int size = 0;

    for (auto it: flds_) {
        const Field *fld = it.second;
        size += fld->type->Size();
    }

    return size;
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
            " @" << var->id <<
            " " << var->type->kind << std::endl;
    }

    for (auto it: flds_) {
        const Field *fld = it.second;

        std::cout << header <<
            "[fld] " << fld->name <<
            " @" << fld->id <<
            " " << fld->type->kind << std::endl;
    }

    for (auto scope: children_) {

        if (scope->func_)
            std::cout << header <<
                "[func] " << scope->func_->name << std::endl;
        else if (scope->clss_)
            std::cout << header <<
                "[clss] " << scope->clss_->name << std::endl;

        scope->Print(depth + 1);
    }
}

// TODO should return const Func *
Func *FindBuiltinFunc(std::string_view name)
{
    static std::vector<Func> builtins = {
        {"print", 0, nullptr, true, true},
        {"exit",  1, nullptr, true, false},
    };

    for (auto &func: builtins) {
        if (name == func.name)
            return &func;
    }

    return nullptr;
}
