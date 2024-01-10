#ifndef CODEGEN_H
#define CODEGEN_H

#include "bytecode.h"
#include "ast.h"

void GenerateCode(Bytecode *code, const Prog *prog);

#endif // _H
