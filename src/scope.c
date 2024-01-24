#include "scope.h"
#include "type.h"
#include "mem.h"
#include <string.h>
#include <stdio.h>

// Func
void DeclareParam(Func *f, const char *name, const Type *type)
{
    struct Symbol *sym = DefineVar(f->scope, name, type);
    Var *var = sym->var;

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
    Field *f = DefineFild(c->scope, name);
    f->type = type;
    c->nflds_++;
}

Field *FindField(const Class *c, const char *name)
{
    return FindClassField(c->scope, name);
}

int FieldCount(const Class *c)
{
    return c->nflds_;
}

int ClassSize(const Class *c)
{
    return FieldSize(c->scope);
}

// Scope
static Scope *new_scope(Scope *parent, int var_offset)
{
    Scope *sc = CALLOC(Scope);
    sc->parent_ = parent;
    sc->children_ = NULL;
    sc->child_tail = NULL;
    sc->next = NULL;

    sc->level_ = parent->level_ + 1;
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

static int next_var_id(const Scope *sc);

Scope *OpenChild(Scope *sc)
{
    const int next_id = IsGlobalScope(sc) ? 0 : next_var_id(sc);

    Scope *child = new_scope(sc, next_id);
    if (!sc->children_)
        sc->child_tail = sc->children_ = child;
    else
        sc->child_tail = sc->child_tail->next = child;

    return child;
}

Scope *Close(const Scope *sc)
{
    return sc->parent_;
}

bool IsGlobalScope(const Scope *sc)
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

// TODO remove forward decls
static Symbol *new_symbol(int kind, const char *name, const Type *t);
Symbol *FindSymbolThisScope(Scope *sc, const char *name);

struct Symbol *DefineVar(Scope *sc, const char *name, const Type *type)
{
    if (FindSymbolThisScope(sc, name))
        return NULL;

    const int next_id = next_var_id(sc);
    Var *var = new_var(name, type, next_id, IsGlobalScope(sc));

    Symbol *sym = new_symbol(SYM_VAR, name, type);
    sym->var = var;

    //-------------------
    if (!sc->vars_)
        sc->vars_tail = sc->vars_ = var;
    else
        sc->vars_tail = sc->vars_tail->next = var;
    //-------------------
    sc->var_offset_ += SizeOf(var->type);

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return sym;
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
    f->type = NewTypeInt();
    f->id = ID;
    f->next = NULL;
    return f;
}

Field *DefineFild(Scope *sc, const char *name)
{
    if (FindClassField(sc, name))
        return NULL;

    const int next_id = sc->field_offset_;
    Field *fld = new_field(name, next_id);
    if (!sc->flds_)
        sc->fld_tail = sc->flds_ = fld;
    else
        sc->fld_tail = sc->fld_tail->next = fld;
    sc->field_offset_ += SizeOf(fld->type);

    return fld;
}

Field *FindClassField(const Scope *sc, const char *name)
{
    for (Field *f = sc->flds_; f; f = f->next) {
        if (!strcmp(f->name, name))
            return f;
    }

    if (sc->parent_)
        return FindClassField(sc->parent_, name);

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

Func *DeclareFunc(Scope *sc)
{
    Scope *param_scope = OpenChild(sc);

    const bool is_builtin = sc->level_ == 0;
    Func *func = new_func(param_scope, is_builtin);

    func->next = sc->funcs_;
    sc->funcs_ = func;

    return func;
}

const Var *FindFunc(const Scope *sc, const char *name)
{
    const Var *var = FindVar(sc, name, true);
    if (var && IsFunc(var->type)) {
        return (Var *)(var);
    }

    if (sc->parent_)
        return FindFunc(sc->parent_, name);

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

Class *DefineClass(Scope *sc, const char *name)
{
    if (FindClass(sc, name))
        return NULL;

    Scope *clss_scope = OpenChild(sc);

    const int next_id = sc->class_offset_;
    Class *clss = new_class(name, next_id, clss_scope);
    if (!sc->clsses_)
        sc->clsses_tail = sc->clsses_ = clss;
    else
        sc->clsses_tail = sc->clsses_tail->next = clss;
    clss_scope->clss_ = clss;
    sc->class_offset_++;
    return clss;
}

Class *FindClass(const Scope *sc, const char *name)
{
    for (Class *c = sc->clsses_; c; c = c->next) {
        if (!strcmp(c->name, name))
            return c;
    }

    if (sc->parent_)
        return FindClass(sc->parent_, name);

    return NULL;
}

static Symbol *new_symbol(int kind, const char *name, const Type *t)
{
    Symbol *sym = CALLOC(Symbol);
    sym->kind = kind;
    sym->name = name;
    sym->type = t;
    return sym;
}

Table *DefineTable(Scope *sc, const char *name)
{
    Table *tab = CALLOC(Table);
    tab->name = name;
    tab->scope = new_scope(sc, 0);

    Symbol *sym = new_symbol(SYM_TABLE, name, NewTypeTable(tab));
    sym->table = tab;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return tab;
}

Symbol *FindSymbolThisScope(Scope *sc, const char *name)
{
    Symbol *sym = HashMapLookup(&sc->symbols, name);
    if (sym)
        return sym;

    return NULL;
}

Symbol *FindSymbol(Scope *sc, const char *name)
{
    Symbol *sym = HashMapLookup(&sc->symbols, name);
    if (sym)
        return sym;

    if (sc->parent_)
        return FindSymbol(sc->parent_, name);

    return NULL;
}

static int next_var_id(const Scope *sc)
{
    return sc->var_offset_;
}

static int max_var_id(const Scope *sc)
{
    int max = next_var_id(sc) - 1;

    for (Scope *child = sc->children_; child; child = child->next) {
        int child_max = max_var_id(child);
        max = max < child_max ? child_max : max;
    }

    return max;
}

int VarSize(const Scope *sc)
{
    return next_var_id(sc);
}

int TotalVarSize(const Scope *sc)
{
    return max_var_id(sc) + 1;
}

int FieldSize(const Scope *sc)
{
    return sc->field_offset_;
}

static void print_header(int depth)
{
    for (int i = 0; i < depth; i++)
        printf("  ");
    printf("%d. ", depth);
}

void PrintScope(const Scope *sc, int depth)
{
    for (Var *v = sc->vars_; v; v = v->next) {

        if (IsFunc(v->type)) {
            print_header(depth);
            printf("[fnc] %s @%d %s\n",
                    v->name, v->id, TypeString(v->type));
            PrintScope(v->type->func->scope, depth + 1);
        }
        else {
            print_header(depth);
            printf("[var] %s @%d %s\n",
                    v->name, v->id, TypeString(v->type));
        }
    }

    for (const Field *fld = sc->flds_; fld; fld = fld->next) {

        print_header(depth);
        printf("[fld] %s @%d %s\n",
                fld->name, fld->id, TypeString(fld->type));
    }

    for (int i = 0; i < sc->symbols.cap; i++) {
        const struct MapEntry *e = &sc->symbols.buckets[i];
        if (!e->key)
            continue;
        const struct Symbol *sym = e->val;

        if (sym->kind == SYM_TABLE) {
            const struct Table *t = sym->table;
            print_header(depth);
            printf("[tbl] %s\n", t->name);
            for (int i = 0; i < t->rows.cap; i++) {
                MapEntry *e = &t->rows.buckets[i];
                if (!e->key)
                    continue;
                Row *r = e->val;
                print_header(depth + 1);
                printf("[row] %s => %lld\n", r->name, r->ival);
            }
        }
    }

    // when we're in global scope, no need to print child scopes as
    // they are printed by func vars. Otherwise go ahead and print them.
    // TODO may be better to detatch func scope from globals
    if (IsGlobalScope(sc))
        return;

    for (Scope *scope = sc->children_; scope; scope = scope->next) {

        if (scope->clss_) {
            print_header(depth);
            printf("[clss] %s\n", scope->clss_->name);
        }

        PrintScope(scope, depth + 1);
    }
}
