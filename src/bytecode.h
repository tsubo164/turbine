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
    // jump instructions return the index of bytes
    // where the destination index is stored.
    Int Jump(Int index);
    Int JumpIfZero(Int index);
    void Label(Int label);
    void Return(Byte argc);
    void AddInt();
    void EqualInt();
    void Exit();
    void End();
    void BackPatch(Int operand_index);

    const Byte *Data() const;
    Int Read(Int index) const;
    Int ReadWord(Int index) const;
    Int Size() const;

    void Print() const;
private:
    std::vector<Byte> bytes_;
    std::unordered_map<Int,Int> label_to_index_;
    std::unordered_map<Int,Int> backpatch_index_;
};

const char *OpcodeString(Byte op);

#endif // _H
