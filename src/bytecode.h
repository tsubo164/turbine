#ifndef BYTECODE_H
#define BYTECODE_H

#include <unordered_map>
#include <cstdint>
#include <vector>
#include "string_table.h"

using Byte = uint8_t;
using Word = uint16_t;
using Int = int64_t;
using Float = double;

enum Opcode {
    OP_NOP = 0,

    OP_LOADB,
    OP_LOADI,
    OP_LOADLOCAL,
    OP_LOADARG,
    OP_STORELOCAL,

    OP_ALLOC,
    OP_CALL,
    OP_RET,
    OP_JMP,
    OP_JEQ,

    OP_ADD,
    OP_EQ,

    OP_EXIT,
    OP_EOC,
};

class Bytecode {
public:
    Bytecode() {}
    ~Bytecode() {}

    void LoadByte(Byte byte);
    void LoadInt(Int integer);
    void LoadLocal(Byte id);
    void LoadArgument(Byte id);
    void StoreLocal(Byte id);
    void AllocateLocal(Byte count);
    void CallFunction(Int label);
    // jump instructions return the address
    // where the destination address is stored.
    Int Jump(Int addr);
    Int JumpIfZero(Int addr);
    void Label(Int label);
    void Return(Byte argc);
    void AddInt();
    void EqualInt();
    void Exit();
    void End();
    void BackPatch(Int operand_addr);

    const Byte *Data() const;
    Int Read(Int addr) const;
    Int ReadWord(Int addr) const;
    Int Size() const;

    void Print() const;

private:
    std::vector<Byte> bytes_;
    std::unordered_map<Int,Int> label_to_addr_;
    struct Patch {
        Patch(Int a, Int l) : addr(a), label(l) {}
        Int addr = 0;  // address to patch
        Int label = 0; // label to jump to
    };
    std::vector<Patch> backpatch_addr_;
};

const char *OpcodeString(Byte op);

#endif // _H
