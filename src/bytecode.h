#ifndef BYTECODE_H
#define BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#include "code_constant_pool.h"
#include "hashmap.h"
#include "value.h"
#include "vec.h"

#define REGISTER_MACHINE 0

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
    // load/store/move
    OP_MOVE__,
    OP_LOADINT__,
    OP_LOADFLOAT__,
    OP_LOAD__,
    OP_STORE__,
    OP_LOADARRAY__,
    OP_STOREARRAY__,
    OP_LOADSTRUCT__,
    OP_STORESTRUCT__,
    OP_LOADTYPENIL__,
    OP_LOADTYPEBOOL__,
    OP_LOADTYPEINT__,
    OP_LOADTYPEFLOAT__,
    OP_LOADTYPESTRING__,
/* TODO remove address operations */
    OP_LOADADDR__,
    OP_DEREF__,
/* ------------------------------ */
    // array/struct
    OP_NEWARRAY__,
    OP_NEWSTRUCT__,
    // arithmetic
    OP_ADDINT__,
    OP_ADDFLOAT__,
    OP_SUBINT__,
    OP_SUBFLOAT__,
    OP_MULINT__,
    OP_MULFLOAT__,
    OP_DIVINT__,
    OP_DIVFLOAT__,
    OP_REMINT__,
    OP_REMFLOAT__,
    OP_EQINT__,
    OP_EQFLOAT__,
    OP_NEQINT__,
    OP_NEQFLOAT__,
    OP_LTINT__,
    OP_LTFLOAT__,
    OP_LTEINT__,
    OP_LTEFLOAT__,
    OP_GTINT__,
    OP_GTFLOAT__,
    OP_GTEINT__,
    OP_GTEFLOAT__,
    OP_BITWISEAND__,
    OP_BITWISEOR__,
    OP_BITWISEXOR__,
    OP_BITWISENOT__,
    OP_SHL__,
    OP_SHR__,
    OP_NEGINT__,
    OP_NEGFLOAT__,
    OP_SETIFZERO__,
    OP_SETIFNOTZ__,
    OP_INC__,
    OP_DEC__,
    // string
    OP_CATSTRING__,
    OP_EQSTRING__,
    OP_NEQSTRING__,
    // function call
    OP_CALL__,
    OP_CALLBUILTIN__,
    OP_RETURN__,
    // jump
    OP_JUMP__,
    OP_JUMPIFZERO__,
    OP_JUMPIFNOTZ__,
    // stack operation TODO move to right place
    OP_ALLOCATE__,
    // conversion
    OP_BOOLTOINT__,
    OP_BOOLTOFLOAT__,
    OP_INTTOBOOL__,
    OP_INTTOFLOAT__,
    OP_FLOATTOBOOL__,
    OP_FLOATTOINT__,
    // program control
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
struct InstVec {
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
    struct InstVec insts;
    struct Value consts[128];
    int const_types[128];
    int const_count;
    int base_reg;
    int curr_reg;
    int max_reg;

    struct IntStack immediate_ints;
    struct code_constant_pool const_pool;

    ByteVec bytes_;
    PtrVec strings_;
    FuncInfoVec funcs_;

    struct HashMap funcnames;

    // back patches
    struct IntStack ors_;
    struct IntStack breaks_;
    struct IntStack continues_;
    struct IntStack casecloses_;
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
// TODO remove Temp?
void InitLocalVarRegister__(struct Bytecode *code, uint8_t lvar_count);
void ResetCurrentRegister__(struct Bytecode *code);
int NewRegister__(struct Bytecode *code);
int GetCurrentRegister__(const struct Bytecode *code);
int SetCurrentRegister__(struct Bytecode *code, int curr);
int GetNextRegister__(struct Bytecode *code, int reg);
bool IsTempRegister(const struct Bytecode *code, int id);

bool IsConstValue__(int id);
bool IsImmediateValue__(int id);
struct Value ReadImmediateValue__(const struct Bytecode *code, Int addr, int id, int *imm_size);

// load/store/move
int Move__(struct Bytecode *code, uint8_t dst, uint8_t src);
int LoadInt__(struct Bytecode *code, int64_t val);
int LoadFloat__(struct Bytecode *code, double val);
int LoadString__(struct Bytecode *code, const char *cstr);
int Load__(struct Bytecode *code, uint8_t dst, uint8_t src);
int Store__(struct Bytecode *code, uint8_t dst, uint8_t src);
int LoadArray__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t idx);
int StoreArray__(struct Bytecode *code, uint8_t dst, uint8_t idx, uint8_t src);
int LoadStruct__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t field_idx);
int StoreStruct__(struct Bytecode *code, uint8_t dst, uint8_t field_idx, uint8_t src);
int LoadTypeNil__(struct Bytecode *code, int dst);
int LoadTypeBool__(struct Bytecode *code, int dst);
int LoadTypeInt__(struct Bytecode *code, int dst);
int LoadTypeFloat__(struct Bytecode *code, int dst);
int LoadTypeString__(struct Bytecode *code, int dst);

/* TODO remove address operations */
int LoadAddress__(struct Bytecode *code, int dst, int src);
int Dereference__(struct Bytecode *code, int dst, int src);
/* ------------------------------ */

// array/struct
int NewArray__(struct Bytecode *code, uint8_t dst, uint8_t len);
int NewStruct__(struct Bytecode *code, uint8_t dst, uint8_t len);

// arithmetic
int AddInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int AddFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int SubInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int SubFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int MulInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int MulFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int DivInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int DivFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int RemInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int RemFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseAnd__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseOr__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseXor__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseNot__(struct Bytecode *code, uint8_t dst, uint8_t src);
int ShiftLeft__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int ShiftRight__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NegateInt__(struct Bytecode *code, uint8_t dst, uint8_t src);
int NegateFloat__(struct Bytecode *code, uint8_t dst, uint8_t src);
int SetIfZero__(struct Bytecode *code, uint8_t dst, uint8_t src);
int SetIfNotZero__(struct Bytecode *code, uint8_t dst, uint8_t src);
int Inc__(struct Bytecode *code, uint8_t src);
int Dec__(struct Bytecode *code, uint8_t src);
// string
int ConcatString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
// function call
int CallFunction__(Bytecode *code, Byte ret_reg, Word func_index, bool builtin);
void Allocate__(Bytecode *code, Byte count);
void Return__(Bytecode *code, Byte id);
// branch
void BeginIf__(struct Bytecode *code);
void code_begin_switch(struct Bytecode *code);
void PushElseEnd__(struct Bytecode *code, Int addr);
void PushBreak__(struct Bytecode *code, Int addr);
void PushContinue__(struct Bytecode *code, Int addr);
void PushCaseEnd__(struct Bytecode *code, Int addr);
// TODO testing new naming convention
void code_push_continue(struct Bytecode *code, Int addr);
// jump instructions return the address
// where the destination address is stored.
Int Jump__(struct Bytecode *code, Int addr);
Int JumpIfZero__(struct Bytecode *code, uint8_t src, Int addr);
Int JumpIfNotZero__(struct Bytecode *code, uint8_t src, Int addr);
// conversion
int BoolToInt__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void BoolToFloat__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void IntToBool__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void IntToFloat__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void FloatToBool__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void FloatToInt__(struct Bytecode *code, uint8_t dst, uint8_t src);
// program control
void Exit__(Bytecode *code);
void End__(Bytecode *code);

// functions
void RegisterFunction__(Bytecode *code, Word func_index, Byte argc);
void SetMaxRegisterCount__(struct Bytecode *code, Word func_index);
int GetMaxRegisterCount__(const struct Bytecode *code, Word func_index);
// back-patches
void BeginFor__(struct Bytecode *code);
void BackPatch__(struct Bytecode *code, Int operand_addr);
void BackPatchBreaks__(struct Bytecode *code);
void BackPatchElseEnds__(struct Bytecode *code);
void BackPatchContinues__(struct Bytecode *code);
void code_backpatch_case_ends(struct Bytecode *code);

struct Instruction {
    int op;
    Byte A, B, C;
    Word BB;
    Int iIMM;
    Float fIMM;
};
void Decode__(uint32_t instcode, struct Instruction *inst);
void PrintInstruction__(const struct Bytecode *code,
        Int addr, const struct Instruction *inst, int *imm_size);

// read / write
uint32_t Read__(const Bytecode *code, Int addr);
void Write__(const Bytecode *code, Int addr, uint32_t inst);
Int Size__(const Bytecode *code);
Int NextAddr__(const struct Bytecode *code);

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
