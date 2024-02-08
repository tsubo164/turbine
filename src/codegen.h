#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>

struct Bytecode;
struct Module;

void SetOptimize(bool enable);
void ResolveOffset(struct Module *mod);
void GenerateCode(struct Bytecode *code, const struct Module *mod);

#endif // _H
