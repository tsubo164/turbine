#ifndef NATIVE_FUNCTION_H
#define NATIVE_FUNCTION_H

#include "runtime_value.h"

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

typedef int (*native_function_t)(struct runtime_gc *gc,
        struct runtime_registers *regs);

#endif /* _H */
