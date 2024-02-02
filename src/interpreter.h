#ifndef INTERPRETER_H
#define INTERPRETER_H

#include "bytecode.h"

typedef struct Option {
    bool print_token;
    bool print_token_raw;
    bool print_tree;
    bool print_symbols;
    bool print_symbols_all;
    bool print_bytecode;
    bool print_stack;
    bool enable_optimize;
} Option;

Int Interpret(const char *src, const char *filename, const Option *opt);

#endif // _H
