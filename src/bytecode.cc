#include "bytecode.h"

const char *OpcodeString(Byte op)
{
#define O(op) case op: return #op;
    switch (op) {
    O(OP_NOP);

    O(OP_LOADB);
    O(OP_LOADLOCAL);
    O(OP_ALLOC);

    O(OP_ADD);

    O(OP_EOC);
    default: return "???";
    }
#undef O
}

void Bytecode::LoadByte(Byte byte)
{
    bytes_.push_back(OP_LOADB);
    bytes_.push_back(byte);
}

void Bytecode::LoadLocal(Byte id)
{
    bytes_.push_back(OP_LOADLOCAL);
    bytes_.push_back(id);
}

void Bytecode::AllocateLocal(Byte count)
{
    bytes_.push_back(OP_ALLOC);
    bytes_.push_back(count);
}

void Bytecode::AddInt()
{
    bytes_.push_back(OP_ADD);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);
}

const Byte *Bytecode::Data() const
{
    return &bytes_[0];
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

void Bytecode::Print() const
{
}
