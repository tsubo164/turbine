#ifndef PARSER_SYMBOL_H
#define PARSER_SYMBOL_H

#include <stdbool.h>
#include <stdint.h>

#include "data_hashmap.h"
#include "data_vec.h"
#include "parser_type.h"
#include "runtime_function.h"

/* var */
struct parser_var {
    const char *name;
    const struct parser_type *type;
    int id;
    bool is_global;
    bool is_param;
};

struct parser_varvec {
    struct parser_var **data;
    int cap;
    int len;
};

struct parser_func_sig {
    const struct parser_type *return_type;
    struct parser_typevec param_types;
    bool is_builtin;
    bool is_variadic;
    bool has_special_var;
    bool has_format_param;
    bool has_union_param;
    bool has_template_return_type;
};

struct parser_func {
    const char *name;
    const char *fullname;
    const struct parser_type *return_type;
    struct parser_varvec params;
    int size;
    int id;

    /* TODO consider having struct func_attrib */
    bool is_builtin;
    bool is_variadic;
    bool has_format_param;
    bool has_union_param;
    bool has_special_var;

    struct parser_scope *scope;
    struct parser_stmt *body;
    struct parser_func_sig *func_sig;
    runtime_native_function_t native_func_ptr;
};

struct parser_funcvec {
    struct parser_func **data;
    int cap;
    int len;
};

/* struct */
struct parser_field {
    const char *name;
    const struct parser_type *type;
    int offset;
};

struct parser_fieldvec {
    struct parser_field **data;
    int cap;
    int len;
};

struct parser_struct {
    const char *name;
    struct parser_fieldvec fields;
};

/* table */
struct parser_column {
    const char *name;
    const struct parser_type *type;
    int id;
    int offset;
};

struct parser_columnvec {
    struct parser_column **data;
    int cap;
    int len;
};

/* TODO consider removing */
struct parser_table_row {
    const char *name;
};

struct parser_cell {
    union {
        int64_t ival;
        double fval;
        const char *sval;
    };
};

struct parser_cellvec {
    struct parser_cell *data;
    int cap;
    int len;
};

struct parser_table {
    const char *name;
    struct data_hashmap rows;
    struct parser_columnvec columns;
    struct parser_cellvec cells;
};

/* module */
struct parser_module {
    const char *name;
    struct parser_scope *scope;
    struct parser_stmt* gvars;
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
    SYM_TABLE,
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
/* TODO consider remove this */
void parser_scope_add_symbol(struct parser_scope *sc, struct parser_symbol *sym);

/* symbol */
struct parser_symbol *parser_new_symbol(int kind, const char *name,
        const struct parser_type *type);
struct parser_symbol *parser_find_symbol(const struct parser_scope *sc,
        const char *name);

/* var */
struct parser_symbol *parser_define_var(struct parser_scope *sc, const char *name,
        const struct parser_type *type, bool isglobal);

/* func */
struct parser_func *parser_declare_func(struct parser_scope *parent,
        const char *name, const char *modulename);
struct parser_func *parser_declare_builtin_func(struct parser_scope *parent,
        const char *name);
struct parser_func_sig *parser_make_func_sig(struct parser_func *func);
void parser_declare_param(struct parser_func *f, const char *name,
        const struct parser_type *type);
const struct parser_type *parser_get_param_type(const struct parser_func_sig *func_sig,
        int index);
int parser_required_param_count(const struct parser_func_sig *func_sig);
bool parser_require_type_sequence(const struct parser_func_sig *func_sig);

/* struct */
struct parser_struct *parser_define_struct(struct parser_scope *sc,
        const char *name);
struct parser_struct *parser_find_struct(const struct parser_scope *sc,
        const char *name);
struct parser_field *parser_add_field(struct parser_struct *strct,
        const char *name, const struct parser_type *type);
struct parser_field *parser_find_field(const struct parser_struct *strct,
        const char *name);
int parser_struct_get_field_count(const struct parser_struct *s);
struct parser_field *parser_struct_get_field(const struct parser_struct *s, int idx);

/* table */
struct parser_table *parser_define_table(struct parser_scope *sc,
        const char *name);
struct parser_table *parser_find_table(const struct parser_scope *sc,
        const char *name);
int parser_add_row(struct parser_table *tab, const char *name);
int parser_find_row(const struct parser_table *tab, const char *name);

struct parser_column *parser_add_column(struct parser_table *tab, const char *name);
struct parser_column *parser_find_column(const struct parser_table *tab, const char *name);
struct parser_column *parser_get_column(const struct parser_table *tab, int idx);
int parser_table_get_column_count(const struct parser_table *tab);

int parser_table_get_row_count(const struct parser_table *tab);
void parser_add_cell(struct parser_table *tab, struct parser_cell cell);
struct parser_cell parser_get_enum_field(const struct parser_table *tab, int x, int y);

/* module */
struct parser_module *parser_define_module(struct parser_scope *sc,
        const char *filename, const char *modulename);
/* TODO consider remove this */
void parser_module_add_func(struct parser_module *mod,
        struct parser_func *func);

#endif /* _H */
