#ifndef PARSER_SYMBOL_H
#define PARSER_SYMBOL_H

#include <stdbool.h>

#include "native_module.h"
#include "value_types.h"
#include "parser_type.h"
#include "data_hashmap.h"
#include "data_vec.h"

/* var */
struct parser_var {
    const char *name;
    const struct parser_type *type;
    int offset;
    bool is_global;
    bool is_param;
    bool is_out;
};

struct parser_func_sig {
    const struct parser_type *return_type;
    struct parser_typevec param_types;

    bool is_native;
    bool is_variadic;
    bool has_special_var;
    bool has_format_param;
    bool has_union_param;
    bool has_template_return_type;
};

struct parser_func {
    const char *name;
    const char *fullname;
    struct parser_func_sig *sig;
    int size;
    int id;

    struct parser_scope *scope;
    struct parser_stmt *body;
    native_func_t native_func_ptr;
};

struct parser_funcvec {
    struct parser_func **data;
    int cap;
    int len;
};

/* struct */
struct parser_struct_field {
    const char *name;
    const struct parser_type *type;
    /* TODO remove offset? */
    int offset;
};

struct parser_struct_fieldvec {
    struct parser_struct_field **data;
    int cap;
    int len;
};

struct parser_struct {
    const char *name;
    struct parser_struct_fieldvec fields;
    int id;
};

/* enum */
struct parser_enum_field {
    const char *name;
    const struct parser_type *type;
    int id;
    int offset;
};

struct parser_enum_fieldvec {
    struct parser_enum_field **data;
    int cap;
    int len;
};

struct parser_enum_value {
    union {
        value_int_t ival;
        value_float_t fval;
        const char *sval;
    };
};

struct parser_exprvec {
    struct parser_expr **data;
    int cap;
    int len;
};

struct parser_enum {
    const char *name;
    struct data_hashmap members;
    struct parser_enum_fieldvec fields;
    struct parser_exprvec valueexprs;

    int member_begin;
    int member_end;
};

/* module */
struct parser_module {
    const char *name;
    struct parser_scope *scope;
    struct parser_stmt* gvars;
    /* for iteration in added order */
    struct parser_funcvec funcs;

    const char *filename;
    const char *src;
    /* TODO remove this */
    const struct parser_func *main_func;
};

/* symbol */
enum parser_symbol_kind {
    SYM_VAR,
    SYM_FUNC,
    SYM_STRUCT,
    SYM_ENUM,
    SYM_MODULE,
    SYM_SCOPE,
};

struct parser_symbol {
    int kind;
    int id;
    const char *name;
    const struct parser_type *type;

    union {
        struct parser_var *var;
        struct parser_func *func;
        struct parser_struct *strct;
        struct parser_enum *enm;
        struct parser_module *module;
        struct parser_scope *scope;
    };
};

struct parser_symbolvec {
    struct parser_symbol **data;
    int cap;
    int len;
};

/* scope */
struct parser_scope {
    struct parser_scope *parent;
    int size;

    struct parser_symbolvec syms;
    struct data_hashmap symbols;
};

/* scope */
struct parser_scope *parser_new_scope(struct parser_scope *parent);
void parser_free_scope(struct parser_scope *sc);
/* TODO consider remove this */
void parser_scope_add_symbol(struct parser_scope *sc, struct parser_symbol *sym);

/* symbol */
struct parser_symbol *parser_new_symbol(int kind, const char *name,
        const struct parser_type *type);
struct parser_symbol *parser_find_symbol(const struct parser_scope *sc,
        const char *name);
struct parser_symbol *parser_find_symbol_local(const struct parser_scope *sc,
        const char *name);

/* var */
struct parser_var *parser_define_var(struct parser_scope *sc, const char *name,
        const struct parser_type *type, bool isglobal);

/* func */
struct parser_func *parser_declare_func(struct parser_scope *parent,
        const char *modulename, const char *name);
struct parser_func *parser_declare_native_func(struct parser_scope *parent,
        const char *modulename, const char *name, native_func_t func_ptr);

void parser_declare_param(struct parser_func *func, const char *name,
        const struct parser_type *type, bool is_out);
void parser_add_return_type(struct parser_func *func, const struct parser_type *type);

const struct parser_type *parser_get_param_type(const struct parser_func_sig *func_sig,
        int index);
int parser_required_param_count(const struct parser_func_sig *func_sig);
bool parser_require_type_sequence(const struct parser_func_sig *func_sig);
bool parser_match_func_signature(const struct parser_func_sig *sig1,
        const struct parser_func_sig *sig2);

/* struct */
struct parser_struct *parser_define_struct(struct parser_scope *sc,
        const char *name);
struct parser_struct *parser_find_struct(const struct parser_scope *sc,
        const char *name);
struct parser_struct_field *parser_add_struct_field(struct parser_struct *strct,
        const char *name, const struct parser_type *type);
struct parser_struct_field *parser_find_struct_field(const struct parser_struct *strct,
        const char *name);
int parser_struct_get_field_count(const struct parser_struct *s);
struct parser_struct_field *parser_get_struct_field(const struct parser_struct *s, int idx);

/* enum */
struct parser_enum *parser_define_enum(struct parser_scope *sc,
        const char *name);
struct parser_enum *parser_find_enum(const struct parser_scope *sc,
        const char *name);

int parser_add_enum_member(struct parser_enum *enm, const char *name);
int parser_find_enum_member(const struct parser_enum *enm, const char *name);
int parser_get_enum_member_count(const struct parser_enum *enm);

struct parser_enum_field *parser_add_enum_field(struct parser_enum *enm, const char *name);
struct parser_enum_field *parser_find_enum_field(const struct parser_enum *enm, const char *name);
struct parser_enum_field *parser_get_enum_field(const struct parser_enum *enm, int idx);
int parser_get_enum_field_count(const struct parser_enum *enm);

void parser_add_enum_value_expr(struct parser_enum *enm, struct parser_expr *e);
struct parser_enum_value parser_get_enum_value(const struct parser_enum *enm, int x, int y);

/* module */
struct parser_module *parser_define_module(struct parser_scope *sc,
        const char *filename, const char *modulename);
void parser_free_module(struct parser_module *mod);

/* TODO consider remove this */
void parser_module_add_func(struct parser_module *mod,
        struct parser_func *func);

#endif /* _H */
