#include "scope.h"
#include "type.h"
#include "mem.h"
#include <string.h>

// Func
void DeclareParam(Func *f, const char *name, const Type *type)
{
    struct Symbol *sym = DefineVar(f->scope, name, type, false);
    VecPush(&f->params, sym->var);

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

    return f->params.data[idx];
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
    return f->params.len;
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

static Field *new_field(const char *Name, const Type *type, int offset)
{
    Field *f = CALLOC(Field);
    f->name = Name;
    f->type = type;
    f->offset = offset;
    return f;
}

// Struct
struct Field *AddField(struct Struct *strct, const char *name, const Type *type)
{
    if (FindField(strct, name))
        return NULL;

    struct Field *f = new_field(name, type, strct->size);
    strct->size += SizeOf(f->type);

    VecPush(&strct->fields, f);
    return f;
}

struct Field *FindField(const struct Struct *strct, const char *name)
{
    for (int i = 0; i < strct->fields.len; i++) {
        Field *f = strct->fields.data[i];
        if (!strcmp(f->name, name))
            return f;
    }
    return NULL;
}

// Scope
static Scope *new_scope(Scope *parent, int var_offset)
{
    Scope *sc = CALLOC(Scope);
    sc->parent = parent;
    sc->var_offset_ = var_offset;

    return sc;
}

static int next_var_id(const Scope *sc);

Scope *OpenChild(Scope *sc)
{
    const int next_id = sc->var_offset_;

    Scope *child = new_scope(sc, next_id);
    if (!sc->children_)
        sc->child_tail = sc->children_ = child;
    else
        sc->child_tail = sc->child_tail->next = child;

    return child;
}

static Var *new_var(const char *Name, const Type *t, int ID, bool global)
{
    Var *v = CALLOC(Var);
    v->name = Name;
    v->type = t;
    v->id = ID;
    v->is_global = global;
    return v;
}

// TODO remove forward decls
static Symbol *new_symbol(int kind, const char *name, const Type *t);
Symbol *FindSymbolThisScope(Scope *sc, const char *name);

struct Symbol *DefineVar(Scope *sc, const char *name, const Type *type, bool isglobal)
{
    if (FindSymbolThisScope(sc, name))
        return NULL;

    const int next_id = next_var_id(sc);
    Var *var = new_var(name, type, next_id, isglobal);

    Symbol *sym = new_symbol(SYM_VAR, name, type);
    sym->var = var;

    sc->var_offset_ += SizeOf(var->type);

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return sym;
}

Func *new_func(Scope *parent, bool builtin)
{
    Func *f = CALLOC(Func);
    const int next_id = 0;
    f->scope = new_scope(parent, next_id);
    f->is_builtin = builtin;
    f->ellipsis_index = -1;

    return f;
}

Func *DeclareFunc(Scope *sc, bool isbuiltin)
{
    Func *func = new_func(sc, isbuiltin);

    func->next = sc->funcs_;
    sc->funcs_ = func;

    return func;
}

static struct Struct *new_struct(const char *name)
{
    struct Struct *s = CALLOC(struct Struct);
    s->name = name;

    return s;
}

struct Struct *DefineStruct(Scope *sc, const char *name)
{
    struct Struct *strct = new_struct(name);
    Symbol *sym = new_symbol(SYM_STRUCT, name, NewTypeStruct(strct));
    sym->strct = strct;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return strct;
}

struct Struct *FindStruct(const Scope *sc, const char *name)
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

struct Module *DefineModule(Scope *sc, const char *name)
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

Symbol *FindSymbolThisScope(Scope *sc, const char *name)
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
