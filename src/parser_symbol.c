#include "parser_symbol.h"
#include "parser_type.h"
#include "data_intern.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_CAP 8

/* vec */
static void push_type(struct parser_typevec *v, const struct parser_type *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_var(struct parser_varvec *v, struct parser_var *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_func(struct parser_funcvec *v, struct parser_func *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_field(struct parser_fieldvec *v, struct parser_field *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_symbol(struct parser_symbolvec *v, struct parser_symbol *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

/* scope */
struct parser_scope *parser_new_scope(struct parser_scope *parent)
{
    struct parser_scope *sc;
    sc = calloc(1, sizeof(*sc));
    sc->parent = parent;
    return sc;
}

void parser_scope_add_symbol(struct parser_scope *sc, struct parser_symbol *sym)
{
    push_symbol(&sc->syms, sym);
}

/* symbol */
struct parser_symbol *parser_new_symbol(int kind,
        const char *name, const struct parser_type *type)
{
    struct parser_symbol *sym;

    sym = calloc(1, sizeof(*sym));
    sym->kind = kind;
    sym->name = name;
    sym->type = type;

    return sym;
}

struct parser_symbol *parser_find_symbol(const struct parser_scope *sc,
        const char *name)
{
    struct data_hashmap_entry *ent = data_hashmap_lookup(&sc->symbols, name);
    if (ent)
        return ent->val;

    if (sc->parent)
        return parser_find_symbol(sc->parent, name);

    return NULL;
}

struct parser_symbol *FindSymbolThisScope(struct parser_scope *sc,
        const char *name)
{
    struct data_hashmap_entry *ent = data_hashmap_lookup(&sc->symbols, name);
    if (ent)
        return ent->val;

    return NULL;
}

/* var */
static struct parser_var *new_var(const char *Name,
        const struct parser_type *t, bool global)
{
    struct parser_var *v;

    v = calloc(1, sizeof(*v));
    v->name = Name;
    v->type = t;
    v->is_global = global;
    return v;
}

struct parser_symbol *parser_define_var(struct parser_scope *sc,
        const char *name, const struct parser_type *type, bool isglobal)
{
    if (FindSymbolThisScope(sc, name))
        return NULL;

    struct parser_symbol *sym = parser_new_symbol(SYM_VAR, name, type);
    sym->var = new_var(name, type, isglobal);

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    push_symbol(&sc->syms, sym);

    return sym;
}

static const char *func_fullname(const char *modulename, const char *funcname)
{
    /* unique func name */
    static char fullname[1024] = {'\0'};
    static const size_t size = sizeof(fullname) / sizeof(fullname[0]);

    snprintf(fullname, size, "%s:%s", modulename, funcname);
    return data_string_intern(fullname);
}

/* func */
static struct parser_func *new_func(struct parser_scope *parent,
        const char *modulename, const char *name)
{
    struct parser_func *f;

    f = calloc(1, sizeof(*f));
    f->name = name;
    f->fullname = func_fullname(modulename, name);
    f->scope = parser_new_scope(parent);
    f->is_builtin = false;

    return f;
}

struct parser_func *parser_declare_func(struct parser_scope *parent,
        const char *name, const char *modulename)
{
    struct parser_func *func = new_func(parent, modulename, name);

    if (parser_find_symbol(parent, func->name))
        return NULL;

    /* add func itself to symbol table */
    struct parser_symbol *sym = parser_new_symbol(SYM_FUNC,
            func->name, parser_new_func_type(func->func_sig));
    sym->func = func;

    if (!data_hashmap_insert(&parent->symbols, func->name, sym))
        return NULL;
    push_symbol(&parent->syms, sym);

    return func;
}

struct parser_func *parser_declare_builtin_func(struct parser_scope *parent,
        const char *name)
{
    struct parser_func *func = parser_declare_func(parent, name, "_builtin");
    func->is_builtin = true;
    return func;
}

static const struct parser_var *parser_get_param(const struct parser_func *f, int index);

struct parser_func_sig *parser_make_func_sig(struct parser_func *func)
{
    struct parser_func_sig *func_sig;

    func_sig = calloc(1, sizeof(*func_sig));
    func_sig->return_type = func->return_type;

    for (int i = 0; i < func->params.len; i++) {
        const struct parser_var *var = parser_get_param(func, i);
        push_type(&func_sig->param_types, var->type);
    }

    func_sig->is_builtin = func->is_builtin;
    func_sig->is_variadic = func->is_variadic;
    func_sig->has_special_var = func->has_special_var;

    return func_sig;
}

void parser_declare_param(struct parser_func *f,
        const char *name, const struct parser_type *type)
{
    struct parser_symbol *sym = parser_define_var(f->scope, name, type, false);
    sym->var->is_param = true;
    push_var(&f->params, sym->var);

    if (!strcmp(name, "..."))
        f->is_variadic = true;

    if (name[0] == '$')
        f->has_special_var = true;
}

static const struct parser_var *parser_get_param(const struct parser_func *f,
        int index)
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

const struct parser_type *parser_get_param_type(const struct parser_func_sig *func_sig,
        int index)
{
    int idx = 0;
    int param_count = func_sig->param_types.len;

    if (func_sig->is_variadic && index >= param_count)
        idx = param_count - 1;
    else
        idx = index;

    if (idx < 0 || idx >= param_count)
        return NULL;

    return func_sig->param_types.data[idx];
}

int parser_required_param_count(const struct parser_func_sig *func_sig)
{
    int param_count = func_sig->param_types.len;

    if (func_sig->is_variadic)
        return param_count - 1;
    else
        return param_count;
}

/* struct */
static struct parser_struct *new_struct(const char *name)
{
    struct parser_struct *s;

    s = calloc(1, sizeof(*s));
    s->name = name;

    return s;
}

struct parser_struct *parser_define_struct(struct parser_scope *sc,
        const char *name)
{
    struct parser_struct *strct = new_struct(name);
    struct parser_symbol *sym = parser_new_symbol(SYM_STRUCT,
            name, parser_new_struct_type(strct));
    sym->strct = strct;

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    push_symbol(&sc->syms, sym);

    return strct;
}

struct parser_struct *parser_find_struct(const struct parser_scope *sc,
        const char *name)
{
    struct parser_symbol *sym = parser_find_symbol(sc, name);
    if (sym)
        return sym->strct;

    return NULL;
}

static struct parser_field *new_field(const char *Name,
        const struct parser_type *type, int offset)
{
    struct parser_field *f;

    f = calloc(1, sizeof(*f));
    f->name = Name;
    f->type = type;
    f->offset = offset;
    return f;
}

struct parser_field *parser_add_field(struct parser_struct *strct,
        const char *name, const struct parser_type *type)
{
    if (parser_find_field(strct, name))
        return NULL;

    struct parser_field *f = new_field(name, type, strct->size);
    strct->size += parser_sizeof_type(f->type);

    push_field(&strct->fields, f);
    return f;
}

struct parser_field *parser_find_field(const struct parser_struct *strct,
        const char *name)
{
    for (int i = 0; i < strct->fields.len; i++) {
        struct parser_field *f = strct->fields.data[i];
        if (!strcmp(f->name, name))
            return f;
    }
    return NULL;
}

int parser_struct_get_field_count(const struct parser_struct *s)
{
    return s->fields.len;
}

/* table */
struct parser_table *parser_define_table(struct parser_scope *sc,
        const char *name)
{
    struct parser_table *tab;

    tab = calloc(1, sizeof(*tab));
    tab->name = name;

    struct parser_symbol *sym = parser_new_symbol(SYM_TABLE,
            name, parser_new_table_type(tab));
    sym->table = tab;

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    push_symbol(&sc->syms, sym);

    return tab;
}

/* module */
struct parser_module *parser_define_module(struct parser_scope *sc,
        const char *filename, const char *modulename)
{
    struct parser_module *mod;

    mod = calloc(1, sizeof(*mod));
    mod->name = modulename;
    mod->filename = filename;
    mod->scope = parser_new_scope(sc);

    struct parser_symbol *sym = parser_new_symbol(SYM_MODULE,
            modulename, parser_new_module_type(mod));
    sym->module = mod;

    if (!data_hashmap_insert(&sc->symbols, modulename, sym))
        return NULL;
    push_symbol(&sc->syms, sym);

    return mod;
}

void parser_module_add_func(struct parser_module *mod,
        struct parser_func *func)
{
    push_func(&mod->funcs, func);
}
