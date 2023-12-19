#ifndef BYTECODE_H
#define BYTECODE_H

#include <string_view>
#include <cstdint>
#include <vector>
#include <string>
#include <stack>

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
    OP(OP_DECLOCAL,     OPERAND_BYTE) \
    OP(OP_DECGLOBAL,    OPERAND_WORD) \
    /* arg type spec */\
    OP(OP_LOADTYPEI,    OPERAND_NONE) \
    OP(OP_LOADTYPEF,    OPERAND_NONE) \
    OP(OP_LOADTYPES,    OPERAND_NONE) \
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
    OP(OP_CATS,         OPERAND_NONE) \
    OP(OP_SUB,          OPERAND_NONE) \
    OP(OP_SUBF,         OPERAND_NONE) \
    OP(OP_MUL,          OPERAND_NONE) \
    OP(OP_MULF,         OPERAND_NONE) \
    OP(OP_DIV,          OPERAND_NONE) \
    OP(OP_DIVF,         OPERAND_NONE) \
    OP(OP_REM,          OPERAND_NONE) \
    OP(OP_REMF,         OPERAND_NONE) \
    /* relational */\
    OP(OP_EQ,           OPERAND_NONE) \
    OP(OP_EQF,          OPERAND_NONE) \
    OP(OP_EQS,          OPERAND_NONE) \
    OP(OP_NEQ,          OPERAND_NONE) \
    OP(OP_NEQF,         OPERAND_NONE) \
    OP(OP_NEQS,         OPERAND_NONE) \
    OP(OP_LT,           OPERAND_NONE) \
    OP(OP_LTF,          OPERAND_NONE) \
    OP(OP_LTE,          OPERAND_NONE) \
    OP(OP_LTEF,         OPERAND_NONE) \
    OP(OP_GT,           OPERAND_NONE) \
    OP(OP_GTF,          OPERAND_NONE) \
    OP(OP_GTE,          OPERAND_NONE) \
    OP(OP_GTEF,         OPERAND_NONE) \
    OP(OP_AND,          OPERAND_NONE) \
    OP(OP_OR,           OPERAND_NONE) \
    OP(OP_XOR,          OPERAND_NONE) \
    OP(OP_NOT,          OPERAND_NONE) \
    OP(OP_SHL,          OPERAND_NONE) \
    OP(OP_SHR,          OPERAND_NONE) \
    OP(OP_NEG,          OPERAND_NONE) \
    OP(OP_NEGF,         OPERAND_NONE) \
    OP(OP_SETZ,         OPERAND_NONE) \
    OP(OP_SETNZ,        OPERAND_NONE) \
    OP(OP_POP,          OPERAND_NONE) \
    OP(OP_DUP,          OPERAND_NONE) \
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
    void DecLocal(Byte id);
    void DecGlobal(Word id);
    void Allocate(Byte count);
    void CallFunction(Word func_index, bool builtin);
    void LoadTypeInt();
    void LoadTypeFloat();
    void LoadTypeString();
    // jump instructions return the address
    // where the destination address is stored.
    Int Jump(Int addr);
    Int JumpIfZero(Int addr);
    void Return();
    void AddInt();
    void AddFloat();
    void ConcatString();
    void SubInt();
    void SubFloat();
    void MulInt();
    void MulFloat();
    void DivInt();
    void DivFloat();
    void RemInt();
    void RemFloat();
    void EqualInt();
    void EqualFloat();
    void EqualString();
    void NotEqualInt();
    void NotEqualFloat();
    void NotEqualString();
    void LessInt();
    void LessFloat();
    void LessEqualInt();
    void LessEqualFloat();
    void GreaterInt();
    void GreaterFloat();
    void GreaterEqualInt();
    void GreaterEqualFloat();
    void And();
    void Or();
    void Xor();
    void Not();
    void ShiftLeft();
    void ShiftRight();
    void NegateInt();
    void NegateFloat();
    void SetIfZero();
    void SetIfNotZero();
    void Pop();
    void DuplicateTop();
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
    Int NextAddr() const;
    Int Size() const;

    // Backpatches
    void BeginFor();
    void BeginSwitch();
    void PushBreak(Int addr);
    void PushContinue(Int addr);
    void PushCaseCloses(Int addr);
    void BackPatchBreaks();
    void BackPatchContinues();
    void BackPatchCaseCloses();

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

    // back patches
    std::stack<Int> breaks_;
    std::stack<Int> continues_;
    std::stack<Int> casecloses_;

    Int print_op(int op, int operand, Int address) const;
};

#endif // _H
