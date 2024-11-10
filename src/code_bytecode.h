#ifndef CODE_BYTECODE_H
#define CODE_BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#include "code_constant_pool.h"
#include "runtime_value.h"
#include "data_hashmap.h"
#include "data_vec.h"

#include "code_instruction.h"

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
    struct code_instructionvec insts;
    int base_reg;
    int curr_reg;
    int max_reg;

    struct data_intstack immediate_ints;
    struct code_constant_pool const_pool;

    ByteVec bytes_;
    PtrVec strings_;
    FuncInfoVec funcs_;

    struct data_hashmap funcnames;

    /* back patches */
    struct data_intstack ors_;
    struct data_intstack breaks_;
    struct data_intstack continues_;
    struct data_intstack casecloses_;
} Bytecode;

/* XXX TEST register */
/* TODO remove Temp? */
void InitLocalVarRegister__(struct Bytecode *code, uint8_t lvar_count);
void ResetCurrentRegister__(struct Bytecode *code);
int NewRegister__(struct Bytecode *code);
int GetCurrentRegister__(const struct Bytecode *code);
int SetCurrentRegister__(struct Bytecode *code, int curr);
int GetNextRegister__(struct Bytecode *code, int reg);
bool IsTempRegister(const struct Bytecode *code, int id);

bool IsImmediateValue__(int id);
struct runtime_value ReadImmediateValue__(const struct Bytecode *code,
        Int addr, int id, int *imm_size);

/* load/store/move */
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

/* array/struct */
int NewArray__(struct Bytecode *code, uint8_t dst, uint8_t len);
int NewStruct__(struct Bytecode *code, uint8_t dst, uint8_t len);

/* arithmetic */
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
/* string */
int ConcatString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
/* function call */
int CallFunction__(Bytecode *code, Byte ret_reg, Word func_index, bool builtin);
int CallFunctionPointer__(struct Bytecode *code, int ret, int src);
void Allocate__(Bytecode *code, Byte count);
void Return__(Bytecode *code, Byte id);
/* branch */
void BeginIf__(struct Bytecode *code);
void code_begin_switch(struct Bytecode *code);
void PushElseEnd__(struct Bytecode *code, Int addr);
void PushBreak__(struct Bytecode *code, Int addr);
void PushContinue__(struct Bytecode *code, Int addr);
void PushCaseEnd__(struct Bytecode *code, Int addr);
/* TODO testing new naming convention */
void code_push_continue(struct Bytecode *code, Int addr);
/* jump instructions return the address */
/* where the destination address is stored. */
Int Jump__(struct Bytecode *code, Int addr);
Int JumpIfZero__(struct Bytecode *code, uint8_t src, Int addr);
Int JumpIfNotZero__(struct Bytecode *code, uint8_t src, Int addr);
/* conversion */
int BoolToInt__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void BoolToFloat__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void IntToBool__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void IntToFloat__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void FloatToBool__(struct Bytecode *code, uint8_t dst, uint8_t src);
//void FloatToInt__(struct Bytecode *code, uint8_t dst, uint8_t src);
/* program control */
void Exit__(Bytecode *code);
void End__(Bytecode *code);

/* functions */
void RegisterFunction__(Bytecode *code, Word func_index, Byte argc);
void SetMaxRegisterCount__(struct Bytecode *code, Word func_index);
int GetMaxRegisterCount__(const struct Bytecode *code, Word func_index);
/* back-patches */
void BeginFor__(struct Bytecode *code);
void BackPatch__(struct Bytecode *code, Int operand_addr);
void BackPatchBreaks__(struct Bytecode *code);
void BackPatchElseEnds__(struct Bytecode *code);
void BackPatchContinues__(struct Bytecode *code);
void code_backpatch_case_ends(struct Bytecode *code);

void PrintInstruction__(const struct Bytecode *code,
        Int addr, const struct code_instruction *inst, int *imm_size);

/* read / write */
uint32_t Read__(const Bytecode *code, Int addr);
void Write__(const Bytecode *code, Int addr, uint32_t inst);
Int Size__(const Bytecode *code);
Int NextAddr__(const struct Bytecode *code);

/* Backpatches */

/* functions */
Int GetFunctionAddress(const Bytecode *code, Word func_index);
Int GetFunctionArgCount(const Bytecode *code, Word func_index);
const char *GetConstString(const Bytecode *code, Word str_index);

/* read/write */

/* print */
void PrintBytecode(const Bytecode *code);

#endif /* _H */
