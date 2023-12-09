#ifndef BYTECODE_H
#define BYTECODE_H

#include <unordered_map>
#include <string_view>
#include <cstdint>
#include <vector>
#include <string>

using Byte = uint8_t;
using Word = uint16_t;
using Int = int64_t;
using Float = double;

enum Opcode {
    OP_NOP = 0,
    // local and arg
    OP_LOADB,
    OP_LOADI,
    OP_LOADF,
    OP_LOADS,
    OP_LOADLOCAL,
    OP_LOADGLOBAL,
    OP_STORELOCAL,
    OP_STOREGLOBAL,
    // jump and function
    OP_ALLOC,
    OP_CALL,
    OP_CALL_BUILTIN,
    OP_RET,
    OP_JMP,
    OP_JEQ,
    // arithmetic
    OP_ADD,
    OP_ADDF,
    OP_ADDS,
    OP_EQ,
    OP_EQF,
    OP_EQS,
    // exit
    OP_EXIT,
    OP_EOC,
};

const char *OpcodeString(Byte op);

class Bytecode {
public:
    Bytecode() {}
    ~Bytecode() {}

    // emit opcode and operand
    void LoadByte(Byte byte);
    void LoadInt(Int integer);
    void LoadFloat(Float fp);
    void LoadString(Word id);
    void LoadLocal(Byte id);
    void LoadGlobal(Word id);
    void StoreLocal(Byte id);
    void StoreGlobal(Word id);
    void AllocateLocal(Byte count);
    void CallFunction(Word func_index, bool builtin);
    // jump instructions return the address
    // where the destination address is stored.
    Int Jump(Int addr);
    Int JumpIfZero(Int addr);
    void Return();
    void AddInt();
    void AddFloat();
    void AddString();
    void EqualInt();
    void EqualFloat();
    void EqualString();
    void Exit();
    void End();
    void BackPatch(Int operand_addr);

    // functions
    Int GetFunctionAddress(Word func_index) const;
    Int GetFunctionArgCount(Word func_index) const;
    void RegisterFunction(Word func_index, Byte argc);
    Int RegisterConstString(std::string_view str);

    const std::string &GetConstString(Word str_index) const;

    // read/write
    Byte Read(Int addr) const;
    Word ReadWord(Int addr) const;
    Int ReadInt(Int addr) const;
    Float ReadFloat(Int addr) const;
    Int Size() const;

    // print
    void Print() const;

private:
    std::vector<Byte> bytes_;
    std::vector<std::string> strings_;

    struct FuncInfo {
        FuncInfo(Word id_, Byte argc_, Int addr_)
            : id(id_), argc(argc_), addr(addr_) {}
        Word id = 0;
        Byte argc = 0;
        Int addr = 0;
    };
    std::vector<FuncInfo> funcs_;
};

#endif // _H
