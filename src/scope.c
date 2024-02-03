#include "scope.h"
#include "intern.h"
#include "type.h"
#include "mem.h"
#include <string.h>
#include <stdio.h>

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
    Symbol *sym = new_symbol(SYM_STRUCT, name, NewStructType(strct));
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

    Symbol *sym = new_symbol(SYM_TABLE, name, NewTableType(tab));
    sym->table = tab;

    if (!HashMapInsert(&sc->symbols, name, sym))
        return NULL;

    return tab;
}

struct Module *DefineModule(struct Scope *sc, const char *filename, const char *modulename)
{
    struct Module *mod = CALLOC(struct Module);
    mod->name = modulename;
    mod->filename = filename;
    mod->scope = new_scope(sc, next_var_id(sc));

    Symbol *sym = new_symbol(SYM_MODULE, modulename, NewModuleType(mod));
    sym->module = mod;

    if (!HashMapInsert(&sc->symbols, modulename, sym))
        return NULL;

    return mod;
}

Symbol *FindSymbolThisScope(struct Scope *sc, const char *name)
{
    struct MapEntry *ent = HashMapLookup(&sc->symbols, name);
    if (ent)
        return ent->val;

    return NULL;
}

Symbol *FindSymbol(const struct Scope *sc, const char *name)
{
    struct MapEntry *ent = HashMapLookup(&sc->symbols, name);
    if (ent)
        return ent->val;

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

static const char *func_fullname(const char *modulefile, const char *funcname)
{
    // unique func name
    char fullname[1024] = {'\0'};
    size_t size = sizeof(fullname) / sizeof(fullname[0]);

    snprintf(fullname, size, "%s:%s", modulefile, funcname);
    return StrIntern(fullname);
}

// Func
struct Func *AddFunc(struct Scope *parent, const char *modulefile, const char *name)
{
    struct Func *f = CALLOC(struct Func);
    int offset = 0;

    f->name = name;
    f->fullname = func_fullname(modulefile, name);
    f->scope = NewScope(parent, offset);
    f->is_builtin = false;

    return f;
}

struct Func *AddBuiltinFunc(struct Scope *parent, const char *name)
{
    struct Func *f = CALLOC(struct Func);
    int offset = 0;

    f->name = name;
    f->fullname = func_fullname(":buitin", name);
    f->scope = NewScope(parent, offset);
    f->is_builtin = true;

    return f;
}

static int param_count(const struct Func *f)
{
    return f->params.len;
}

void DeclareParam(struct Func *f, const char *name, const Type *type)
{
    struct Symbol *sym = DefineVar(f->scope, name, type, false);
    VecPush(&f->params, sym->var);

    if (!strcmp(name, "..."))
        f->is_variadic = true;

    if (name[0] == '$')
        f->has_special_var = true;
}

const struct Var *GetParam(const struct Func *f, int index)
{
    int idx = 0;

    if (f->is_variadic && index >= param_count(f))
        idx = param_count(f) - 1;
    else
        idx = index;

    if (idx < 0 || idx >= param_count(f))
        return NULL;

    return f->params.data[idx];
}

int RequiredParamCount(const struct Func *f)
{
    if (f->is_variadic)
        return param_count(f) - 1;
    else
        return param_count(f);
}

// Struct
static struct Field *new_field(const char *Name, const Type *type, int offset)
{
    struct Field *f = CALLOC(struct Field);
    f->name = Name;
    f->type = type;
    f->offset = offset;
    return f;
}

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
        struct Field *f = strct->fields.data[i];
        if (!strcmp(f->name, name))
            return f;
    }
    return NULL;
}
