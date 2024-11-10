#include "code_bytecode.h"
#include "error.h"
#include "data_vec.h"
#include "mem.h"
/* TODO can remove this? */
#include "gc.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

enum immediate_value_register {
    IMMEDIATE_INT32   = 255,
    IMMEDIATE_INT64   = 254,
    IMMEDIATE_FLOAT   = 253,
    IMMEDIATE_STRING  = 252,
    IMMEDIATE_SMALLINT_END   = 251,
    IMMEDIATE_SMALLINT_BEGIN = 192,
};

static int new_cap(int cur_cap, int min_cap)
{
    return cur_cap < min_cap ? min_cap : 2 * cur_cap;
}

static void push_info(FuncInfoVec *v, Word id, Byte argc, Int addr)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 8);
        /* TODO Remove cast */
        v->data = (FuncInfo *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    FuncInfo *info = &v->data[v->len++];
    info->id = id;
    info->argc = argc;
    info->addr = addr;
}

static void assert_range(const FuncInfoVec *v,  Word index)
{
    if (index >= v->len) {
        InternalError(__FILE__, __LINE__,
                "function index out of range: %d, function count: %d\n",
                index, v->len);
    }
}

static void push_immediate_value(struct Bytecode *code, int operand);

static void push_inst_op(struct Bytecode *code, uint8_t op)
{
    code_push_instruction__(&code->insts, op);
}

static void push_inst_a(struct Bytecode *code, uint8_t op, uint8_t a)
{
    code_push_instruction_a(&code->insts, op, a);
    push_immediate_value(code, a);
}

static void push_inst_ab(struct Bytecode *code, uint8_t op, uint8_t a, uint8_t b)
{
    code_push_instruction_ab(&code->insts, op, a, b);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
}

static void push_inst_abc(struct Bytecode *code, uint8_t op, uint8_t a, uint8_t b, uint8_t c)
{
    code_push_instruction_abc(&code->insts, op, a, b, c);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
    push_immediate_value(code, c);
}

static void push_inst_abb(struct Bytecode *code, uint8_t op, uint8_t a, uint16_t bb)
{
    code_push_instruction_abb(&code->insts, op, a, bb);
    push_immediate_value(code, a);
}

static bool is_localreg_full(const struct Bytecode *code)
{
    return code->curr_reg == IMMEDIATE_SMALLINT_BEGIN - 1;
}

void InitLocalVarRegister__(struct Bytecode *code, uint8_t lvar_count)
{
    code->base_reg = lvar_count - 1;
    code->curr_reg = code->base_reg;
    code->max_reg = code->base_reg;
}

void ResetCurrentRegister__(struct Bytecode *code)
{
    code->curr_reg = code->base_reg;
}

int NewRegister__(struct Bytecode *code)
{
    if (is_localreg_full(code)) {
        return -1;
    }

    code->curr_reg++;
    if (code->max_reg < code->curr_reg)
        code->max_reg = code->curr_reg;

    return code->curr_reg;
}

int GetCurrentRegister__(const struct Bytecode *code)
{
    return code->curr_reg;
}

int SetCurrentRegister__(struct Bytecode *code, int curr)
{
    code->curr_reg = curr;
    return curr;
}

int GetNextRegister__(struct Bytecode *code, int reg)
{
    int next = -1;

    if (reg == code->max_reg) {
        next = NewRegister__(code);
    }
    else {
        next = reg + 1;
        code->curr_reg = next;
    }

    return next;
}

bool IsTempRegister(const struct Bytecode *code, int id)
{
    return id > code->base_reg && !IsImmediateValue__(id);
}

/* TODO remove and embed */
static bool can_fit_smallint(int64_t val)
{
    int SMALLINT_SIZE = IMMEDIATE_SMALLINT_END - IMMEDIATE_SMALLINT_BEGIN + 1;
    return val >= 0 && val < SMALLINT_SIZE;
}

/* TODO remove and embed */
static bool can_fit_int32(int64_t val)
{
    return val >= INT32_MIN && val <= INT32_MAX;
}

bool IsImmediateValue__(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN;
}

static bool is_smallint_register(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN && id <= IMMEDIATE_SMALLINT_END;
}

static bool is_constpool_register(int id)
{
    return id > IMMEDIATE_SMALLINT_END && id <= 0xFF;
}

int register_to_smallint(int id)
{
    return id - IMMEDIATE_SMALLINT_BEGIN;
}

int smallint_to_register(int64_t val)
{
    return val + IMMEDIATE_SMALLINT_BEGIN;
}

static void push_immediate_value(struct Bytecode *code, int operand)
{
    if (!is_constpool_register(operand))
        return;

    int64_t val = data_intstack_pop(&code->immediate_ints);
    int32_t id = val & 0xFFFFFFFF;

    code_push_immediate_value(&code->insts, id);
}

struct runtime_value ReadImmediateValue__(const struct Bytecode *code,
        Int addr, int id, int *imm_size)
{
    struct runtime_value value;

    if (is_smallint_register(id)) {
        value.inum = register_to_smallint(id);
        return value;
    }

    switch (id) {

    case IMMEDIATE_INT32:
        {
            int32_t imm = Read__(code, addr);
            value.inum = imm;
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_INT64:
        {
            int32_t id = Read__(code, addr);
            value = code_constant_pool_get_int(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            int32_t id = Read__(code, addr);
            value = code_constant_pool_get_float(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_STRING:
        {
            int64_t id = Read__(code, addr);
            value = code_constant_pool_get_string(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;
    }

    return value;
}

/* Load/store/move */
int Move__(Bytecode *code, Byte dst, Byte src)
{
    if (dst == src)
        return dst;
    push_inst_ab(code, OP_MOVE, dst, src);
    return dst;
}

int LoadInt__(struct Bytecode *code, int64_t val)
{
    if (can_fit_smallint(val)) {
        return smallint_to_register(val);
    }
    else if (can_fit_int32(val)) {
        data_intstack_push(&code->immediate_ints, val);
        return IMMEDIATE_INT32;
    }
    else {
        int id = code_constant_pool_push_int(&code->const_pool, val);
        data_intstack_push(&code->immediate_ints, id);
        return IMMEDIATE_INT64;
    }
}

int LoadFloat__(struct Bytecode *code, double val)
{
    int id = code_constant_pool_push_float(&code->const_pool, val);
    data_intstack_push(&code->immediate_ints, id);
    return IMMEDIATE_FLOAT;
}

int LoadString__(struct Bytecode *code, const char *cstr)
{
    int id = code_constant_pool_push_string(&code->const_pool, cstr);
    data_intstack_push(&code->immediate_ints, id);
    return IMMEDIATE_STRING;
}

int Load__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_LOAD, dst, src);
    return dst;
}

int Store__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_STORE, dst, src);
    return dst;
}

int LoadArray__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t idx)
{
    push_inst_abc(code, OP_LOADARRAY, dst, src, idx);
    return dst;
}

int StoreArray__(struct Bytecode *code, uint8_t dst, uint8_t idx, uint8_t src)
{
    push_inst_abc(code, OP_STOREARRAY, dst, idx, src);
    return dst;
}

int LoadStruct__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t field_idx)
{
    push_inst_abc(code, OP_LOADSTRUCT, dst, src, field_idx);
    return dst;
}

int StoreStruct__(struct Bytecode *code, uint8_t dst, uint8_t field_idx, uint8_t src)
{
    push_inst_abc(code, OP_STORESTRUCT, dst, field_idx, src);
    return dst;
}

int LoadTypeNil__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPENIL, dst);
    return dst;
}

int LoadTypeBool__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEBOOL, dst);
    return dst;
}

int LoadTypeInt__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEINT, dst);
    return dst;
}

int LoadTypeFloat__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEFLOAT, dst);
    return dst;
}

int LoadTypeString__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPESTRING, dst);
    return dst;
}

/* TODO remove address operations */
int LoadAddress__(struct Bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_LOADADDR, dst, src);
    return dst;
}

int Dereference__(struct Bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_DEREF, dst, src);
    return dst;
}
/* ------------------------------ */

/* array/struct */
int NewArray__(struct Bytecode *code, uint8_t dst, uint8_t len)
{
    push_inst_ab(code, OP_NEWARRAY, dst, len);
    return dst;
}

int NewStruct__(struct Bytecode *code, uint8_t dst, uint8_t len)
{
    push_inst_ab(code, OP_NEWSTRUCT, dst, len);
    return dst;
}

/* arithmetic */
int AddInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_ADDINT, dst, src0, src1);
    return dst;
}

int AddFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_ADDFLOAT, dst, src0, src1);
    return dst;
}

int SubInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SUBINT, dst, src0, src1);
    return dst;
}

int SubFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SUBFLOAT, dst, src0, src1);
    return dst;
}

int MulInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_MULINT, dst, src0, src1);
    return dst;
}

int MulFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_MULFLOAT, dst, src0, src1);
    return dst;
}

int DivInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_DIVINT, dst, src0, src1);
    return dst;
}

int DivFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_DIVFLOAT, dst, src0, src1);
    return dst;
}

int RemInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_REMINT, dst, src0, src1);
    return dst;
}

int RemFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_REMFLOAT, dst, src0, src1);
    return dst;
}

int EqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQINT, dst, src0, src1);
    return dst;
}

int EqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQFLOAT, dst, src0, src1);
    return dst;
}

int NotEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQINT, dst, src0, src1);
    return dst;
}

int NotEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQFLOAT, dst, src0, src1);
    return dst;
}

int LessInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTINT, dst, src0, src1);
    return dst;
}

int LessFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTFLOAT, dst, src0, src1);
    return dst;
}

int LessEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTEINT, dst, src0, src1);
    return dst;
}

int LessEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTEFLOAT, dst, src0, src1);
    return dst;
}

int GreaterInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTINT, dst, src0, src1);
    return dst;
}

int GreaterFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTFLOAT, dst, src0, src1);
    return dst;
}

int GreaterEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTEINT, dst, src0, src1);
    return dst;
}

int GreaterEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTEFLOAT, dst, src0, src1);
    return dst;
}

int BitwiseAnd__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEAND, dst, src0, src1);
    return dst;
}

int BitwiseOr__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEOR, dst, src0, src1);
    return dst;
}

int BitwiseXor__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEXOR, dst, src0, src1);
    return dst;
}

int BitwiseNot__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_BITWISENOT, dst, src);
    return dst;
}

int ShiftLeft__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SHL, dst, src0, src1);
    return dst;
}

int ShiftRight__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SHR, dst, src0, src1);
    return dst;
}

int NegateInt__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_NEGINT, dst, src);
    return dst;
}

int NegateFloat__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_NEGFLOAT, dst, src);
    return dst;
}

int SetIfZero__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_SETIFZERO, dst, src);
    return dst;
}

int SetIfNotZero__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_SETIFNOTZ, dst, src);
    return dst;
}

int Inc__(struct Bytecode *code, uint8_t src)
{
    push_inst_a(code, OP_INC, src);
    return src;
}

int Dec__(struct Bytecode *code, uint8_t src)
{
    push_inst_a(code, OP_DEC, src);
    return src;
}

/* string */
int ConcatString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_CATSTRING, dst, src0, src1);
    return dst;
}

int EqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQSTRING, dst, src0, src1);
    return dst;
}

int NotEqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQSTRING, dst, src0, src1);
    return dst;
}

/* Function call */
int CallFunction__(Bytecode *code, Byte ret_reg, Word func_index, bool builtin)
{
    int reg0 = ret_reg;

    if (builtin) {
        push_inst_abb(code, OP_CALLBUILTIN, reg0, func_index);
    }
    else {
        push_inst_abb(code, OP_CALL, reg0, func_index);
    }

    return reg0;
}

int CallFunctionPointer__(struct Bytecode *code, int ret, int src)
{
    push_inst_ab(code, OP_CALLPOINTER, ret, src);
    return ret;
}

void Allocate__(Bytecode *code, Byte count)
{
    if (count == 0)
        return;

    push_inst_a(code, OP_ALLOCATE, count);
}

void Return__(Bytecode *code, Byte id)
{
    push_inst_a(code, OP_RETURN, id);
}

/* branch */
void BeginIf__(struct Bytecode *code)
{
    data_intstack_push(&code->ors_, -1);
}

void code_begin_switch(struct Bytecode *code)
{
    data_intstack_push(&code->casecloses_, -1);
}

void PushElseEnd__(struct Bytecode *code, Int addr)
{
    data_intstack_push(&code->ors_, addr);
}

void PushBreak__(struct Bytecode *code, Int addr)
{
    data_intstack_push(&code->breaks_, addr);
}

void PushContinue__(struct Bytecode *code, Int addr)
{
    data_intstack_push(&code->continues_, addr);
}

void PushCaseEnd__(struct Bytecode *code, Int addr)
{
    data_intstack_push(&code->casecloses_, addr);
}

void code_push_continue(struct Bytecode *code, Int addr)
{
    data_intstack_push(&code->continues_, addr);
}

/* jump instructions return the address */
/* where the destination address is stored. */
Int Jump__(struct Bytecode *code, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMP, 0, addr);
    return operand_addr;
}

Int JumpIfZero__(struct Bytecode *code, uint8_t id, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMPIFZERO, id, addr);
    return operand_addr;
}

Int JumpIfNotZero__(struct Bytecode *code, uint8_t id, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMPIFNOTZ, id, addr);
    return operand_addr;
}

/* conversion */
int BoolToInt__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_BOOLTOINT, dst, src);
    return dst;
}

/*
void BoolToFloat__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_BTOF);
}

void IntToBool__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOB);
}

void IntToFloat__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOF);
}

void FloatToBool__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOB);
}

void FloatToInt__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOI);
}
*/

/* program control */
void Exit__(Bytecode *code)
{
    push_inst_op(code, OP_EXIT);
}

void End__(Bytecode *code)
{
    push_inst_op(code, OP_EOC);
}

/* Functions */
void RegisterFunction__(Bytecode *code, Word func_index, Byte argc)
{
    const Word next_index = code->funcs_.len;

    if (func_index != next_index) {
        InternalError(__FILE__, __LINE__,
                "function index %d and next index %d should match\n",
                func_index, next_index);
    }

    const Int next_addr = NextAddr__(code);
    push_info(&code->funcs_, func_index, argc, next_addr);
}

void SetMaxRegisterCount__(Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    code->funcs_.data[func_index].reg_count = code->max_reg + 1;
}

int GetMaxRegisterCount__(const struct Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    return code->funcs_.data[func_index].reg_count;
}

void BeginFor__(struct Bytecode *code)
{
    data_intstack_push(&code->breaks_, -1);
    data_intstack_push(&code->continues_, -1);
}

void BackPatch__(struct Bytecode *code, Int operand_addr)
{
    Int next_addr = NextAddr__(code);
    uint32_t inst = Read__(code, operand_addr);

    inst = (inst & 0xFFFF0000) | (next_addr & 0x0000FFFF);
    Write__(code, operand_addr, inst);
}

void BackPatchBreaks__(struct Bytecode *code)
{
    while (!data_intstack_is_empty(&code->breaks_)) {
        Int addr = data_intstack_pop(&code->breaks_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void BackPatchElseEnds__(struct Bytecode *code)
{
    while (!data_intstack_is_empty(&code->ors_)) {
        Int addr = data_intstack_pop(&code->ors_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void BackPatchContinues__(struct Bytecode *code)
{
    while (!data_intstack_is_empty(&code->continues_)) {
        Int addr = data_intstack_pop(&code->continues_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void code_backpatch_case_ends(Bytecode *code)
{
    while (!data_intstack_is_empty(&code->casecloses_)) {
        Int addr = data_intstack_pop(&code->casecloses_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

static Int print_op__(const Bytecode *code, Int addr, const struct code_instruction *inst, int *imm_size);
void PrintInstruction__(const struct Bytecode *code,
        Int addr, const struct code_instruction *inst, int *imm_size)
{
    print_op__(code, addr, inst, imm_size);
}

uint32_t Read__(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size__(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size__(code));

    return code->insts.data[addr];
}

void Write__(const Bytecode *code, Int addr, uint32_t inst)
{
    if (addr < 0 || addr >= Size__(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size__(code));

    code->insts.data[addr] = inst;
}

Int Size__(const Bytecode *code)
{
    return code->insts.len;
}

Int NextAddr__(const struct Bytecode *code)
{
    return Size__(code);
}

Int GetFunctionAddress(const Bytecode *code, Word func_index)
{
    assert_range(&code->funcs_, func_index);
    return code->funcs_.data[func_index].addr;
}

Int GetFunctionArgCount(const Bytecode *code, Word func_index)
{
    assert_range(&code->funcs_, func_index);
    return code->funcs_.data[func_index].argc;
}

/* XXX TEST */
void print_value(struct runtime_value val, int type)
{
    switch (type) {

    case VAL_INT:
        printf("%lld", val.inum);
        break;

    case VAL_FLOAT:
        if (fmod(val.fpnum, 1.) == 0.0)
            printf("%g.0", val.fpnum);
        else
            printf("%g", val.fpnum);
        break;

    case VAL_STRING:
        printf("\"%s\"", val.str->data);
        break;

    default:
        UNREACHABLE;
        break;
    }
}

void PrintBytecode(const Bytecode *code)
{
    if (code_constant_pool_get_int_count(&code->const_pool) > 0) {
        printf("* constant int:\n");
        int count = code_constant_pool_get_int_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct runtime_value val = code_constant_pool_get_int(&code->const_pool, i);
            printf("[%6d] %lld\n", i, val.inum);
        }
    }

    if (code_constant_pool_get_float_count(&code->const_pool) > 0) {
        printf("* constant float:\n");
        int count = code_constant_pool_get_float_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct runtime_value val = code_constant_pool_get_float(&code->const_pool, i);
            printf("[%6d] %g\n", i, val.fpnum);
        }
    }

    if (code_constant_pool_get_string_count(&code->const_pool) > 0) {
        printf("* constant string:\n");
        int count = code_constant_pool_get_string_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct runtime_value val = code_constant_pool_get_string(&code->const_pool, i);
            printf("[%6d] \"%s\"\n", i, runtime_string_get_cstr(val.str));
        }
    }

    /* function info */
    for (int i = 0; i < code->funcs_.len; i++) {
        const FuncInfo *info = &code->funcs_.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < Size__(code)) {
        const uint32_t instcode = Read__(code, addr);
        struct code_instruction inst = {0};
        int inc = 1;

        code_decode_instruction(instcode, &inst);
        int imm_size = 0;
        PrintInstruction__(code, addr, &inst, &imm_size);
        inc += imm_size;

        if (inst.op == OP_EOC)
            break;

        //addr++;
        addr += inc;

        /* TODO come up with better way */
        const struct code_opcode_info *info = code_lookup_opecode_info(inst.op);
        if (info->extend)
            addr += 2;
    }
}

static void print_operand__(const struct Bytecode *code,
        int addr, int operand, bool separator, int *imm_size)
{
    switch (operand) {

    case IMMEDIATE_INT32:
    case IMMEDIATE_INT64:
        {
            struct runtime_value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("$%lld", val.inum);
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            struct runtime_value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("$%g", val.fpnum);
        }
        break;

    case IMMEDIATE_STRING:
        {
            struct runtime_value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("\"%s\"", runtime_string_get_cstr(val.str));
        }
        break;

    default:
        if (is_smallint_register(operand)) {
            printf("$%d", register_to_smallint(operand));
        }
        else {
            printf("r%d", operand);
        }
        break;
    }

    if (separator)
        printf(", ");
}

static void print_operand16__(const struct Bytecode *code, int operand)
{
    printf("$%d", operand);
}

static Int print_op__(const Bytecode *code, Int addr, const struct code_instruction *inst, int *imm_size)
{
    const struct code_opcode_info *info = code_lookup_opecode_info(inst->op);

    if (addr >= 0)
        printf("[%6lld] ", addr);

    /* padding spaces */
    if (info->operand != OPERAND____)
        printf("%-12s", info->mnemonic);
    else
        printf("%s", info->mnemonic);

    /* append operand */
    switch (info->operand) {

    case OPERAND____:
        break;

    case OPERAND_A__:
        if (inst->op == OP_ALLOCATE)
            print_operand16__(code, inst->A);
        else
            print_operand__(code, addr, inst->A, 0, NULL);
        break;

    case OPERAND_AB_:
        print_operand__(code, addr, inst->A, 1, NULL);
        print_operand__(code, addr, inst->B, 0, imm_size);
        break;

    case OPERAND_ABB:
        print_operand__(code, addr, inst->A, 1, NULL);
        print_operand16__(code, inst->BB);
        break;

    case OPERAND_ABC:
        print_operand__(code, addr, inst->A, 1, imm_size);
        print_operand__(code, addr, inst->B, 1, imm_size);
        print_operand__(code, addr, inst->C, 0, imm_size);
        break;
    }

    if (info->extend) {
        int64_t lo = Read__(code, addr + 1);
        int64_t hi = Read__(code, addr + 2);
        int64_t immediate = (hi << 32) | lo;
        printf(" $%lld", immediate);
    }

    printf("\n");
    return addr;
}
