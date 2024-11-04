#include "scope.h"
#include "intern.h"
#include "type.h"
#include "mem.h"
#include <string.h>
#include <stdio.h>

/* TODO make specific vec structs */
void VecPush(struct Vec *v, void *data)
{
    if (!v)
        return;

    if (v->len >= v->cap) {
        v->cap = v->cap < 4 ? 4 : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(v->data[0]));
    }
    v->data[v->len++] = data;
}

void VecFree(struct Vec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}
/* ------------------------------ */

// Scope
struct Scope *NewScope(struct Scope *parent)
{
    struct Scope *sc = CALLOC(struct Scope);
    sc->parent = parent;
    return sc;
}

// Symbol
struct Symbol *NewSymbol(int kind, const char *name, const struct Type *type)
{
    struct Symbol *sym = CALLOC(struct Symbol);
    sym->kind = kind;
    sym->name = name;
    sym->type = type;
    return sym;
}

struct Symbol *FindSymbol(const struct Scope *sc, const char *name)
{
    struct data_hashmap_entry *ent = data_hashmap_lookup(&sc->symbols, name);
    if (ent)
        return ent->val;

    if (sc->parent)
        return FindSymbol(sc->parent, name);

    return NULL;
}

struct Symbol *FindSymbolThisScope(struct Scope *sc, const char *name)
{
    struct data_hashmap_entry *ent = data_hashmap_lookup(&sc->symbols, name);
    if (ent)
        return ent->val;

    return NULL;
}

// Var
static struct Var *new_var(const char *Name, const struct Type *t, bool global)
{
    struct Var *v = CALLOC(struct Var);
    v->name = Name;
    v->type = t;
    v->is_global = global;
    return v;
}

struct Symbol *DefineVar(struct Scope *sc, const char *name, const struct Type *type, bool isglobal)
{
    if (FindSymbolThisScope(sc, name))
        return NULL;

    struct Symbol *sym = NewSymbol(SYM_VAR, name, type);
    sym->var = new_var(name, type, isglobal);

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    VecPush(&sc->syms, sym);

    return sym;
}

static const char *func_fullname(const char *modulefile, const char *funcname)
{
    // unique func name
    static char fullname[1024] = {'\0'};
    static const size_t size = sizeof(fullname) / sizeof(fullname[0]);

    snprintf(fullname, size, "%s:%s", modulefile, funcname);
    return StrIntern(fullname);
}

// Func
static struct Func *new_func(struct Scope *parent, const char *modulefile, const char *name)
{
    struct Func *f = CALLOC(struct Func);
    f->name = name;
    f->fullname = func_fullname(modulefile, name);
    f->scope = NewScope(parent);
    f->is_builtin = false;
    return f;
}

struct Func *DeclareFunc(struct Scope *parent, const char *name, const char *modulefile)
{
    struct Func *func = new_func(parent, modulefile, name);

    if (FindSymbol(parent, func->name))
        return NULL;

    // Add func itself to symbol table
    struct Symbol *sym = NewSymbol(SYM_FUNC, func->name, NewFuncType(func->func_type));
    sym->func = func;

    if (!data_hashmap_insert(&parent->symbols, func->name, sym))
        return NULL;
    VecPush(&parent->syms, sym);

    return func;
}

struct Func *DeclareBuiltinFunc(struct Scope *parent, const char *name)
{
    struct Func *func = DeclareFunc(parent, name, ":buitin");
    func->is_builtin = true;
    return func;
}

struct FuncType *MakeFuncType(struct Func *func)
{
    struct FuncType *func_type = CALLOC(struct FuncType);

    func_type->return_type = func->return_type;
    for (int i = 0; i < func->params.len; i++) {
        const struct Var *var = GetParam(func, i);
        VecPush(&func_type->param_types, (void*) var->type);
    }

    func_type->is_builtin = func->is_builtin;
    func_type->is_variadic = func->is_variadic;
    func_type->has_special_var = func->has_special_var;

    return func_type;
}

void DeclareParam(struct Func *f, const char *name, const struct Type *type)
{
    struct Symbol *sym = DefineVar(f->scope, name, type, false);
    sym->var->is_param = true;
    VecPush(&f->params, sym->var);

    if (!strcmp(name, "..."))
        f->is_variadic = true;

    if (name[0] == '$')
        f->has_special_var = true;
}

const struct Var *GetParam(const struct Func *f, int index)
{
    int idx = 0;
    int param_count = f->params.len;

    if (f->is_variadic && index >= param_count)
        idx = param_count - 1;
    else
        idx = index;

    if (idx < 0 || idx >= param_count)
        return NULL;

    return f->params.data[idx];
}

const struct Type *GetParamType(const struct FuncType *func_type, int index)
{
    int idx = 0;
    int param_count = func_type->param_types.len;

    if (func_type->is_variadic && index >= param_count)
        idx = param_count - 1;
    else
        idx = index;

    if (idx < 0 || idx >= param_count)
        return NULL;

    return func_type->param_types.data[idx];
}

int RequiredParamCount(const struct FuncType *func_type)
{
    int param_count = func_type->param_types.len;

    if (func_type->is_variadic)
        return param_count - 1;
    else
        return param_count;
}

// Struct
static struct Struct *new_struct(const char *name)
{
    struct Struct *s = CALLOC(struct Struct);
    s->name = name;

    return s;
}

struct Struct *DefineStruct(struct Scope *sc, const char *name)
{
    struct Struct *strct = new_struct(name);
    struct Symbol *sym = NewSymbol(SYM_STRUCT, name, NewStructType(strct));
    sym->strct = strct;

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    VecPush(&sc->syms, sym);

    return strct;
}

struct Struct *FindStruct(const struct Scope *sc, const char *name)
{
    struct Symbol *sym = FindSymbol(sc, name);
    if (sym)
        return sym->strct;

    return NULL;
}

static struct Field *new_field(const char *Name, const struct Type *type, int offset)
{
    struct Field *f = CALLOC(struct Field);
    f->name = Name;
    f->type = type;
    f->offset = offset;
    return f;
}

struct Field *AddField(struct Struct *strct, const char *name, const struct Type *type)
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

int parser_struct_get_field_count(const struct Struct *s)
{
    return s->fields.len;
}

// Table
struct Table *DefineTable(struct Scope *sc, const char *name)
{
    struct Table *tab = CALLOC(struct Table);
    tab->name = name;

    struct Symbol *sym = NewSymbol(SYM_TABLE, name, NewTableType(tab));
    sym->table = tab;

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    VecPush(&sc->syms, sym);

    return tab;
}

// Module
struct Module *DefineModule(struct Scope *sc, const char *filename, const char *modulename)
{
    struct Module *mod = CALLOC(struct Module);
    mod->name = modulename;
    mod->filename = filename;
    mod->scope = NewScope(sc);

    struct Symbol *sym = NewSymbol(SYM_MODULE, modulename, NewModuleType(mod));
    sym->module = mod;

    if (!data_hashmap_insert(&sc->symbols, modulename, sym))
        return NULL;
    VecPush(&sc->syms, sym);

    return mod;
}
