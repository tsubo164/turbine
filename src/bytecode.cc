#include "bytecode.h"

Bytecode::Bytecode()
{
}

Bytecode::~Bytecode()
{
}

void Bytecode::LoadByte(Byte operand)
{
    bytes_.push_back(OP_LOADB);
    bytes_.push_back(operand);
}

void Bytecode::AddInt()
{
    bytes_.push_back(OP_ADD);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);
}

Byte Bytecode::Read(Int index) const
{
    if (index < 0 || index >= Size())
        return OP_NOP;

    return bytes_[index];
}

Int Bytecode::Size() const
{
    return bytes_.size();
}
