#ifndef CODE_PRINT_H
#define CODE_PRINT_H

#include <stdint.h>

#include "code_bytecode.h"

void code_print_bytecode(const struct code_bytecode *code, bool print_builtin);

void code_print_instruction(const struct code_bytecode *code,
        value_addr_t addr, const struct code_instruction *inst);

#endif /* _H */
