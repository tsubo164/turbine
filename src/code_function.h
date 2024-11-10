#ifndef CODE_FUNC_H
#define CODE_FUNC_H

#include <stdint.h>

struct code_function {
    int id;
    int argc;
    int reg_count;
    int64_t addr;
};

struct code_functionvec {
    struct code_function *data;
    int cap;
    int len;
};

void code_push_function(struct code_functionvec *v, int id, int argc, int64_t addr);

struct code_function *code_lookup_function(struct code_functionvec *v, int id);

#endif /* _H */
