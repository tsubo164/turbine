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

#define BYTECODE_LIST \
    /* OPCODE        OPERAND_SIZE */\
    /* ========================== */\
    OP(OP_NOP,          OPERAND_NONE) \
    /* local and arg */\
    OP(OP_LOADB,        OPERAND_BYTE) \
    OP(OP_LOADI,        OPERAND_QUAD) \
    OP(OP_LOADF,        OPERAND_QUAD) \
    OP(OP_LOADS,        OPERAND_WORD) \
    OP(OP_LOADLOCAL,    OPERAND_BYTE) \
    OP(OP_LOADGLOBAL,   OPERAND_WORD) \
    OP(OP_STORELOCAL,   OPERAND_BYTE) \
    OP(OP_STOREGLOBAL,  OPERAND_WORD) \
    OP(OP_INCLOCAL,     OPERAND_BYTE) \
    OP(OP_INCGLOBAL,    OPERAND_WORD) \
    /* jump and function */\
    OP(OP_ALLOC,        OPERAND_BYTE) \
    OP(OP_CALL,         OPERAND_WORD) \
    OP(OP_CALL_BUILTIN, OPERAND_BYTE) \
    OP(OP_RET,          OPERAND_NONE) \
    OP(OP_JMP,          OPERAND_WORD) \
    OP(OP_JEQ,          OPERAND_WORD) \
    /* arithmetic */\
    OP(OP_ADD,          OPERAND_NONE) \
    OP(OP_ADDF,         OPERAND_NONE) \
    OP(OP_ADDS,         OPERAND_NONE) \
    OP(OP_EQ,           OPERAND_NONE) \
    OP(OP_EQF,          OPERAND_NONE) \
    OP(OP_EQS,          OPERAND_NONE) \
    OP(OP_NEQ,          OPERAND_NONE) \
    OP(OP_NEQF,         OPERAND_NONE) \
    OP(OP_NEQS,         OPERAND_NONE) \
    /* exit */\
    OP(OP_EXIT,         OPERAND_NONE) \
    OP(OP_EOC,          OPERAND_NONE) \

enum Opcode {
#define OP(opcode, operand_size) opcode,
    BYTECODE_LIST
#undef OP
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
    void IncLocal(Byte id);
    void IncGlobal(Word id);
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
    void NotEqualInt();
    void NotEqualFloat();
    void NotEqualString();
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

    Int print_op(int op, int operand, Int address) const;
};

#endif // _H
