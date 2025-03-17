#ifndef NATIVE_FUNCTION_H
#define NATIVE_FUNCTION_H

#include "runtime_value.h"

struct parser_struct;
struct parser_scope;
struct parser_type;

enum native_function_result {
    RESULT_NORETURN,
    RESULT_SUCCESS,
    RESULT_FAIL,
};

struct runtime_gc;

struct runtime_registers {
    struct runtime_value *locals;
    struct runtime_value *globals;
    int local_count;
    int global_count;
};

typedef int (*native_func_t)(struct runtime_gc *gc, struct runtime_registers *regs);

/* function */
struct native_func_param {
    const char *name;
    const struct parser_type *type;
    bool is_format;
};

void native_declare_func(struct parser_scope *scope,
        const char *modulename,
        const char *funcname,
        const struct native_func_param *params,
        native_func_t native_func);

/* struct */
struct native_struct_field {
    const char *name;
    const struct parser_type *type;
};

struct parser_struct *native_define_struct(struct parser_scope *scope,
        const char *structname,
        const struct native_struct_field *fields);

/* global */
struct native_global_var {
    const char *name;
    const struct parser_type *type;
    struct runtime_value init_val;
};

void native_define_global_vars(struct parser_scope *scope,
        const struct native_global_var *gvars);

#endif /* _H */
