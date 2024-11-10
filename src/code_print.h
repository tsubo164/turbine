#ifndef CODE_PRINT_H
#define CODE_PRINT_H

#include <stdint.h>

#include "code_bytecode.h"

void code_print_bytecode(const struct code_bytecode *code);

void code_print_instruction(const struct code_bytecode *code,
        int64_t addr, const struct code_instruction *inst, int *imm_size);

#endif /* _H */
