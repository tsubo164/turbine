#include "parser_type.h"
#include "parser_symbol.h"
#include "data_intern.h"
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

struct parser_type *parser_new_func_type(struct parser_func_sig *func_sig)
{
    struct parser_type *t = new_type(TYP_FUNC);
    t->func_sig = func_sig;
    return t;
}

struct parser_type *parser_new_struct_type(struct parser_struct *s)
{
    struct parser_type *t = new_type(TYP_STRUCT);
    t->strct = s;
    return t;
}

struct parser_type *parser_new_table_type(struct parser_table *tab)
{
    struct parser_type *t = new_type(TYP_TABLE);
    t->table = tab;
    return t;
}

struct parser_type *parser_new_module_type(struct parser_module *mod)
{
    struct parser_type *t = new_type(TYP_MODULE);
    t->module = mod;
    return t;
}

struct parser_type *parser_new_ptr_type(const struct parser_type *underlying)
{
    struct parser_type *t = new_type(TYP_PTR);
    t->underlying = underlying;
    return t;
}

struct parser_type *parser_new_array_type(const struct parser_type *underlying)
{
    struct parser_type *t = new_type(TYP_ARRAY);
    t->underlying = underlying;
    return t;
}

struct parser_type *parser_new_any_type(void)
{
    static struct parser_type t;
    t.kind = TYP_ANY;
    return &t;
}

struct parser_type *parser_new_template_type(int id)
{
    struct parser_type *t = new_type(TYP_TEMPLATE);
    t->template_id = id;
    return t;
}

int parser_sizeof_type(const struct parser_type *t)
{
    if (parser_is_array_type(t))
        return 1;
    else if (parser_is_struct_type(t))
        return t->strct->size;
    else
        return 1;
}

bool parser_is_nil_type(const struct parser_type *t)      { return t->kind == TYP_NIL; }
bool parser_is_bool_type(const struct parser_type *t)     { return t->kind == TYP_BOOL; }
bool parser_is_int_type(const struct parser_type *t)      { return t->kind == TYP_INT; }
bool parser_is_float_type(const struct parser_type *t)    { return t->kind == TYP_FLOAT; }
bool parser_is_string_type(const struct parser_type *t)   { return t->kind == TYP_STRING; }
bool parser_is_func_type(const struct parser_type *t)     { return t->kind == TYP_FUNC; }
bool parser_is_struct_type(const struct parser_type *t)   { return t->kind == TYP_STRUCT; }
bool parser_is_table_type(const struct parser_type *t)    { return t->kind == TYP_TABLE; }
bool parser_is_module_type(const struct parser_type *t)   { return t->kind == TYP_MODULE; }
bool parser_is_ptr_type(const struct parser_type *t)      { return t->kind == TYP_PTR; }
bool parser_is_array_type(const struct parser_type *t)    { return t->kind == TYP_ARRAY; }
bool parser_is_any_type(const struct parser_type *t)      { return t->kind == TYP_ANY; }
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
    case TYP_STRUCT:   return "struct";
    case TYP_TABLE:    return "table";
    case TYP_MODULE:   return "module";
    case TYP_PTR:      return "*";
    case TYP_ARRAY:    return "[]";
    case TYP_ANY:      return "any";
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

        if (type->kind == TYP_ARRAY) {
            sprintf(buf, "[]%s", interned);
        }
        else if (type->kind == TYP_PTR) {
            sprintf(buf, "%s*", interned);
        }
        else if (type->kind == TYP_STRUCT) {
            sprintf(buf, "%s%s", interned, type->strct->name);
        }
        else if (type->kind == TYP_TEMPLATE) {
            sprintf(buf, "%stype%d", interned, type->template_id);
        }
        else {
            sprintf(buf, "%s%s", interned, type_kind_string(type->kind));
        }

        interned = data_string_intern(buf);
    }

    return interned;
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

    return t1->kind == t2->kind;
}

struct parser_type *parser_duplicate_type(const struct parser_type *t)
{
    struct parser_type *dup = new_type(0);
    *dup = *t;
    return dup;
}
