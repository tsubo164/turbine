#include "bytecode.h"
#include <iostream>
#include <iomanip>
#include <cstring>

const char *OpcodeString(Byte op)
{
#define O(op) case op: return #op;
    switch (op) {
    O(OP_NOP);

    O(OP_LOADB);
    O(OP_LOADI);
    O(OP_LOADLOCAL);
    O(OP_LOADARG);
    O(OP_STORELOCAL);

    O(OP_ALLOC);
    O(OP_CALL);
    O(OP_RET);
    O(OP_JMP);
    O(OP_JEQ);

    O(OP_ADD);
    O(OP_EQ);

    O(OP_EXIT);
    O(OP_EOC);
    default:
        std::cerr << "Opcode: " << static_cast<int>(op)
            << " not in OpcodeString()" << std::endl;
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
#undef O
}

template<typename T>
void push_back(std::vector<Byte> &bytes, T operand)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    std::memcpy(buf, &operand, SIZE);

    for (int i = 0; i < SIZE; i++)
        bytes.push_back(buf[i]);
}

template<typename T>
void write(std::vector<Byte> &bytes, Int addr, T operand)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    std::memcpy(buf, &operand, SIZE);

    for (int i = 0; i < SIZE; i++)
        bytes[addr + i] = buf[i];
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

void Bytecode::LoadArgument(Byte id)
{
    bytes_.push_back(OP_LOADARG);
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

void Bytecode::CallFunction(Int label)
{
    Int addr = -1;
    const auto it = label_to_addr_.find(label);
    if (it != label_to_addr_.end()) {
        addr = it->second;
    }
    else {
        // backpatch to the next to call instruction
        const Int next_addr = bytes_.size() + 1;
        backpatch_addr_.emplace_back(next_addr, label);
    }

    bytes_.push_back(OP_CALL);
    push_back<Word>(bytes_, addr);
}

Int Bytecode::JumpIfZero(Int addr)
{
    bytes_.push_back(OP_JEQ);
    const Int operand_addr = bytes_.size();
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

Int Bytecode::Jump(Int addr)
{
    bytes_.push_back(OP_JMP);
    const Int operand_addr = bytes_.size();
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

void Bytecode::Label(Int label)
{
    const auto it = label_to_addr_.find(label);
    if (it != label_to_addr_.end()) {
        std::cerr << "error: re-defined label: " << label << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const Int next_addr = bytes_.size();
    label_to_addr_.insert({label, next_addr});
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

void Bytecode::Exit()
{
    bytes_.push_back(OP_EXIT);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);

    for (const auto &patch: backpatch_addr_) {
        const Int func_addr = label_to_addr_[patch.label];
        write<Word>(bytes_, patch.addr, func_addr);
    }
}

void Bytecode::BackPatch(Int operand_addr)
{
    Int current_addr = bytes_.size();

    write<Word>(bytes_, operand_addr, current_addr);
}

const Byte *Bytecode::Data() const
{
    return &bytes_[0];
}

Int Bytecode::Read(Int addr) const
{
    if (addr < 0 || addr >= Size())
        return OP_NOP;

    return bytes_[addr];
}

Int Bytecode::ReadWord(Int addr) const
{
    if (addr < 0 || addr >= Size())
        return 0;

    constexpr int SIZE = sizeof(Word);
    Byte buf[SIZE] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = static_cast<Byte>(Read(addr + i));

    Word ret = 0;
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

Int Bytecode::Size() const
{
    return bytes_.size();
}

void Bytecode::Print() const
{
    bool brk = false;
    int addr = 0;

    while (addr < Size() && !brk) {
        std::cout << "[" << std::setw(6) << addr << "] ";

        const int op = Read(addr++);

        switch (op) {
        case OP_NOP:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_LOADB:
            std::cout << OpcodeString(op) << " $" << Read(addr++) << std::endl;
            break;

        case OP_LOADLOCAL:
            std::cout << OpcodeString(op) << " @" << Read(addr++) << std::endl;
            break;

        case OP_LOADARG:
            std::cout << OpcodeString(op) << " @" << Read(addr++) << std::endl;
            break;

        case OP_STORELOCAL:
            std::cout << OpcodeString(op) << " @" << Read(addr++) << std::endl;
            break;

        case OP_ALLOC:
            std::cout << OpcodeString(op) << " $" << Read(addr++) << std::endl;
            break;

        case OP_CALL:
            std::cout << OpcodeString(op) << " $" << ReadWord(addr) << std::endl;
            addr += 2;
            break;

        case OP_RET:
            std::cout << OpcodeString(op) << " $" << Read(addr++) << std::endl;
            break;

        case OP_JMP:
            std::cout << OpcodeString(op) << " $" << ReadWord(addr) << std::endl;
            addr += 2;
            break;

        case OP_JEQ:
            std::cout << OpcodeString(op) << " $" << ReadWord(addr) << std::endl;
            addr += 2;
            break;

        case OP_ADD:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_EQ:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_EXIT:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_EOC:
            std::cout << OpcodeString(op) << std::endl;
            brk = true;
            break;

        default:
            std::cerr << "Opcode: " << " not in Bytecode::Print()" << std::endl;
            std::exit(EXIT_FAILURE);
            break;
        }
    }
}
