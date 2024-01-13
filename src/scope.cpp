#include "scope.h"
#include "type.h"
#include <algorithm>
#include <iostream>

// Func
void Func::DeclareParam(const char *name, const Type *type)
{
    const Var *var = scope->DefineVar(name, type);
    params_.push_back(var);

    if (!strcmp(name, "..."))
        ellipsis_index_ = ParamCount() - 1;

    if (name[0] == '$')
        has_special_var_ = true;
}

const Var *Func::GetParam(int index) const
{
    int i = 0;

    if (IsVariadic() && index >= ParamCount())
        i = ParamCount() - 1;
    else
        i = index;

    if (i < 0 || i >= ParamCount())
        return nullptr;
    return params_[i];
}

int Func::RequiredParamCount() const
{
    if (IsVariadic())
        return ParamCount() - 1;
    else
        return ParamCount();
}

int Func::ParamCount() const
{
    return params_.size();
}

// Class
void Class::DeclareField(const char *name, const Type *type)
{
    Field *f = scope->DefineFild(name);
    f->type = type;
    nflds_++;
}

Field *Class::FindField(const char *name) const
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
    : parent_(nullptr), level_(0), var_offset_(0)
{
}

Scope::Scope(Scope *parent, int level, int var_offset)
    : parent_(parent), level_(level), var_offset_(var_offset)
{
}

Scope::~Scope()
{
    for (auto child: children_)
        delete child;
}

Scope *Scope::OpenChild()
{
    const int next_id = IsGlobal() ? 0 : next_var_id();

    Scope *child = new Scope(this, level_ + 1, next_id);
    children_.push_back(child);

    return child;
}

Scope *Scope::Close() const
{
    return parent_;
}

bool Scope::IsGlobal() const
{
    // level 0: builtin scope
    // level 1: global (file) scope
    return level_ == 1;
}

Var *Scope::DefineVar(const char *name, const Type *type)
{
    const auto found = vars_.find(name);
    if (found != vars_.end()) {
        return nullptr;
    }

    const int next_id = next_var_id();
    Var *var = new Var(name, type, next_id, IsGlobal());
    vars_.insert({name, var});
    var_offset_ += var->type->Size();

    return var;
}

Var *Scope::FindVar(const char *name, bool find_in_parents) const
{
    const auto it = vars_.find(name);
    if (it != vars_.end()) {
        return it->second;
    }

    if (!find_in_parents)
        return nullptr;

    if (parent_)
        return parent_->FindVar(name);

    return nullptr;
}

Field *Scope::DefineFild(const char *name)
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

Field *Scope::FindField(const char *name) const
{
    const auto it = flds_.find(name);
    if (it != flds_.end()) {
        return it->second;
    }

    if (parent_)
        return parent_->FindField(name);

    return nullptr;
}

int Scope::FieldCount() const
{
    return flds_.size();
}

Func *Scope::DeclareFunc()
{
    Scope *param_scope = OpenChild();

    const bool is_builtin = level_ == 0;
    Func *func = new Func(param_scope, is_builtin);

    return func;
}

const Var *Scope::FindFunc(const char *name) const
{
    const Var *var = FindVar(name);
    if (var && var->type->IsFunc()) {
        return const_cast<Var *>(var);
    }

    if (parent_)
        return parent_->FindFunc(name);

    return nullptr;
}

Class *Scope::DefineClass(const char *name)
{
    const auto it = clsses_.find(name);
    if (it != clsses_.end())
        return nullptr;

    Scope *clss_scope = OpenChild();

    const int next_id = clsses_.size();
    Class *clss = new Class(name, next_id, clss_scope);
    clsses_.insert({name, clss});

    clss_scope->clss_ = clss;
    return clss;
}

Class *Scope::FindClass(const char *name) const
{
    const auto it = clsses_.find(name);
    if (it != clsses_.end())
        return it->second;

    if (parent_)
        return parent_->FindClass(name);

    return nullptr;
}

int Scope::next_var_id() const
{
    return var_offset_;
}

int Scope::VarSize() const
{
    return next_var_id();
}

int Scope::TotalVarSize() const
{
    return max_var_id() + 1;
}

int Scope::max_var_id() const
{
    int max = next_var_id() - 1;

    for (auto child: children_)
        max = std::max(max, child->max_var_id());

    return max;
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

        const char *tag = nullptr;
        if (var->type->IsFunc())
            tag = "[fnc] ";
        else
            tag = "[var] ";
        std::cout << header <<
            tag << var->name <<
            " @" << var->id <<
            " " << var->type << std::endl;
    }

    for (auto it: flds_) {
        const Field *fld = it.second;

        std::cout << header <<
            "[fld] " << fld->name <<
            " @" << fld->id <<
            " " << fld->type << std::endl;
    }

    for (auto scope: children_) {

        if (scope->clss_)
            std::cout << header <<
                "[clss] " << scope->clss_->name << std::endl;

        scope->Print(depth + 1);
    }
}
