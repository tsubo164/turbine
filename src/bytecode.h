#ifndef BYTECODE_H
#define BYTECODE_H

#include <string_view>
#include <cstdint>
#include <vector>
#include <string>
#include <stack>

//using Byte = uint8_t;
//using Word = uint16_t;
//using Int = int64_t;
//using Float = double;
typedef uint8_t  Byte;
typedef uint16_t Word;
typedef int64_t  Int;
typedef double   Float;

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
    OP(OP_LOAD,         OPERAND_NONE) \
    OP(OP_STORE,        OPERAND_NONE) \
    OP(OP_INCLOCAL,     OPERAND_BYTE) \
    OP(OP_INCGLOBAL,    OPERAND_WORD) \
    OP(OP_DECLOCAL,     OPERAND_BYTE) \
    OP(OP_DECGLOBAL,    OPERAND_WORD) \
    OP(OP_ALLOC,        OPERAND_BYTE) \
    /* address */\
    OP(OP_LOADA,        OPERAND_WORD) \
    OP(OP_DEREF,        OPERAND_NONE) \
    OP(OP_INDEX,        OPERAND_NONE) \
    /* arg type spec */\
    OP(OP_LOADTYPEN,    OPERAND_NONE) \
    OP(OP_LOADTYPEB,    OPERAND_NONE) \
    OP(OP_LOADTYPEI,    OPERAND_NONE) \
    OP(OP_LOADTYPEF,    OPERAND_NONE) \
    OP(OP_LOADTYPES,    OPERAND_NONE) \
    /* jump and function */\
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
    /* conversion */\
    OP(OP_BTOI,         OPERAND_NONE) \
    OP(OP_BTOF,         OPERAND_NONE) \
    OP(OP_ITOB,         OPERAND_NONE) \
    OP(OP_ITOF,         OPERAND_NONE) \
    OP(OP_FTOB,         OPERAND_NONE) \
    OP(OP_FTOI,         OPERAND_NONE) \
    /* exit */\
    OP(OP_EXIT,         OPERAND_NONE) \
    OP(OP_EOC,          OPERAND_NONE) \

enum Opcode {
#define OP(opcode, operand_size) opcode,
    BYTECODE_LIST
#undef OP
};

const char *OpcodeString(Byte op);

typedef struct Bytecode {
    // arithmetic
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
    // stack
    void Pop();
    void DuplicateTop();
    // conversion
    void BoolToInt();
    void BoolToFloat();
    void IntToBool();
    void IntToFloat();
    void FloatToBool();
    void FloatToInt();
    //
    void Exit();
    void End();

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
    std::stack<Int> ors_;
    std::stack<Int> breaks_;
    std::stack<Int> continues_;
    std::stack<Int> casecloses_;

} Bytecode;

// emit opcode and operand
void LoadByte(Bytecode *code, Byte byte);
void LoadInt(Bytecode *code, Int integer);
void LoadFloat(Bytecode *code, Float fp);
void LoadString(Bytecode *code, Word id);
void LoadLocal(Bytecode *code, Byte id);
void LoadGlobal(Bytecode *code, Word id);
void StoreLocal(Bytecode *code, Byte id);
void StoreGlobal(Bytecode *code, Word id);
void Load(Bytecode *code);
void Store(Bytecode *code);
void IncLocal(Bytecode *code, Byte id);
void IncGlobal(Bytecode *code, Word id);
void DecLocal(Bytecode *code, Byte id);
void DecGlobal(Bytecode *code, Word id);
void Allocate(Bytecode *code, Byte count);

// address
void LoadAddress(Bytecode *code, Word id);
void Dereference(Bytecode *code);
void Index(Bytecode *code);
// arg type spec
void LoadTypeNil(Bytecode *code);
void LoadTypeBool(Bytecode *code);
void LoadTypeInt(Bytecode *code);
void LoadTypeFloat(Bytecode *code);
void LoadTypeString(Bytecode *code);
// jump and function
void CallFunction(Bytecode *code, Word func_index, bool builtin);
// jump instructions return the address
// where the destination address is stored.
Int Jump(Bytecode *code, Int addr);
Int JumpIfZero(Bytecode *code, Int addr);
void Return(Bytecode *code);

// Backpatches
void BeginIf(Bytecode *code);
void BeginFor(Bytecode *code);
void BeginSwitch(Bytecode *code);
void PushOrClose(Bytecode *code, Int addr);
void PushBreak(Bytecode *code, Int addr);
void PushContinue(Bytecode *code, Int addr);
void PushCaseClose(Bytecode *code, Int addr);
void BackPatch(Bytecode *code, Int operand_addr);
void BackPatchOrCloses(Bytecode *code);
void BackPatchBreaks(Bytecode *code);
void BackPatchContinues(Bytecode *code);
void BackPatchCaseCloses(Bytecode *code);

// functions
Int GetFunctionAddress(const Bytecode *code, Word func_index);
Int GetFunctionArgCount(const Bytecode *code, Word func_index);
void RegisterFunction(Bytecode *code, Word func_index, Byte argc);
Int RegisterConstString(Bytecode *code, std::string_view str);
const std::string &GetConstString(const Bytecode *code, Word str_index);

// read/write
Byte Read(const Bytecode *code, Int addr);
Word ReadWord(const Bytecode *code, Int addr);
Int ReadInt(const Bytecode *code, Int addr);
Float ReadFloat(const Bytecode *code, Int addr);
Int NextAddr(const Bytecode *code);
Int Size(const Bytecode *code);

// print
void PrintBytecode(const Bytecode *code);

#endif // _H
