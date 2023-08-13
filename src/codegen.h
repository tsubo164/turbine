#ifndef CODEGEN_H
#define CODEGEN_H

#include "bytecode.h"
#include "ast.h"

void GenerateCode(const Node *tree, Bytecode &code);

#endif // _H
