#ifndef RUNTIME_FUNCTION_H
#define RUNTIME_FUNCTION_H

#include "runtime_value.h"

enum runtime_function_result {
    RESULT_NORETURN,
    RESULT_SUCCESS,
    RESULT_FAIL,
};

struct runtime_gc;

typedef int (*runtime_native_function_t)(struct runtime_gc *gc,
        struct runtime_value *registers, int reg_count);

#endif /* _H */
