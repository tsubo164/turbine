#ifndef NATIVE_FUNCTION_H
#define NATIVE_FUNCTION_H

#include "runtime_value.h"

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

#endif /* _H */
