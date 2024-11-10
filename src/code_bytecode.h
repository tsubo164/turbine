#ifndef CODE_BYTECODE_H
#define CODE_BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#include "code_instruction.h"
#include "code_constant_pool.h"
#include "code_function.h"
#include "runtime_value.h"
#include "data_vec.h"

struct code_bytecode {
    /* instructions */
    struct code_instructionvec insts;

    /* registers */
    int base_reg;
    int curr_reg;
    int max_reg;

    /* constants */
    struct data_intstack immediate_ints;
    struct code_constant_pool const_pool;

    /* functions */
    struct code_functionvec funcs;

    /* back patches */
    struct data_intstack ors;
    struct data_intstack breaks;
    struct data_intstack continues;
    struct data_intstack casecloses;
};

/* TODO remove Temp? */
void InitLocalVarRegister__(struct code_bytecode *code, uint8_t lvar_count);
void ResetCurrentRegister__(struct code_bytecode *code);
int NewRegister__(struct code_bytecode *code);
int GetCurrentRegister__(const struct code_bytecode *code);
int SetCurrentRegister__(struct code_bytecode *code, int curr);
int GetNextRegister__(struct code_bytecode *code, int reg);
bool IsTempRegister(const struct code_bytecode *code, int id);

bool IsImmediateValue__(int id);
struct runtime_value ReadImmediateValue__(const struct code_bytecode *code,
        Int addr, int id, int *imm_size);

/* load/store/move */
int code_emit_move(struct code_bytecode *code, int dst, int src);
int code_emit_load_int(struct code_bytecode *code, int64_t val);
int code_emit_load_float(struct code_bytecode *code, double val);
int code_emit_load_string(struct code_bytecode *code, const char *cstr);
int code_emit_load_global(struct code_bytecode *code, int dst, int src);
int code_emit_store_global(struct code_bytecode *code, int dst, int src);
int code_emit_load_array(struct code_bytecode *code, int dst, int src, int idx);
int code_emit_store_array(struct code_bytecode *code, int dst, int idx, int src);
int code_emit_load_struct(struct code_bytecode *code, int dst, int src, int field_idx);
int code_emit_store_struct(struct code_bytecode *code, int dst, int field_idx, int src);
int code_emit_load_type_nil(struct code_bytecode *code, int dst);
int code_emit_load_type_bool(struct code_bytecode *code, int dst);
int code_emit_load_type_int(struct code_bytecode *code, int dst);
int code_emit_load_type_float(struct code_bytecode *code, int dst);
int code_emit_load_type_string(struct code_bytecode *code, int dst);
/* TODO remove address operations */
int code_emit_load_address(struct code_bytecode *code, int dst, int src);
int code_emit_dereference(struct code_bytecode *code, int dst, int src);

/* array/struct */
int NewArray__(struct code_bytecode *code, uint8_t dst, uint8_t len);
int NewStruct__(struct code_bytecode *code, uint8_t dst, uint8_t len);

/* arithmetic */
int AddInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int AddFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int SubInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int SubFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int MulInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int MulFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int DivInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int DivFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int RemInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int RemFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessEqualInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int LessEqualFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterEqualInt__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int GreaterEqualFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseAnd__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseOr__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseXor__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int BitwiseNot__(struct code_bytecode *code, uint8_t dst, uint8_t src);
int ShiftLeft__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int ShiftRight__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NegateInt__(struct code_bytecode *code, uint8_t dst, uint8_t src);
int NegateFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src);
int SetIfZero__(struct code_bytecode *code, uint8_t dst, uint8_t src);
int SetIfNotZero__(struct code_bytecode *code, uint8_t dst, uint8_t src);
int Inc__(struct code_bytecode *code, uint8_t src);
int Dec__(struct code_bytecode *code, uint8_t src);
/* string */
int ConcatString__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int EqualString__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
int NotEqualString__(struct code_bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1);
/* function call */
int CallFunction__(struct code_bytecode *code, Byte ret_reg, Word func_index, bool builtin);
int CallFunctionPointer__(struct code_bytecode *code, int ret, int src);
void Allocate__(struct code_bytecode *code, Byte count);
void Return__(struct code_bytecode *code, Byte id);
/* branch */
void BeginIf__(struct code_bytecode *code);
void code_begin_switch(struct code_bytecode *code);
void PushElseEnd__(struct code_bytecode *code, Int addr);
void PushBreak__(struct code_bytecode *code, Int addr);
void PushContinue__(struct code_bytecode *code, Int addr);
void PushCaseEnd__(struct code_bytecode *code, Int addr);
/* TODO testing new naming convention */
void code_push_continue(struct code_bytecode *code, Int addr);
/* jump instructions return the address */
/* where the destination address is stored. */
Int Jump__(struct code_bytecode *code, Int addr);
Int JumpIfZero__(struct code_bytecode *code, uint8_t src, Int addr);
Int JumpIfNotZero__(struct code_bytecode *code, uint8_t src, Int addr);
/* conversion */
int BoolToInt__(struct code_bytecode *code, uint8_t dst, uint8_t src);
//void BoolToFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src);
//void IntToBool__(struct code_bytecode *code, uint8_t dst, uint8_t src);
//void IntToFloat__(struct code_bytecode *code, uint8_t dst, uint8_t src);
//void FloatToBool__(struct code_bytecode *code, uint8_t dst, uint8_t src);
//void FloatToInt__(struct code_bytecode *code, uint8_t dst, uint8_t src);
/* program control */
void Exit__(struct code_bytecode *code);
void End__(struct code_bytecode *code);

/* back-patches */
void BeginFor__(struct code_bytecode *code);
void BackPatch__(struct code_bytecode *code, Int operand_addr);
void BackPatchBreaks__(struct code_bytecode *code);
void BackPatchElseEnds__(struct code_bytecode *code);
void BackPatchContinues__(struct code_bytecode *code);
void code_backpatch_case_ends(struct code_bytecode *code);

/* read/write/address */
uint32_t Read__(const struct code_bytecode *code, Int addr);
void Write__(const struct code_bytecode *code, Int addr, uint32_t inst);
Int Size__(const struct code_bytecode *code);
Int NextAddr__(const struct code_bytecode *code);

/* functions */
void RegisterFunction__(struct code_bytecode *code, Word func_index, Byte argc);
void SetMaxRegisterCount__(struct code_bytecode *code, Word func_index);
int GetMaxRegisterCount__(const struct code_bytecode *code, Word func_index);
Int GetFunctionAddress(const struct code_bytecode *code, Word func_index);
Int GetFunctionArgCount(const struct code_bytecode *code, Word func_index);

/* print */
void PrintBytecode(const struct code_bytecode *code);
void PrintInstruction__(const struct code_bytecode *code,
        Int addr, const struct code_instruction *inst, int *imm_size);

#endif /* _H */
