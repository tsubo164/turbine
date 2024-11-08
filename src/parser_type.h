#ifndef PARSER_TYPE_H
#define PARSER_TYPE_H

#include <stdbool.h>

struct FuncType;
struct Struct;
struct Table;
struct Module;

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
};

struct parser_type {
    int kind;
    const struct parser_type *underlying;

    union {
        const struct FuncType *func_type;
        const struct Struct *strct;
        const struct Table *table;
        const struct Module *module;
        int len;
    };
};

struct parser_type *parser_new_nil_type(void);
struct parser_type *parser_new_bool_type(void);
struct parser_type *parser_new_int_type(void);
struct parser_type *parser_new_float_type(void);
struct parser_type *parser_new_string_type(void);
struct parser_type *parser_new_func_type(struct FuncType *func_type);
struct parser_type *parser_new_struct_type(struct Struct *s);
struct parser_type *parser_new_table_type(struct Table *t);
struct parser_type *parser_new_module_type(struct Module *m);
struct parser_type *parser_new_ptr_type(const struct parser_type *underlying);
struct parser_type *parser_new_array_type(int len, const struct parser_type *underlying);
struct parser_type *parser_new_any_type(void);

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

int parser_sizeof_type(const struct parser_type *t);
bool parser_match_type(const struct parser_type *t1, const struct parser_type *t2);
struct parser_type *parser_duplicate_type(const struct parser_type *t);
const char *parser_type_string(const struct parser_type *t);

#endif /* _H */
