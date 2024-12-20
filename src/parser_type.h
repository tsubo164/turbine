#ifndef PARSER_TYPE_H
#define PARSER_TYPE_H

#include <stdbool.h>

struct parser_func_sig;
struct parser_struct;
struct parser_table;
struct parser_module;

enum parser_type_kind {
    TYP_NIL,
    TYP_BOOL,
    TYP_INT,
    TYP_FLOAT,
    TYP_STRING,
    TYP_FUNC,
    TYP_STRUCT,
    TYP_TABLE,
    TYP_MODULE,
    TYP_PTR,
    TYP_ARRAY,
    TYP_ANY,
    TYP_TEMPLATE,
};

struct parser_type {
    int kind;
    const struct parser_type *underlying;

    union {
        const struct parser_func_sig *func_sig;
        const struct parser_struct *strct;
        const struct parser_table *table;
        const struct parser_module *module;
        int template_id;
    };
};

struct parser_type *parser_new_nil_type(void);
struct parser_type *parser_new_bool_type(void);
struct parser_type *parser_new_int_type(void);
struct parser_type *parser_new_float_type(void);
struct parser_type *parser_new_string_type(void);
struct parser_type *parser_new_func_type(struct parser_func_sig *func_sig);
struct parser_type *parser_new_struct_type(struct parser_struct *s);
struct parser_type *parser_new_table_type(struct parser_table *t);
struct parser_type *parser_new_module_type(struct parser_module *m);
struct parser_type *parser_new_ptr_type(const struct parser_type *underlying);
struct parser_type *parser_new_array_type(const struct parser_type *underlying);
struct parser_type *parser_new_any_type(void);
struct parser_type *parser_new_template_type(int id);

bool parser_is_nil_type(const struct parser_type *t);
bool parser_is_bool_type(const struct parser_type *t);
bool parser_is_int_type(const struct parser_type *t);
bool parser_is_float_type(const struct parser_type *t);
bool parser_is_string_type(const struct parser_type *t);
bool parser_is_func_type(const struct parser_type *t);
bool parser_is_struct_type(const struct parser_type *t);
bool parser_is_table_type(const struct parser_type *t);
bool parser_is_module_type(const struct parser_type *t);
bool parser_is_ptr_type(const struct parser_type *t);
bool parser_is_array_type(const struct parser_type *t);
bool parser_is_any_type(const struct parser_type *t);
bool parser_is_template_type(const struct parser_type *t);

int parser_sizeof_type(const struct parser_type *t);
bool parser_match_type(const struct parser_type *t1, const struct parser_type *t2);
struct parser_type *parser_duplicate_type(const struct parser_type *t);
const char *parser_type_string(const struct parser_type *t);

#endif /* _H */
