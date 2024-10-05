#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stdbool.h>
#include "hashmap.h"

// XXX TEST nees this?
#include "gc.h"
#define REGISTER_MACHINE 0

typedef uint8_t  Byte;
typedef uint16_t Word;
typedef int64_t  Int;
typedef double   Float;

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
    OP_LOAD,
    OP_STORE,
    OP_INCLOCAL,
    OP_INCGLOBAL,
    OP_DECLOCAL,
    OP_DECGLOBAL,
    OP_ALLOC,
    // clear
    OP_CLEAR_LOCAL,
    OP_CLEAR_GLOBAL,
    OP_COPY_LOCAL,
    OP_COPY_GLOBAL,
    // address
    OP_LOADA,
    OP_DEREF,
    OP_INDEX,
    // arg type spec
    OP_LOADTYPEN,
    OP_LOADTYPEB,
    OP_LOADTYPEI,
    OP_LOADTYPEF,
    OP_LOADTYPES,
    // jump and function
    OP_CALL,
    OP_CALL_POINTER,
    OP_CALL_BUILTIN,
    OP_RET,
    OP_JMP,
    OP_JEQ,
    // arithmetic
    OP_ADD,
    OP_ADDF,
    OP_CATS,
    OP_SUB,
    OP_SUBF,
    OP_MUL,
    OP_MULF,
    OP_DIV,
    OP_DIVF,
    OP_REM,
    OP_REMF,
    // relational
    OP_EQ,
    OP_EQF,
    OP_EQS,
    OP_NEQ,
    OP_NEQF,
    OP_NEQS,
    OP_LT,
    OP_LTF,
    OP_LTE,
    OP_LTEF,
    OP_GT,
    OP_GTF,
    OP_GTE,
    OP_GTEF,
    OP_AND,
    OP_OR,
    OP_XOR,
    OP_NOT,
    OP_SHL,
    OP_SHR,
    OP_NEG,
    OP_NEGF,
    OP_SETZ,
    OP_SETNZ,
    OP_POP,
    OP_DUP,
    // conversion
    OP_BTOI,
    OP_BTOF,
    OP_ITOB,
    OP_ITOF,
    OP_FTOB,
    OP_FTOI,
    // array
    OP_ARRAYLOCAL,
    // debug
    OP_PUSH_CHECK_NUM,
    OP_POP_CHECK_NUM,
    // exit
    OP_EXIT,
    OP_EOC,
    // XXX TEST register machine
    OP_NOP__,
    // Load/store/move
    OP_COPY__,
    OP_LOADINT16__,
    OP_LOAD__,
    OP_STORE__,
    // Arithmetic
    OP_ADDINT__,
    // Function call
    OP_CALL__,
    OP_RETURN__,
    // Stack operation
    OP_ALLOCATE__,
    // Program control
    OP_EXIT__,
    OP_EOC__,
    // XXX TEST register machine
    END_OF_OPCODE__,
};

struct OpcodeInfo {
    int opcode;
    const char *mnemonic;
    int operand_size;
};

const char *OpcodeString(Byte op);

// XXX TEST register machine
struct Int32Vec {
    uint32_t *data;
    int cap;
    int len;
};
// XXX TEST register machine

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
    Byte reg_count;
    Int addr;
} FuncInfo;

typedef struct FuncInfoVec {
    FuncInfo *data;
    int cap;
    int len;
} FuncInfoVec;

typedef struct Bytecode {
    struct Int32Vec insts;
    struct Value consts[128];
    int const_count;
    int bp;
    int sp;
    int maxsp;

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
// clear
void ClearLocal(Bytecode *code, uint16_t base, uint16_t count);
void ClearGlobal(Bytecode *code, uint16_t base, uint16_t count);
void CopyLocal(Bytecode *code, uint16_t src, uint16_t dst, uint16_t count);
void CopyGlobal(Bytecode *code, uint16_t src, uint16_t dst, uint16_t count);

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
void CallFunctionPointer(struct Bytecode *code);
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
// array
void ArrayLocal(Bytecode *code, Byte id);
//
void PushCheckNum(Bytecode *code, int64_t num);
void PopCheckNum(Bytecode *code, int64_t num);
//
void Exit(Bytecode *code);
void End(Bytecode *code);

// XXX TEST register
int NewRegister__(Bytecode *code);
void ResetTempRegister__(struct Bytecode *code);

int PoolInt__(Bytecode *code, Int val);
struct Value GetConstValue__(const Bytecode *code, Byte id);
bool IsConstValue__(Byte id);

// Load/store/move
int Copy__(Bytecode *code, Byte dst, Byte src);
int LoadInt__(Bytecode *code, Int integer);
int Load__(struct Bytecode *code, uint8_t dst, uint8_t src);
int Store__(struct Bytecode *code, uint8_t dst, uint8_t src);
// Arithmetic
int AddInt__(Bytecode *code, Byte dst, Byte src0, Byte src1);
// Function call
int CallFunction__(Bytecode *code, Byte ret_reg, Word func_index, bool builtin);
void Allocate__(Bytecode *code, Byte count);
void Return__(Bytecode *code, Byte id);
// Program control
void Exit__(Bytecode *code);
void End__(Bytecode *code);
bool IsTempRegister(const struct Bytecode *code, Byte id);

// Functions
void RegisterFunction__(Bytecode *code, Word func_index, Byte argc);
void SetMaxRegisterCount__(struct Bytecode *code, Word func_index);
int GetMaxRegisterCount__(const struct Bytecode *code, Word func_index);

struct Instruction {
    int op;
    Byte A, B, C;
    Word BB;
    Int iIMM;
    Float fIMM;
};
void Decode__(uint32_t instcode, struct Instruction *inst);
void PrintInstruction__(const struct Bytecode *code,
        Int addr, const struct Instruction *inst);

uint32_t Read__(const Bytecode *code, Int addr);
Int Size__(const Bytecode *code);
Int NextAddr__(const Bytecode *code);

#define DECODE_OP(inst) (((inst) >> 24))
#define DECODE_A(inst)  (((inst) >> 16) & 0xFF)
#define DECODE_B(inst)  (((inst) >>  8) & 0xFF)
#define DECODE_C(inst)  ((inst) & 0xFF)
#define DECODE_BB(inst) ((inst) & 0xFFFF)
#define ENCODE_ABB(op,a,bb) (((op) << 24) | ((a) << 16) | (bb))
// XXX TEST register

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
