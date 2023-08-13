#include "codegen.h"

static void gen(const Node *node, Bytecode &code)
{
    switch (node->kind) {

    case NOD_INTNUM:
        code.LoadByte(node->ival);
        break;

    case NOD_ADD:
        gen(node->lhs, code);
        gen(node->rhs, code);
        code.AddInt();
        break;

    default:
        break;
    }
}

void GenerateCode(const Node *tree, Bytecode &code)
{
    gen(tree, code);
    code.End();
}
