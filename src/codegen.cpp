#include "codegen.h"

void GenerateCode(Bytecode *code, const Prog *prog)
{
    gen_prog(code, prog);
    code->End();
}
