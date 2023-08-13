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
    OP_ADD,
    OP_EOC,
};

class Bytecode {
public:
	Bytecode();
	~Bytecode();

	void LoadByte(Byte operand);
	void AddInt();
	void End();

	Byte Read(Int index) const;
	Int Size() const;

private:
	std::vector<Byte> bytes_;
};

#endif // _H
