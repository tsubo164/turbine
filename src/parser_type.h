#ifndef PARSER_TYPE_H
#define PARSER_TYPE_H

#include <stdbool.h>

struct parser_func_sig;
struct parser_struct;
struct parser_enum;
struct parser_module;

enum parser_type_kind {
    TYP_NIL,
    TYP_BOOL,
    TYP_INT,
    TYP_FLOAT,
    TYP_STRING,
    TYP_FUNC,
    TYP_ARRAY,
    TYP_MAP,
    TYP_SET,
    TYP_STRUCT,
    TYP_ENUM,
    TYP_MODULE,
    TYP_ANY,
    TYP_UNION,
    TYP_TEMPLATE,
};

struct parser_type;

struct parser_typevec {
    const struct parser_type **data;
    int cap;
    int len;
};

struct parser_type {
    int kind;

    /* TODO move to union after removing pointer */
    const struct parser_type *underlying;
    struct parser_typevec unions;

    union {
        const struct parser_func_sig *func_sig;
        const struct parser_struct *strct;
        const struct parser_enum *enm;
        const struct parser_module *module;
        int template_id;
    };
};

struct parser_type *parser_new_nil_type(void);
struct parser_type *parser_new_bool_type(void);
struct parser_type *parser_new_int_type(void);
struct parser_type *parser_new_float_type(void);
struct parser_type *parser_new_string_type(void);
struct parser_type *parser_new_func_type(const struct parser_func_sig *func_sig);
struct parser_type *parser_new_array_type(const struct parser_type *underlying);
struct parser_type *parser_new_map_type(const struct parser_type *underlying);
struct parser_type *parser_new_set_type(const struct parser_type *underlying);
struct parser_type *parser_new_struct_type(const struct parser_struct *s);
struct parser_type *parser_new_enum_type(const struct parser_enum *e);
struct parser_type *parser_new_module_type(const struct parser_module *m);
struct parser_type *parser_new_any_type(void);
struct parser_type *parser_new_union_type(int id);
struct parser_type *parser_new_template_type(int id);

bool parser_is_nil_type(const struct parser_type *t);
bool parser_is_bool_type(const struct parser_type *t);
bool parser_is_int_type(const struct parser_type *t);
bool parser_is_float_type(const struct parser_type *t);
bool parser_is_string_type(const struct parser_type *t);
bool parser_is_func_type(const struct parser_type *t);
bool parser_is_array_type(const struct parser_type *t);
bool parser_is_map_type(const struct parser_type *t);
bool parser_is_set_type(const struct parser_type *t);
bool parser_is_struct_type(const struct parser_type *t);
bool parser_is_enum_type(const struct parser_type *t);
bool parser_is_module_type(const struct parser_type *t);
bool parser_is_any_type(const struct parser_type *t);
bool parser_is_union_type(const struct parser_type *t);
bool parser_is_template_type(const struct parser_type *t);
bool parser_has_template_type(const struct parser_type *t);

bool parser_match_type(const struct parser_type *t1, const struct parser_type *t2);
struct parser_type *parser_duplicate_type(const struct parser_type *t);
const char *parser_type_string(const struct parser_type *t);

void parser_add_union_type(struct parser_type *uni, const struct parser_type *t);

/* type vec */
void parser_typevec_push(struct parser_typevec *v, const struct parser_type *val);

/* type list is an ordered list packed into a string */
struct data_strbuf;

struct parser_typelist_iterator {
    const char *curr;
    enum parser_type_kind kind;
};

void parser_typelist_begin(struct parser_typelist_iterator *it, const char *typelist);
int parser_typelist_next(struct parser_typelist_iterator *it);
bool parser_typelist_end(const struct parser_typelist_iterator *it);
bool parser_typelist_struct_end(const struct parser_typelist_iterator *it);
void parser_typelist_push(struct data_strbuf *sb, const struct parser_type *t);

#endif /* _H */
