#include "bytecode.h"
#include <iostream>
#include <cstring>

const char *OpcodeString(Byte op)
{
#define O(op) case op: return #op;
    switch (op) {
    O(OP_NOP);

    O(OP_LOADB);
    O(OP_LOADI);
    O(OP_LOADLOCAL);
    O(OP_STORELOCAL);
    O(OP_ALLOC);
    O(OP_RET);

    O(OP_ADD);
    O(OP_EQ);

    O(OP_EOC);
    default: return "???";
    }
#undef O
}

template<typename T>
void push_back(std::vector<Byte> &bytes, T operand)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    std::memcpy(buf, &operand, SIZE);

    for ( int i = 0; i < SIZE; i++ )
        bytes.push_back(buf[i]);
}

void Bytecode::LoadByte(Byte byte)
{
    bytes_.push_back(OP_LOADB);
    bytes_.push_back(byte);
}

void Bytecode::LoadInt(Int integer)
{
    bytes_.push_back(OP_LOADI);
    push_back<Int>(bytes_, integer);
}

void Bytecode::LoadLocal(Byte id)
{
    bytes_.push_back(OP_LOADLOCAL);
    bytes_.push_back(id);
}

void Bytecode::StoreLocal(Byte id)
{
    bytes_.push_back(OP_STORELOCAL);
    bytes_.push_back(id);
}

void Bytecode::AllocateLocal(Byte count)
{
    bytes_.push_back(OP_ALLOC);
    bytes_.push_back(count);
}

void Bytecode::Return(Byte argc)
{
    bytes_.push_back(OP_RET);
    bytes_.push_back(argc);
}

void Bytecode::AddInt()
{
    bytes_.push_back(OP_ADD);
}

void Bytecode::EqualInt()
{
    bytes_.push_back(OP_EQ);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);
}

const Byte *Bytecode::Data() const
{
    return &bytes_[0];
}

Int Bytecode::Read(Int index) const
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
    bool brk = false;
    int index = 0;

    while (index < Size() && !brk) {
        const int op = Read(index++);

        switch (op) {
        case OP_NOP:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_LOADB:
            std::cout << OpcodeString(op) << " $" << Read(index++) << std::endl;
            break;

        case OP_LOADLOCAL:
            std::cout << OpcodeString(op) << " @" << Read(index++) << std::endl;
            break;

        case OP_STORELOCAL:
            std::cout << OpcodeString(op) << " @" << Read(index++) << std::endl;
            break;

        case OP_ALLOC:
            std::cout << OpcodeString(op) << " $" << Read(index++) << std::endl;
            break;

        case OP_RET:
            std::cout << OpcodeString(op) << " $" << Read(index++) << std::endl;
            break;

        case OP_ADD:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_EOC:
            std::cout << OpcodeString(op) << std::endl;
            brk = true;
            break;

        default:
            break;
        }
    }
}
