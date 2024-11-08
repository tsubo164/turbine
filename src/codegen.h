#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>

struct Bytecode;
struct parser_module;

void SetOptimize(bool enable);
void ResolveOffset(struct parser_module *mod);
void GenerateCode(struct Bytecode *code, const struct parser_module *mod);

#endif // _H
