#include "parser_type.h"
#include "parser_symbol.h"
#include "data_intern.h"
#include "data_strbuf.h"
#include "assert.h"

#include <stdio.h>
#include <stdlib.h>

/* TODO consider allocate basic types */
struct parser_type *parser_new_nil_type(void)
{
    static struct parser_type t;
    t.kind = TYP_NIL;
    return &t;
}

struct parser_type *parser_new_bool_type(void)
{
    static struct parser_type t;
    t.kind = TYP_BOOL;
    return &t;
}

struct parser_type *parser_new_int_type(void)
{
    static struct parser_type t;
    t.kind = TYP_INT;
    return &t;
}

struct parser_type *parser_new_float_type(void)
{
    static struct parser_type t;
    t.kind = TYP_FLOAT;
    return &t;
}

struct parser_type *parser_new_string_type(void)
{
    static struct parser_type t;
    t.kind = TYP_STRING;
    return &t;
}

static struct parser_type *new_type(int kind)
{
    struct parser_type *t;

    t = calloc(1, sizeof(*t));
    t->kind = kind;

    return t;
}

struct parser_type *parser_new_func_type(const struct parser_func_sig *func_sig)
{
    struct parser_type *t = new_type(TYP_FUNC);
    t->func_sig = func_sig;
    return t;
}

struct parser_type *parser_new_array_type(const struct parser_type *underlying)
{
    struct parser_type *t = new_type(TYP_ARRAY);
    t->underlying = underlying;
    return t;
}

struct parser_type *parser_new_map_type(const struct parser_type *underlying)
{
    struct parser_type *t = new_type(TYP_MAP);
    t->underlying = underlying;
    return t;
}

struct parser_type *parser_new_set_type(const struct parser_type *underlying)
{
    struct parser_type *t = new_type(TYP_SET);
    t->underlying = underlying;
    return t;
}

struct parser_type *parser_new_struct_type(const struct parser_struct *s)
{
    struct parser_type *t = new_type(TYP_STRUCT);
    t->strct = s;
    return t;
}

struct parser_type *parser_new_enum_type(const struct parser_enum *e)
{
    struct parser_type *t = new_type(TYP_ENUM);
    t->enm = e;
    return t;
}

struct parser_type *parser_new_module_type(const struct parser_module *mod)
{
    struct parser_type *t = new_type(TYP_MODULE);
    t->module = mod;
    return t;
}

struct parser_type *parser_new_any_type(void)
{
    static struct parser_type t;
    t.kind = TYP_ANY;
    return &t;
}

struct parser_type *parser_new_union_type(int id)
{
    struct parser_type *t = new_type(TYP_UNION);
    t->template_id = id;
    return t;
}

struct parser_type *parser_new_template_type(int id)
{
    struct parser_type *t = new_type(TYP_TEMPLATE);
    t->template_id = id;
    return t;
}

bool parser_is_nil_type(const struct parser_type *t)      { return t->kind == TYP_NIL; }
bool parser_is_bool_type(const struct parser_type *t)     { return t->kind == TYP_BOOL; }
bool parser_is_int_type(const struct parser_type *t)      { return t->kind == TYP_INT; }
bool parser_is_float_type(const struct parser_type *t)    { return t->kind == TYP_FLOAT; }
bool parser_is_string_type(const struct parser_type *t)   { return t->kind == TYP_STRING; }
bool parser_is_func_type(const struct parser_type *t)     { return t->kind == TYP_FUNC; }
bool parser_is_array_type(const struct parser_type *t)    { return t->kind == TYP_ARRAY; }
bool parser_is_map_type(const struct parser_type *t)      { return t->kind == TYP_MAP; }
bool parser_is_set_type(const struct parser_type *t)      { return t->kind == TYP_SET; }
bool parser_is_struct_type(const struct parser_type *t)   { return t->kind == TYP_STRUCT; }
bool parser_is_enum_type(const struct parser_type *t)     { return t->kind == TYP_ENUM; }
bool parser_is_module_type(const struct parser_type *t)   { return t->kind == TYP_MODULE; }
bool parser_is_any_type(const struct parser_type *t)      { return t->kind == TYP_ANY; }
bool parser_is_union_type(const struct parser_type *t)    { return t->kind == TYP_UNION; }
bool parser_is_template_type(const struct parser_type *t) { return t->kind == TYP_TEMPLATE; }

bool parser_has_template_type(const struct parser_type *t)
{
    if (parser_is_array_type(t))
        return parser_has_template_type(t->underlying);
    else
        return parser_is_template_type(t);
}

static const char *type_kind_string(int kind)
{
    switch ((enum parser_type_kind) kind) {
    case TYP_NIL:      return "nil";
    case TYP_BOOL:     return "bool";
    case TYP_INT:      return "int";
    case TYP_FLOAT:    return "float";
    case TYP_STRING:   return "string";
    case TYP_FUNC:     return "func";
    case TYP_ARRAY:    return "[]";
    case TYP_MAP:      return "{}";
    case TYP_SET:      return "set{}";
    case TYP_STRUCT:   return "struct";
    case TYP_ENUM:     return "enum";
    case TYP_MODULE:   return "module";
    case TYP_ANY:      return "any";
    case TYP_UNION:    return "union";
    case TYP_TEMPLATE: return "template";
    }

    assert("unreachable");
    return NULL;
}

const char *parser_type_string(const struct parser_type *t)
{
    const char *interned = "";

    for (const struct parser_type *type = t; type; type = type->underlying) {
        /* TODO take care of buffer overflow */
        char buf[128] = {'\0'};

        if (parser_is_array_type(type)) {
            sprintf(buf, "[]%s", interned);
        }
        else if (parser_is_struct_type(type)) {
            sprintf(buf, "%s%s", interned, type->strct->name);
        }
        else if (parser_is_enum_type(type)) {
            sprintf(buf, "%s%s", interned, type->enm->name);
        }
        else if (parser_is_module_type(type)) {
            sprintf(buf, "%s%s", interned, type->module->name);
        }
        else if (parser_is_template_type(type)) {
            sprintf(buf, "%stype%d", interned, type->template_id);
        }
        else {
            sprintf(buf, "%s%s", interned, type_kind_string(type->kind));
        }

        interned = data_string_intern(buf);
    }

    return interned;
}

static bool find_in_union(const struct parser_type *uni, const struct parser_type *t)
{
    if (!parser_is_union_type(uni))
        return false;

    for (int i = 0; i < uni->unions.len; i++) {
        const struct parser_type *u = uni->unions.data[i];

        if (parser_match_type(u, t))
            return true;
    }

    return false;
}

bool parser_match_type(const struct parser_type *t1, const struct parser_type *t2)
{
    if (parser_is_any_type(t1) || parser_is_any_type(t2))
        return true;

    if (parser_is_template_type(t1) && !parser_is_template_type(t2))
        return true;

    if (!parser_is_template_type(t1) && parser_is_template_type(t2))
        return true;

    if (parser_is_template_type(t1) && parser_is_template_type(t2))
        return t1->template_id == t2->template_id;

    if (parser_is_array_type(t1) && parser_is_array_type(t2))
        return parser_match_type(t1->underlying, t2->underlying);

    if (parser_is_union_type(t1) && !parser_is_union_type(t2))
        return find_in_union(t1, t2);

    if (!parser_is_union_type(t1) && parser_is_union_type(t2))
        return find_in_union(t2, t1);

    if (parser_is_func_type(t1) && parser_is_func_type(t2))
        return parser_match_func_signature(t1->func_sig, t2->func_sig);

    return t1->kind == t2->kind;
}

struct parser_type *parser_duplicate_type(const struct parser_type *t)
{
    struct parser_type *dup = new_type(0);
    *dup = *t;
    return dup;
}

void parser_add_union_type(struct parser_type *uni, const struct parser_type *t)
{
    if (find_in_union(uni, t))
        return;

    parser_typevec_push(&uni->unions, t);
}

#define MIN_CAP 8

/* type vec */
void parser_typevec_push(struct parser_typevec *v, const struct parser_type *val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

/* type list */
static const int table[] = {
    [TYP_NIL]      = 'n',
    [TYP_BOOL]     = 'b',
    [TYP_INT]      = 'i',
    [TYP_FLOAT]    = 'f',
    [TYP_STRING]   = 's',
    [TYP_FUNC]     = 'F',
    [TYP_ARRAY]    = 'A',
    [TYP_MAP]      = 'M',
    [TYP_SET]      = 'S',
    [TYP_STRUCT]   = 'R',
    [TYP_ENUM]     = 'E',
    [TYP_MODULE]   = 'm',
    [TYP_ANY]      = 'a',
    [TYP_UNION]    = 'u',
    [TYP_TEMPLATE] = 't',
};
static const int tablesize = sizeof(table)/sizeof(table[0]);

static int kind_to_char(int kind)
{
    assert(kind >= 0 && kind < tablesize);
    return table[kind];
}

static int char_to_kind(int ch)
{
    for (int kind = 0; kind < tablesize; kind++) {
        if (ch == table[kind])
            return kind;
    }
    assert(!"variadic argument error");
    return -1;
}

void parser_typelist_begin(struct parser_typelist_iterator *it, const char *typelist)
{
    assert(typelist);
    it->curr = typelist - 1;
    it->kind = TYP_NIL;
    parser_typelist_next(it);
}

bool parser_typelist_end(const struct parser_typelist_iterator *it)
{
    return it->kind == -1;
}

bool parser_typelist_struct_end(const struct parser_typelist_iterator *it)
{
    return it->kind == -2;
}

int parser_typelist_next(struct parser_typelist_iterator *it)
{
    it->curr++;
    int ch = *it->curr;

    if (ch == '\0')
        it->kind = -1;
    else if (ch == '.')
        it->kind = -2;
    else
        it->kind = char_to_kind(ch);

    return it->kind;
}

void parser_typelist_push(struct data_strbuf *sb, const struct parser_type *t)
{
    char ch = kind_to_char(t->kind);
    data_strbuf_push(sb, ch);

    if (parser_is_array_type(t)) {
        parser_typelist_push(sb, t->underlying);
    }
    else if (parser_is_map_type(t)) {
        parser_typelist_push(sb, t->underlying);
    }
    else if (parser_is_struct_type(t)) {
        const struct parser_struct *strct = t->strct;
        int count = parser_struct_get_field_count(strct);

        for (int i = 0; i < count; i++) {
            const struct parser_struct_field *field;
            field = parser_get_struct_field(strct, i);
            parser_typelist_push(sb, field->type);
        }
        data_strbuf_push(sb, '.');
    }
}
