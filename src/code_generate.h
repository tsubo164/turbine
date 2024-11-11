#ifndef CODE_GEN_H
#define CODE_GEN_H

#include <stdbool.h>

struct code_bytecode;
struct parser_module;

void code_resolve_offset(struct parser_module *mod);
void code_generate(struct code_bytecode *code, const struct parser_module *mod);

#endif // _H
