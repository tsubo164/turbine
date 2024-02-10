#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

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
    /* debug */\
    OP(OP_PUSH_CHECK_NUM, OPERAND_QUAD) \
    OP(OP_POP_CHECK_NUM,  OPERAND_QUAD) \
    /* exit */\
    OP(OP_EXIT,         OPERAND_NONE) \
    OP(OP_EOC,          OPERAND_NONE) \

enum Opcode {
#define OP(opcode, operand_size) opcode,
    BYTECODE_LIST
#undef OP
};

const char *OpcodeString(Byte op);

typedef struct ByteVec {
    Byte *data;
    int cap;
    int len;
} ByteVec;

typedef struct AddrStack {
    Int *data;
    int sp;
} AddrStack;

typedef struct PtrVec {
    char **data;
    int cap;
    int len;
} PtrVec;

typedef struct FuncInfo {
    Word id;
    Byte argc;
    Int addr;
} FuncInfo;

typedef struct FuncInfoVec {
    FuncInfo *data;
    int cap;
    int len;
} FuncInfoVec;

typedef struct Bytecode {
    ByteVec bytes_;
    PtrVec strings_;
    FuncInfoVec funcs_;

    struct HashMap funcnames;

    // back patches
    AddrStack ors_;
    AddrStack breaks_;
    AddrStack continues_;
    AddrStack casecloses_;
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
void CallFunc(Bytecode *code, const char *fullname, bool builtin);
void CallFunction(Bytecode *code, Word func_index, bool builtin);
// jump instructions return the address
// where the destination address is stored.
Int Jump(Bytecode *code, Int addr);
Int JumpIfZero(Bytecode *code, Int addr);
void Return(Bytecode *code);

// arithmetic
void AddInt(Bytecode *code);
void AddFloat(Bytecode *code);
void ConcatString(Bytecode *code);
void SubInt(Bytecode *code);
void SubFloat(Bytecode *code);
void MulInt(Bytecode *code);
void MulFloat(Bytecode *code);
void DivInt(Bytecode *code);
void DivFloat(Bytecode *code);
void RemInt(Bytecode *code);
void RemFloat(Bytecode *code);
void EqualInt(Bytecode *code);
void EqualFloat(Bytecode *code);
void EqualString(Bytecode *code);
void NotEqualInt(Bytecode *code);
void NotEqualFloat(Bytecode *code);
void NotEqualString(Bytecode *code);
void LessInt(Bytecode *code);
void LessFloat(Bytecode *code);
void LessEqualInt(Bytecode *code);
void LessEqualFloat(Bytecode *code);
void GreaterInt(Bytecode *code);
void GreaterFloat(Bytecode *code);
void GreaterEqualInt(Bytecode *code);
void GreaterEqualFloat(Bytecode *code);
void And(Bytecode *code);
void Or(Bytecode *code);
void Xor(Bytecode *code);
void Not(Bytecode *code);
void ShiftLeft(Bytecode *code);
void ShiftRight(Bytecode *code);
void NegateInt(Bytecode *code);
void NegateFloat(Bytecode *code);
void SetIfZero(Bytecode *code);
void SetIfNotZero(Bytecode *code);

// stack
void Pop(Bytecode *code);
void DuplicateTop(Bytecode *code);
// conversion
void BoolToInt(Bytecode *code);
void BoolToFloat(Bytecode *code);
void IntToBool(Bytecode *code);
void IntToFloat(Bytecode *code);
void FloatToBool(Bytecode *code);
void FloatToInt(Bytecode *code);
//
void PushCheckNum(Bytecode *code, int64_t num);
void PopCheckNum(Bytecode *code, int64_t num);
//
void Exit(Bytecode *code);
void End(Bytecode *code);

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
void BackPatchFuncAddr(struct Bytecode *code, const char *fullname);
uint16_t RegisterFunc(struct Bytecode *code, const char *fullname, uint8_t argc);
Int GetFunctionAddress(const Bytecode *code, Word func_index);
Int GetFunctionArgCount(const Bytecode *code, Word func_index);
void RegisterFunction(Bytecode *code, Word func_index, Byte argc);
Int RegisterConstString(Bytecode *code, const char *str);
const char *GetConstString(const Bytecode *code, Word str_index);

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
