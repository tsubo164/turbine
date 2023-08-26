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
    void CallFunction(SharedStr name);
    void Label(SharedStr name);
    void Return();
    void AddInt();
    void EqualInt();
    void Exit();
    void End();

    const Byte *Data() const;
    Int Read(Int index) const;
    Int ReadWord(Int index) const;
    Int Size() const;

    void Print() const;
private:
    std::vector<Byte> bytes_;
    std::unordered_map<SharedStr,Int> name_to_index_;
    std::unordered_map<Int,SharedStr> backpatch_index_;
};

const char *OpcodeString(Byte op);

#endif // _H
