#include "parser_symbol.h"
#include "parser_ast.h"
#include "data_intern.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

#define MIN_CAP 8

/* vec */
static void push_func(struct parser_funcvec *v, struct parser_func *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void free_funcvec(struct parser_funcvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

static void push_struct_field(struct parser_struct_fieldvec *v, struct parser_struct_field *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_enum_field(struct parser_enum_fieldvec *v, struct parser_enum_field *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_expr(struct parser_exprvec *v, struct parser_expr *val)
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

static void free_symbolvec(struct parser_symbolvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

/* scope */
struct parser_scope *parser_new_scope(struct parser_scope *parent)
{
    struct parser_scope *sc;
    sc = calloc(1, sizeof(*sc));
    sc->parent = parent;
    return sc;
}

static void free_var(struct parser_var *var);
static void free_func(struct parser_func *func);
static void free_struct(struct parser_struct *strct);
static void free_enum(struct parser_enum *enm);

void parser_free_scope(struct parser_scope *sc)
{
    for (int i = 0; i < sc->syms.len; i++) {
        struct parser_symbol *sym = sc->syms.data[i];

        switch ((enum parser_symbol_kind) sym->kind) {

        case SYM_VAR:
            free_var(sym->var);
            break;

        case SYM_FUNC:
            free_func(sym->func);
            break;

        case SYM_STRUCT:
            free_struct(sym->strct);
            break;

        case SYM_ENUM:
            free_enum(sym->enm);
            break;

        case SYM_MODULE:
            parser_free_module(sym->module);
            break;

        case SYM_SCOPE:
            parser_free_scope(sym->scope);
            break;
        }

        free(sym);
    }

    free_symbolvec(&sc->syms);
    data_hashmap_free(&sc->symbols);
    free(sc);
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

struct parser_symbol *find_symbol_this_scope(struct parser_scope *sc,
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

static void free_var(struct parser_var *var)
{
    free(var);
}

struct parser_symbol *parser_define_var(struct parser_scope *sc,
        const char *name, const struct parser_type *type, bool isglobal)
{
    if (find_symbol_this_scope(sc, name))
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
    struct parser_func *func;

    func = calloc(1, sizeof(*func));
    func->sig = calloc(1, sizeof(*func->sig));
    func->name = name;
    func->fullname = func_fullname(modulename, name);
    func->scope = parser_new_scope(parent);

    return func;
}

static void free_func_sig(struct parser_func_sig *sig)
{
    parser_typevec_free(&sig->param_types);
    free(sig);
}

static void free_func(struct parser_func *func)
{
    free_func_sig(func->sig);
    parser_free_scope(func->scope);
    parser_free_stmt(func->body);
    free(func);
}

struct parser_func *parser_declare_func(struct parser_scope *parent,
        const char *modulename, const char *name)
{
    if (find_symbol_this_scope(parent, name))
        return NULL;

    struct parser_func *func = new_func(parent, modulename, name);

    /* add func itself to symbol enum */
    struct parser_symbol *sym = parser_new_symbol(SYM_FUNC,
            func->name, parser_new_func_type(func->sig));
    sym->func = func;

    if (!data_hashmap_insert(&parent->symbols, func->name, sym))
        return NULL;
    push_symbol(&parent->syms, sym);

    return func;
}

struct parser_func *parser_declare_native_func(struct parser_scope *parent,
        const char *modulename, const char *name, native_func_t func_ptr)
{
    struct parser_func *func = parser_declare_func(parent, modulename, name);
    func->native_func_ptr = func_ptr;
    func->sig->is_native = true;
    return func;
}

void parser_declare_param(struct parser_func *func,
        const char *name, const struct parser_type *type)
{
    struct parser_symbol *sym = parser_define_var(func->scope, name, type, false);
    sym->var->is_param = true;

    if (!strcmp(name, "..."))
        func->sig->is_variadic = true;

    if (name[0] == '$')
        func->sig->has_special_var = true;

    /* func sig */
    parser_typevec_push(&func->sig->param_types, type);
}

void parser_add_return_type(struct parser_func *func, const struct parser_type *type)
{
    func->sig->return_type = type;

    if (parser_has_template_type(func->sig->return_type))
        func->sig->has_template_return_type = true;
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

bool parser_require_type_sequence(const struct parser_func_sig *func_sig)
{
    return func_sig->is_variadic || func_sig->has_union_param;
}

bool parser_match_func_signature(const struct parser_func_sig *sig1,
        const struct parser_func_sig *sig2)
{
    if (!parser_match_type(sig1->return_type, sig2->return_type))
        return false;

    int len1 = parser_required_param_count(sig1);
    int len2 = parser_required_param_count(sig2);

    if (len1 != len2)
        return false;

    for (int i = 0; i < len1; i++) {
        const struct parser_type *t1 = parser_get_param_type(sig1, i);
        const struct parser_type *t2 = parser_get_param_type(sig2, i);

        if (!parser_match_type(t1, t2))
            return false;
    }

    return true;
}

/* struct */
static struct parser_struct *new_struct(const char *name)
{
    struct parser_struct *s;

    s = calloc(1, sizeof(*s));
    s->name = name;

    return s;
}

static void free_struct_field(struct parser_struct_field *field)
{
    free(field);
}

static void free_fieldvec(struct parser_struct_fieldvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

static void free_struct(struct parser_struct *strct)
{
    for (int i = 0; i < strct->fields.len; i++) {
        struct parser_struct_field *f = strct->fields.data[i];
        free_struct_field(f);
    }
    free_fieldvec(&strct->fields);
    free(strct);
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

static struct parser_struct_field *new_struct_field(const char *Name,
        const struct parser_type *type, int offset)
{
    struct parser_struct_field *f;

    f = calloc(1, sizeof(*f));
    f->name = Name;
    f->type = type;
    f->offset = offset;
    return f;
}

struct parser_struct_field *parser_add_struct_field(struct parser_struct *strct,
        const char *name, const struct parser_type *type)
{
    if (parser_find_struct_field(strct, name))
        return NULL;

    int offset = strct->fields.len;
    struct parser_struct_field *f = new_struct_field(name, type, offset);

    push_struct_field(&strct->fields, f);
    return f;
}

struct parser_struct_field *parser_find_struct_field(const struct parser_struct *strct,
        const char *name)
{
    for (int i = 0; i < strct->fields.len; i++) {
        struct parser_struct_field *f = strct->fields.data[i];
        if (!strcmp(f->name, name))
            return f;
    }
    return NULL;
}

int parser_struct_get_field_count(const struct parser_struct *s)
{
    return s->fields.len;
}

struct parser_struct_field *parser_get_struct_field(const struct parser_struct *s, int idx)
{
    assert(idx >= 0 && idx < s->fields.len);
    return s->fields.data[idx];
}

/* enum */
struct parser_enum *parser_define_enum(struct parser_scope *sc,
        const char *name)
{
    struct parser_enum *enm;

    enm = calloc(1, sizeof(*enm));
    enm->name = name;

    struct parser_symbol *sym = parser_new_symbol(SYM_ENUM,
            name, parser_new_enum_type(enm));
    sym->enm = enm;

    if (!data_hashmap_insert(&sc->symbols, name, sym))
        return NULL;
    push_symbol(&sc->syms, sym);

    return enm;
}

struct parser_enum *parser_find_enum(const struct parser_scope *sc,
        const char *name)
{
    struct parser_symbol *sym = parser_find_symbol(sc, name);
    if (sym)
        return sym->enm;

    return NULL;
}

int parser_add_enum_member(struct parser_enum *enm, const char *name)
{
    int idx = parser_find_enum_member(enm, name);
    if (idx >= 0)
        return -1;

    int64_t new_idx = parser_get_enum_member_count(enm);
    data_hashmap_insert(&enm->members, name, (void*)new_idx);

    return new_idx;
}

int parser_find_enum_member(const struct parser_enum *enm, const char *name)
{
    struct data_hashmap_entry *ent;
    ent = data_hashmap_lookup(&enm->members, name);

    if (!ent)
        return -1;

    int64_t idx = (int64_t) ent->val;
    return idx;
}

int parser_get_enum_member_count(const struct parser_enum *enm)
{
    return data_hashmap_get_count(&enm->members);
}

static struct parser_enum_field *new_enum_field(const char *Name,
        const struct parser_type *type, int id)
{
    struct parser_enum_field *f;

    f = calloc(1, sizeof(*f));
    f->name = Name;
    f->type = type;
    f->id = id;
    return f;
}

struct parser_enum_field *parser_add_enum_field(struct parser_enum *enm, const char *name)
{
    if (parser_find_enum_field(enm, name))
        return NULL;

    int new_id = enm->fields.len;
    struct parser_enum_field *f = new_enum_field(name, NULL, new_id);

    push_enum_field(&enm->fields, f);
    return f;
}

struct parser_enum_field *parser_find_enum_field(const struct parser_enum *enm, const char *name)
{
    for (int i = 0; i < enm->fields.len; i++) {
        struct parser_enum_field *f = enm->fields.data[i];
        if (!strcmp(f->name, name))
            return f;
    }
    return NULL;
}

struct parser_enum_field *parser_get_enum_field(const struct parser_enum *enm, int idx)
{
    assert(idx >= 0 && idx < parser_get_enum_field_count(enm));
    return enm->fields.data[idx];
}

int parser_get_enum_field_count(const struct parser_enum *enm)
{
    return enm->fields.len;
}

void parser_add_enum_value_expr(struct parser_enum *enm, struct parser_expr *e)
{
    push_expr(&enm->valueexprs, e);
}

struct parser_enum_value parser_get_enum_value(const struct parser_enum *enm, int x, int y)
{
    int fields = parser_get_enum_field_count(enm);
    int idx = x + y * fields;
    struct parser_expr *expr = enm->valueexprs.data[idx];
    struct parser_enum_value val = {.ival = expr->ival};

    return val;
}

static void free_enum_field(struct parser_enum_field *f)
{
    free(f);
}

static void free_enum_fieldvec(struct parser_enum_fieldvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

static void free_exprvec(struct parser_exprvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
}

static void free_enum(struct parser_enum *enm)
{
    data_hashmap_free(&enm->members);

    /* enum fields */
    for (int i = 0; i < enm->fields.len; i++) {
        struct parser_enum_field *f = enm->fields.data[i];
        free_enum_field(f);
    }
    free_enum_fieldvec(&enm->fields);

    /* enum values */
    for (int i = 0; i < enm->valueexprs.len; i++) {
        struct parser_expr *e = enm->valueexprs.data[i];
        parser_free_expr(e);
    }
    free_exprvec(&enm->valueexprs);

    free(enm);
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

void parser_free_module(struct parser_module *mod)
{
    parser_free_scope(mod->scope);
    parser_free_stmt(mod->gvars);
    free_funcvec(&mod->funcs);
    free(mod);
}

void parser_module_add_func(struct parser_module *mod,
        struct parser_func *func)
{
    push_func(&mod->funcs, func);
}
