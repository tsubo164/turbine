#include "codegen.h"

void GenerateCode(const Node *tree, Bytecode &code)
{
    tree->Gen(code);
    code.End();
}
