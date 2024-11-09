#ifndef CODE_BYTECODE_H
#define CODE_BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#include "code_constant_pool.h"
#include "runtime_value.h"
#include "data_hashmap.h"
#include "data_vec.h"

enum Opcode {
    /* XXX TEST register machine */
    OP_NOP__,
    /* load/store/move */
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
    /* array/struct */
    OP_NEWARRAY__,
    OP_NEWSTRUCT__,
    /* arithmetic */
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
    /* string */
    OP_CATSTRING__,
    OP_EQSTRING__,
    OP_NEQSTRING__,
    /* function call */
    OP_CALL__,
    OP_CALLPOINTER__,
    OP_CALLBUILTIN__,
    OP_RETURN__,
    /* jump */
    OP_JUMP__,
    OP_JUMPIFZERO__,
    OP_JUMPIFNOTZ__,
    /* stack operation TODO move to right place */
    OP_ALLOCATE__,
    /* conversion */
    OP_BOOLTOINT__,
    OP_BOOLTOFLOAT__,
    OP_INTTOBOOL__,
    OP_INTTOFLOAT__,
    OP_FLOATTOBOOL__,
    OP_FLOATTOINT__,
    /* program control */
    OP_EXIT__,
    OP_EOC__,
    /* XXX TEST register machine */
    END_OF_OPCODE__,
};

struct OpcodeInfo {
    int opcode;
    const char *mnemonic;
    int operand_size;
};

const char *OpcodeString(Byte op);

/* XXX TEST register machine */
struct InstVec {
    uint32_t *data;
    int cap;
    int len;
};
/* XXX TEST register machine */

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

/* read / write */
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
/* XXX TEST register */

/* Backpatches */

/* functions */
Int GetFunctionAddress(const Bytecode *code, Word func_index);
Int GetFunctionArgCount(const Bytecode *code, Word func_index);
const char *GetConstString(const Bytecode *code, Word str_index);

/* read/write */

/* print */
void PrintBytecode(const Bytecode *code);

#endif /* _H */
