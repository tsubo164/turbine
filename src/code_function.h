#ifndef CODE_FUNC_H
#define CODE_FUNC_H

#include <stdint.h>
#include "native_module.h"

struct code_function {
    int id;
    int argc;
    int reg_count;
    int64_t addr;
    const char *fullname;
    native_func_t native_func_ptr;

    bool is_variadic;
};

struct code_functionvec {
    struct code_function *data;
    int cap;
    int len;
};

int code_push_function(struct code_functionvec *v, const char *fullname, int argc);

struct code_function *code_lookup_function(struct code_functionvec *v, int id);
const struct code_function *code_lookup_const_function(const struct code_functionvec *v,
        int id);

#endif /* _H */
