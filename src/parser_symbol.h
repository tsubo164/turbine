#ifndef PARSER_SCOPE_H
#define PARSER_SCOPE_H

#include <stdbool.h>
#include <stdint.h>

#include "data_hashmap.h"
#include "data_vec.h"

/* TODO make specific vec structs */
struct Vec {
    void **data;
    int cap;
    int len;
};

void VecPush(struct Vec *v, void *data);
void VecFree(struct Vec *v);
/* ------------------------------ */

struct parser_type;

struct parser_var {
    const char *name;
    const struct parser_type *type;
    int offset;
    bool is_global;
    bool is_param;
};

/* TODO consider parser_func_signature */
struct parser_func_type {
    const struct parser_type *return_type;
    struct Vec param_types;
    bool is_builtin;
    bool is_variadic;
    bool has_special_var;
};

struct parser_func {
    const char *name;
    const char *fullname;
    const struct parser_type *return_type;
    struct Vec params;
    int size;
    int id;

    bool is_builtin;
    bool is_variadic;
    bool has_special_var;

    struct parser_scope *scope;
    struct parser_stmt *body;
    struct parser_func_type *func_type;
};

struct parser_field {
    const char *name;
    const struct parser_type *type;
    int offset;
};

struct parser_struct {
    const char *name;
    struct Vec fields;
    int size;
};

struct parser_table_row {
    const char *name;
    union {
        int64_t ival;
        double fval;
        const char *sval;
    };
};

struct parser_table {
    const char *name;
    struct data_hashmap rows;
};

struct parser_module {
    const char *name;
    struct parser_scope *scope;
    struct parser_stmt* gvars;
    struct Vec funcs;

    const char *filename;
    const char *src;
    /* TODO remove this */
    const struct parser_func *main_func;
};

enum parser_symbol_kind {
    SYM_VAR,
    SYM_FUNC,
    SYM_TABLE,
    SYM_STRUCT,
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
        struct parser_table *table;
        struct parser_module *module;
        struct parser_scope *scope;
    };
};

struct parser_scope {
    struct parser_scope *parent;
    int size;

    struct Vec syms;
    struct data_hashmap symbols;
};

/* scope */
struct parser_scope *parser_new_scope(struct parser_scope *parent);

/* symbol */
struct parser_symbol *parser_new_symbol(int kind, const char *name, const struct parser_type *type);
struct parser_symbol *parser_find_symbol(const struct parser_scope *sc, const char *name);

/* var */
struct parser_symbol *parser_define_var(struct parser_scope *sc, const char *name,
        const struct parser_type *type, bool isglobal);

/* func */
struct parser_func *parser_declare_func(struct parser_scope *parent, const char *name, const char *modulefile);
struct parser_func *parser_declare_builtin_func(struct parser_scope *parent, const char *name);
struct parser_func_type *parser_make_func_type(struct parser_func *func);
void parser_declare_param(struct parser_func *f, const char *name, const struct parser_type *type);
const struct parser_type *parser_get_param_type(const struct parser_func_type *func_type, int index);
int parser_required_param_count(const struct parser_func_type *func_type);

/* struct */
struct parser_struct *parser_define_struct(struct parser_scope *sc, const char *name);
struct parser_struct *parser_find_struct(const struct parser_scope *sc, const char *name);
struct parser_field *parser_add_field(struct parser_struct *strct, const char *name, const struct parser_type *type);
struct parser_field *parser_find_field(const struct parser_struct *strct, const char *name);
int parser_struct_get_field_count(const struct parser_struct *s);

/* table */
struct parser_table *parser_define_table(struct parser_scope *sc, const char *name);

/* module */
struct parser_module *parser_define_module(struct parser_scope *sc, const char *filename, const char *modulename);

#endif /* _H */
