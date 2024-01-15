#include "scope.h"
#include "compiler.h"
#include "type.h"
#include <algorithm>
#include <iostream>

// Func
void DeclareParam(Func *f, const char *name, const Type *type)
{
    Var *var = DefineVar(f->scope, name, type);
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
void DeclareField(Class *c, const char *name, const Type *type)
{
    Field *f = c->scope->DefineFild(name);
    f->type = type;
    c->nflds_++;
}

Field *FindField(const Class *c, const char *name)
{
    return c->scope->FindField(name);
}

int FieldCount(const Class *c)
{
    return c->nflds_;
}

int Size(const Class *c)
{
    return c->scope->FieldSize();
}

// Scope
Scope *new_scope(Scope *parent, int level, int var_offset)
{
    Scope *sc = CALLOC(Scope);
    sc->parent_ = parent;
    sc->level_ = level;
    sc->var_offset_ = var_offset;

    sc->field_offset_ = 0;
    sc->class_offset_ = 0;
    sc->clss_ = NULL;

    sc->vars_ = NULL;
    sc->vars_tail = NULL;
    sc->funcs_ = NULL;
    sc->flds_ = NULL;
    sc->fld_tail = NULL;
    sc->clsses_ = NULL;
    sc->clsses_tail = NULL;

    return sc;
}

Scope *OpenChild(Scope *sc)
{
    const int next_id = IsGlobal(sc) ? 0 : sc->next_var_id();

    Scope *child = new_scope(sc, sc->level_ + 1, next_id);
    sc->children_.push_back(child);

    return child;
}

Scope *Close(const Scope *sc)
{
    return sc->parent_;
}

bool IsGlobal(const Scope *sc)
{
    // level 0: builtin scope
    // level 1: global (file) scope
    return sc->level_ == 1;
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

Var *DefineVar(Scope *sc, const char *name, const Type *type)
{
    if (FindVar(sc, name, false))
        return NULL;

    const int next_id = sc->next_var_id();
    Var *var = new_var(name, type, next_id, IsGlobal(sc));
    if (!sc->vars_)
        sc->vars_tail = sc->vars_ = var;
    else
        sc->vars_tail = sc->vars_tail->next = var;
    sc->var_offset_ += SizeOf(var->type);

    return var;
}

Var *FindVar(const Scope *sc, const char *name, bool find_in_parents)
{
    for (Var *v = sc->vars_; v; v = v->next) {
        if (!strcmp(v->name, name))
            return v;
    }

    if (!find_in_parents)
        return NULL;

    if (sc->parent_)
        return FindVar(sc->parent_, name, true);

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
    Scope *param_scope = OpenChild(this);

    const bool is_builtin = level_ == 0;
    Func *func = new_func(param_scope, is_builtin);

    func->next = funcs_;
    funcs_ = func;

    return func;
}

const Var *Scope::FindFunc(const char *name) const
{
    const Var *var = FindVar(this, name, true);
    if (var && IsFunc(var->type)) {
        return const_cast<Var *>(var);
    }

    if (parent_)
        return parent_->FindFunc(name);

    return NULL;
}

static Class *new_class(const char *name, int id, Scope *sc)
{
    Class *c = CALLOC(Class);
    c->name = name;
    c->id = id;
    c->scope = sc;
    c->nflds_ = 0;
    c->next = NULL;

    return c;
}

Class *Scope::DefineClass(const char *name)
{
    if (FindClass(name))
        return NULL;

    Scope *clss_scope = OpenChild(this);

    const int next_id = class_offset_;
    Class *clss = new_class(name, next_id, clss_scope);
    if (!clsses_)
        clsses_tail = clsses_ = clss;
    else
        clsses_tail = clsses_tail->next = clss;
    clss_scope->clss_ = clss;
    class_offset_++;
    return clss;
}

Class *Scope::FindClass(const char *name) const
{
    for (Class *c = clsses_; c; c = c->next) {
        if (!strcmp(c->name, name))
            return c;
    }

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
}

void PrintScope(const Scope *sc, int depth)
{
    const std::string header =
        std::string(depth * 2, ' ') +
        std::to_string(depth) + ". ";

    for (Var *v = sc->vars_; v; v = v->next) {

        if (IsFunc(v->type)) {
            printf("%s[fnc] %s @%d %s\n",
                    header.c_str(), v->name, v->id, TypeString(v->type));
            PrintScope(v->type->func->scope, depth + 1);
        }
        else {
            printf("%s[var] %s @%d %s\n",
                    header.c_str(), v->name, v->id, TypeString(v->type));
        }
    }

    for (const Field *fld = sc->flds_; fld; fld = fld->next) {

        std::cout << header <<
            "[fld] " << fld->name <<
            " @" << fld->id <<
            " " << fld->type << std::endl;
    }

    // when we're in global scope, no need to print child scopes as
    // they are printed by func vars. Otherwise go ahead and print them.
    // TODO may be better to detatch func scope from globals
    if (IsGlobal(sc))
        return;

    for (auto scope: sc->children_) {

        if (scope->clss_)
            std::cout << header <<
                "[clss] " << scope->clss_->name << std::endl;

        PrintScope(scope, depth + 1);
    }
}
