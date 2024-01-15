#include "scope.h"
#include "compiler.h"
#include "type.h"
#include <algorithm>
#include <iostream>

// Func
void DeclareParam(Func *f, const char *name, const Type *type)
{
    Var *var = f->scope->DefineVar(name, type);
    if (f->param_count == 0)
        f->params = var;
    f->param_count++;

    if (!strcmp(name, "..."))
        f->ellipsis_index = ParamCount(f) - 1;

    if (name[0] == '$')
        f->has_special_var = true;
}

const Var *GetParam(const Func *f, int index)
{
    int idx = 0;

    if (IsVariadic(f) && index >= ParamCount(f))
        idx = ParamCount(f) - 1;
    else
        idx = index;

    if (idx < 0 || idx >= ParamCount(f))
        return NULL;

    Var *param = f->params;
    for (int i = 0; i < idx; i++)
        param = param->next;
    return param;
}

int RequiredParamCount(const Func *f)
{
    if (IsVariadic(f))
        return ParamCount(f) - 1;
    else
        return ParamCount(f);
}

int ParamCount(const Func *f)
{
    return f->param_count;
}

bool HasSpecialVar(const Func *f)
{
    return f->has_special_var;
}

bool IsVariadic(const Func *f)
{
    return f->ellipsis_index >= 0;
}

bool IsBuiltin(const Func *f)
{
    return f->is_builtin;
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
    : parent_(NULL), level_(0), var_offset_(0)
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

static Var *new_var(const char *Name, const Type *t, int ID, bool global)
{
    Var *v = CALLOC(Var);
    v->name = Name;
    v->type = t;
    v->id = ID;
    v->is_global = global;
    v->next = NULL;
    return v;
}

Var *Scope::DefineVar(const char *name, const Type *type)
{
    if (FindVar(name, false))
        return NULL;

    const int next_id = next_var_id();
    Var *var = new_var(name, type, next_id, IsGlobal());
    if (!vars_)
        vars_tail = vars_ = var;
    else
        vars_tail = vars_tail->next = var;
    var_offset_ += SizeOf(var->type);

    return var;
}

Var *Scope::FindVar(const char *name, bool find_in_parents) const
{
    for (Var *v = vars_; v; v = v->next) {
        if (!strcmp(v->name, name))
            return v;
    }

    if (!find_in_parents)
        return NULL;

    if (parent_)
        return parent_->FindVar(name);

    return NULL;
}

static Field *new_field(const char *Name, int ID)
{
    Field *f = CALLOC(Field);
    f->name = Name;
    // TODO add type
    f->type = NewIntType();
    f->id = ID;
    f->next = NULL;
    return f;
}

Field *Scope::DefineFild(const char *name)
{
    if (FindField(name))
        return NULL;

    const int next_id = field_offset_;
    Field *fld = new_field(name, next_id);
    if (!flds_)
        fld_tail = flds_ = fld;
    else
        fld_tail = fld_tail->next = fld;
    field_offset_ += SizeOf(fld->type);

    return fld;
}

Field *Scope::FindField(const char *name) const
{
    for (Field *f = flds_; f; f = f->next) {
        if (!strcmp(f->name, name))
            return f;
    }

    if (parent_)
        return parent_->FindField(name);

    return NULL;
}

Func *new_func(Scope *sc, bool builtin)
{
    Func *f = CALLOC(Func);
    f->scope = sc;
    f->return_type = NULL;
    f->params = NULL;
    f->param_count = 0;
    f->is_builtin = builtin;
    f->has_special_var = false;
    f->ellipsis_index = -1;
    f->next = NULL;

    return f;
}

Func *Scope::DeclareFunc()
{
    Scope *param_scope = OpenChild();

    const bool is_builtin = level_ == 0;
    Func *func = new_func(param_scope, is_builtin);

    func->next = funcs_;
    funcs_ = func;

    return func;
}

const Var *Scope::FindFunc(const char *name) const
{
    const Var *var = FindVar(name);
    if (var && IsFunc(var->type)) {
        return const_cast<Var *>(var);
    }

    if (parent_)
        return parent_->FindFunc(name);

    return NULL;
}

Class *Scope::DefineClass(const char *name)
{
    const auto it = clsses_.find(name);
    if (it != clsses_.end())
        return NULL;

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

    return NULL;
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
    return field_offset_;
    /*
    int size = 0;

    for (auto it: flds_) {
        const Field *fld = it.second;
        size += SizeOf(fld->type);
    }

    return size;
    */
}

void Scope::Print(int depth) const
{
    const std::string header =
        std::string(depth * 2, ' ') +
        std::to_string(depth) + ". ";

    for (Var *v = vars_; v; v = v->next) {

        if (IsFunc(v->type)) {
            printf("%s[fnc] %s @%d %s\n",
                    header.c_str(), v->name, v->id, TypeString(v->type));
            v->type->func->scope->Print(depth + 1);
        }
        else {
            printf("%s[var] %s @%d %s\n",
                    header.c_str(), v->name, v->id, TypeString(v->type));
        }
    }

    //for (auto it: flds_) {
    //    const Field *fld = it.second;
    for (const Field *fld = flds_; fld; fld = fld->next) {

        std::cout << header <<
            "[fld] " << fld->name <<
            " @" << fld->id <<
            " " << fld->type << std::endl;
    }

    // when we're in global scope, no need to print child scopes as
    // they are printed by func vars. Otherwise go ahead and print them.
    // TODO may be better to detatch func scope from globals
    if (IsGlobal())
        return;

    for (auto scope: children_) {

        if (scope->clss_)
            std::cout << header <<
                "[clss] " << scope->clss_->name << std::endl;

        scope->Print(depth + 1);
    }
}
