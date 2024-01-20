#ifndef CODEGEN_H
#define CODEGEN_H

#include <stdbool.h>

struct Bytecode;
struct Prog;

void SetOptimize(bool enable);
void GenerateCode(struct Bytecode *code, const struct Prog *prog);

#endif // _H
