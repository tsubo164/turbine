#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <stdbool.h>
#include <stdint.h>

struct interpreter_args {
    const char *filename;
    const char **values;
    int count;
};

struct interpreter_option {
    bool print_token;
    bool print_token_raw;
    bool print_tree;
    bool print_symbols;
    bool print_symbols_all;
    bool print_bytecode;
    bool print_stack;
};

int64_t interpret_source(const char *text, const struct interpreter_args *args,
        const struct interpreter_option *opt);

#endif /* _H */
