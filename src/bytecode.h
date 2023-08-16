#ifndef BYTECODE_H
#define BYTECODE_H

#include <cstdint>
#include <vector>

using Byte = uint8_t;
using Int = int64_t;
using Float = double;

enum Opcode {
    OP_NOP = 0,
    OP_LOADB,
    OP_LOADLOCAL,
    OP_ALLOC,
    OP_ADD,
    OP_EOC,
};

class Bytecode {
public:
    Bytecode() {}
    ~Bytecode() {}

    void LoadByte(Byte byte);
    void LoadLocal(Byte id);
    void AllocateLocal(Byte count);
    void AddInt();
    void End();

    const Byte *Data() const;
    Byte Read(Int index) const;
    Int Size() const;

    void Print() const;
private:
    std::vector<Byte> bytes_;
};

const char *OpcodeString(Byte op);

#endif // _H
