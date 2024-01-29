#include "scope.h"
#include "prog.h"
#include "type.h"
#include "mem.h"
#include <string.h>

// Scope
static struct Scope *new_scope(struct Scope *parent, int var_offset)
{
    struct Scope *sc = CALLOC(struct Scope);
    sc->parent = parent;
    sc->var_offset_ = var_offset;

    return sc;
}

static int next_var_id(const struct Scope *sc);

struct Scope *NewScope(struct Scope *parent, int var_offset)
{
    return new_scope(parent, var_offset);
}

struct Scope *OpenChild(struct Scope *sc)
{
    const int next_id = sc->var_offset_;

    struct Scope *child = new_scope(sc, next_id);
    if (!sc->children_)
        sc->child_tail = sc->children_ = child;
    else
        sc->child_tail = sc->child_tail->next = child;

    return child;
}

static struct Var *new_var(const char *Name, const struct Type *t, int offset, bool global)
{
    struct Var *v = CALLOC(struct Var);
    v->name = Name;
    v->type = t;
    v->offset = offset;
    v->is_global = global;
    return v;
}

// TODO remove forward decls
static Symbol *new_symbol(int kind, const char *name, const Type *t);
Symbol *FindSymbolThisScope(struct Scope *sc, const char *name);

struct Symbol *DefineVar(struct Scope *sc, const char *name, const Type *type, bool isglobal)
{
    if (FindSymbolThisScope(sc, name))
        return NULL;

    const int next_id = next_var_id(sc);
    struct Var *var = new_var(name, type, next_id, isglobal);

    Symbol *sym = new_symbol(SYM_VAR, name, type);
    sym->var = var;

    sc->var_offset_ += SizeOf(var->type);

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return sym;
}

static struct Struct *new_struct(const char *name)
{
    struct Struct *s = CALLOC(struct Struct);
    s->name = name;

    return s;
}

struct Struct *DefineStruct(struct Scope *sc, const char *name)
{
    struct Struct *strct = new_struct(name);
    Symbol *sym = new_symbol(SYM_STRUCT, name, NewTypeStruct(strct));
    sym->strct = strct;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return strct;
}

struct Struct *FindStruct(const struct Scope *sc, const char *name)
{
    Symbol *sym = FindSymbol(sc, name);
    if (sym)
        return sym->strct;

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

struct Table *DefineTable(struct Scope *sc, const char *name)
{
    struct Table *tab = CALLOC(struct Table);
    tab->name = name;

    Symbol *sym = new_symbol(SYM_TABLE, name, NewTypeTable(tab));
    sym->table = tab;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return tab;
}

struct Module *DefineModule(struct Scope *sc, const char *name)
{
    struct Module *mod = CALLOC(struct Module);
    mod->name = name;
    mod->scope = new_scope(sc, next_var_id(sc));

    Symbol *sym = new_symbol(SYM_MODULE, name, NewTypeModule(mod));
    sym->module = mod;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return mod;
}

Symbol *FindSymbolThisScope(struct Scope *sc, const char *name)
{
    Symbol *sym = HashMapLookup(&sc->symbols, name);
    if (sym)
        return sym;

    return NULL;
}

Symbol *FindSymbol(const struct Scope *sc, const char *name)
{
    Symbol *sym = HashMapLookup(&sc->symbols, name);
    if (sym)
        return sym;

    if (sc->parent)
        return FindSymbol(sc->parent, name);

    return NULL;
}

static int next_var_id(const struct Scope *sc)
{
    return sc->var_offset_;
}

static int max_var_id(const struct Scope *sc)
{
    int max = next_var_id(sc) - 1;

    for (struct Scope *child = sc->children_; child; child = child->next) {
        int child_max = max_var_id(child);
        max = max < child_max ? child_max : max;
    }

    return max;
}

int VarSize(const struct Scope *sc)
{
    return next_var_id(sc);
}

int TotalVarSize(const struct Scope *sc)
{
    return max_var_id(sc) + 1;
}
