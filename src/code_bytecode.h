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
int code_emit_new_array(struct code_bytecode *code, int dst, int len);
int code_emit_new_struct(struct code_bytecode *code, int dst, int len);

/* arithmetic */
int code_emit_add_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_add_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_sub_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_sub_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_mul_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_mul_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_div_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_div_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_rem_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_rem_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_equal_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_equal_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_not_equal_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_not_equal_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_less_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_less_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_less_equal_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_less_equal_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_greater_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_greater_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_greater_equal_int(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_greater_equal_float(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_bitwise_and(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_bitwise_or(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_bitwise_xor(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_bitwise_not(struct code_bytecode *code, int dst, int src);
int code_emit_shift_left(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_shift_right(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_negate_int(struct code_bytecode *code, int dst, int src);
int code_emit_negate_float(struct code_bytecode *code, int dst, int src);
int code_emit_set_if_zero(struct code_bytecode *code, int dst, int src);
int code_emit_set_if_not_zero(struct code_bytecode *code, int dst, int src);
int code_emit_inc(struct code_bytecode *code, int src);
int code_emit_dec(struct code_bytecode *code, int src);

/* string */
int code_emit_concat_string(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_equal_string(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_not_equal_string(struct code_bytecode *code, int dst, int src0, int src1);

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
