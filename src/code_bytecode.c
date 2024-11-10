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

static void assert_range(const struct code_functionvec *v,  Word index)
{
    if (index >= v->len) {
        InternalError(__FILE__, __LINE__,
                "function index out of range: %d, function count: %d\n",
                index, v->len);
    }
}

static void push_immediate_value(struct code_bytecode *code, int operand);

static void push_inst_op(struct code_bytecode *code, uint8_t op)
{
    code_push_instruction__(&code->insts, op);
}

static void push_inst_a(struct code_bytecode *code, uint8_t op, uint8_t a)
{
    code_push_instruction_a(&code->insts, op, a);
    push_immediate_value(code, a);
}

static void push_inst_ab(struct code_bytecode *code, uint8_t op, uint8_t a, uint8_t b)
{
    code_push_instruction_ab(&code->insts, op, a, b);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
}

static void push_inst_abc(struct code_bytecode *code, uint8_t op, uint8_t a, uint8_t b, uint8_t c)
{
    code_push_instruction_abc(&code->insts, op, a, b, c);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
    push_immediate_value(code, c);
}

static void push_inst_abb(struct code_bytecode *code, uint8_t op, uint8_t a, uint16_t bb)
{
    code_push_instruction_abb(&code->insts, op, a, bb);
    push_immediate_value(code, a);
}

static bool is_localreg_full(const struct code_bytecode *code)
{
    return code->curr_reg == IMMEDIATE_SMALLINT_BEGIN - 1;
}

void code_init_local_var_registers(struct code_bytecode *code, uint8_t lvar_count)
{
    code->base_reg = lvar_count - 1;
    code->curr_reg = code->base_reg;
    code->max_reg = code->base_reg;
}

void code_clear_temporary_registers(struct code_bytecode *code)
{
    code->curr_reg = code->base_reg;
}

int code_allocate_temporary_register(struct code_bytecode *code)
{
    if (is_localreg_full(code)) {
        fprintf(stderr,
                "error: temp register overflow: keep the temp register under %d\n",
                IMMEDIATE_SMALLINT_BEGIN);
        exit(1);
    }

    return code_set_register_pointer(code, code->curr_reg + 1);
}

int code_get_register_pointer(const struct code_bytecode *code)
{
    return code->curr_reg;
}

int code_set_register_pointer(struct code_bytecode *code, int dst)
{
    assert(dst == code->base_reg || code_is_temporary_register(code, dst));

    code->curr_reg = dst;

    if (code->max_reg < code->curr_reg)
        code->max_reg = code->curr_reg;

    return code->curr_reg;
}

bool code_is_temporary_register(const struct code_bytecode *code, int id)
{
    return id > code->base_reg && !code_is_immediate_value(id);
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

bool code_is_immediate_value(int id)
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

static void push_immediate_value(struct code_bytecode *code, int operand)
{
    if (!is_constpool_register(operand))
        return;

    int64_t val = data_intstack_pop(&code->immediate_ints);
    int32_t id = val & 0xFFFFFFFF;

    code_push_immediate_value(&code->insts, id);
}

struct runtime_value code_read_immediate_value(const struct code_bytecode *code,
        int64_t addr, int id, int *imm_size)
{
    struct runtime_value value;

    if (is_smallint_register(id)) {
        value.inum = register_to_smallint(id);
        return value;
    }

    switch (id) {

    case IMMEDIATE_INT32:
        {
            int32_t imm = code_read(code, addr);
            value.inum = imm;
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_INT64:
        {
            int32_t id = code_read(code, addr);
            value = code_constant_pool_get_int(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            int32_t id = code_read(code, addr);
            value = code_constant_pool_get_float(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_STRING:
        {
            int64_t id = code_read(code, addr);
            value = code_constant_pool_get_string(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;
    }

    return value;
}

/* Load/store/move */
int code_emit_move(struct code_bytecode *code, int dst, int src)
{
    if (dst == src)
        return dst;

    push_inst_ab(code, OP_MOVE, dst, src);
    return dst;
}

int code_emit_load_int(struct code_bytecode *code, int64_t val)
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

int code_emit_load_float(struct code_bytecode *code, double val)
{
    int id = code_constant_pool_push_float(&code->const_pool, val);
    data_intstack_push(&code->immediate_ints, id);
    return IMMEDIATE_FLOAT;
}

int code_emit_load_string(struct code_bytecode *code, const char *cstr)
{
    int id = code_constant_pool_push_string(&code->const_pool, cstr);
    data_intstack_push(&code->immediate_ints, id);
    return IMMEDIATE_STRING;
}

int code_emit_load_global(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_LOAD, dst, src);
    return dst;
}

int code_emit_store_global(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_STORE, dst, src);
    return dst;
}

int code_emit_load_array(struct code_bytecode *code, int dst, int src, int idx)
{
    push_inst_abc(code, OP_LOADARRAY, dst, src, idx);
    return dst;
}

int code_emit_store_array(struct code_bytecode *code, int dst, int idx, int src)
{
    push_inst_abc(code, OP_STOREARRAY, dst, idx, src);
    return dst;
}

int code_emit_load_struct(struct code_bytecode *code, int dst, int src, int field_idx)
{
    push_inst_abc(code, OP_LOADSTRUCT, dst, src, field_idx);
    return dst;
}

int code_emit_store_struct(struct code_bytecode *code, int dst, int field_idx, int src)
{
    push_inst_abc(code, OP_STORESTRUCT, dst, field_idx, src);
    return dst;
}

int code_emit_load_type_nil(struct code_bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPENIL, dst);
    return dst;
}

int code_emit_load_type_bool(struct code_bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEBOOL, dst);
    return dst;
}

int code_emit_load_type_int(struct code_bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEINT, dst);
    return dst;
}

int code_emit_load_type_float(struct code_bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEFLOAT, dst);
    return dst;
}

int code_emit_load_type_string(struct code_bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPESTRING, dst);
    return dst;
}

int code_emit_load_address(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_LOADADDR, dst, src);
    return dst;
}

int code_emit_dereference(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_DEREF, dst, src);
    return dst;
}

/* array/struct */
int code_emit_new_array(struct code_bytecode *code, int dst, int len)
{
    push_inst_ab(code, OP_NEWARRAY, dst, len);
    return dst;
}

int code_emit_new_struct(struct code_bytecode *code, int dst, int len)
{
    push_inst_ab(code, OP_NEWSTRUCT, dst, len);
    return dst;
}

/* arithmetic */
int code_emit_add_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_ADDINT, dst, src0, src1);
    return dst;
}

int code_emit_add_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_ADDFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_sub_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_SUBINT, dst, src0, src1);
    return dst;
}

int code_emit_sub_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_SUBFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_mul_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_MULINT, dst, src0, src1);
    return dst;
}

int code_emit_mul_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_MULFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_div_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_DIVINT, dst, src0, src1);
    return dst;
}

int code_emit_div_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_DIVFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_rem_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_REMINT, dst, src0, src1);
    return dst;
}

int code_emit_rem_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_REMFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_EQINT, dst, src0, src1);
    return dst;
}

int code_emit_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_EQFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_NEQINT, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_NEQFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_less_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_LTINT, dst, src0, src1);
    return dst;
}

int code_emit_less_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_LTFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_less_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_LTEINT, dst, src0, src1);
    return dst;
}

int code_emit_less_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_LTEFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_greater_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_GTINT, dst, src0, src1);
    return dst;
}

int code_emit_greater_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_GTFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_greater_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_GTEINT, dst, src0, src1);
    return dst;
}

int code_emit_greater_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_GTEFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_and(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_BITWISEAND, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_or(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_BITWISEOR, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_xor(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_BITWISEXOR, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_not(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_BITWISENOT, dst, src);
    return dst;
}

int code_emit_shift_left(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_SHL, dst, src0, src1);
    return dst;
}

int code_emit_shift_right(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_SHR, dst, src0, src1);
    return dst;
}

int code_emit_negate_int(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_NEGINT, dst, src);
    return dst;
}

int code_emit_negate_float(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_NEGFLOAT, dst, src);
    return dst;
}

int code_emit_set_if_zero(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_SETIFZERO, dst, src);
    return dst;
}

int code_emit_set_if_not_zero(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_SETIFNOTZ, dst, src);
    return dst;
}

int code_emit_inc(struct code_bytecode *code, int src)
{
    push_inst_a(code, OP_INC, src);
    return src;
}

int code_emit_dec(struct code_bytecode *code, int src)
{
    push_inst_a(code, OP_DEC, src);
    return src;
}

/* string */
int code_emit_concat_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_CATSTRING, dst, src0, src1);
    return dst;
}

int code_emit_equal_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_EQSTRING, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    push_inst_abc(code, OP_NEQSTRING, dst, src0, src1);
    return dst;
}

/* function call */
int code_emit_call_function(struct code_bytecode *code, int ret_reg,
        int func_index, bool builtin)
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

int code_emit_call_function_pointer(struct code_bytecode *code, int ret, int src)
{
    push_inst_ab(code, OP_CALLPOINTER, ret, src);
    return ret;
}

void code_emit_allocate(struct code_bytecode *code, int count)
{
    if (count == 0)
        return;

    push_inst_a(code, OP_ALLOCATE, count);
}

void code_emit_return(struct code_bytecode *code, int id)
{
    push_inst_a(code, OP_RETURN, id);
}

/* branch */
void code_begin_if(struct code_bytecode *code)
{
    data_intstack_push(&code->ors, -1);
}

void code_begin_for(struct code_bytecode *code)
{
    data_intstack_push(&code->breaks, -1);
    data_intstack_push(&code->continues, -1);
}

void code_begin_switch(struct code_bytecode *code)
{
    data_intstack_push(&code->casecloses, -1);
}

void code_push_else_end(struct code_bytecode *code, int64_t addr)
{
    data_intstack_push(&code->ors, addr);
}

void code_push_break(struct code_bytecode *code, int64_t addr)
{
    data_intstack_push(&code->breaks, addr);
}

void code_push_continue(struct code_bytecode *code, int64_t addr)
{
    data_intstack_push(&code->continues, addr);
}

void code_push_case_end(struct code_bytecode *code, int64_t addr)
{
    data_intstack_push(&code->casecloses, addr);
}

/* jump instructions return the address */
/* where the destination address is stored. */
int64_t code_emit_jump(struct code_bytecode *code, int64_t addr)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMP, 0, addr);
    return operand_addr;
}

int64_t code_emit_jump_if_zero(struct code_bytecode *code, int id, int64_t addr)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMPIFZERO, id, addr);
    return operand_addr;
}

int64_t code_emit_jump_if_not_zero(struct code_bytecode *code, int id, int64_t addr)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMPIFNOTZ, id, addr);
    return operand_addr;
}

/* conversion */
int code_emit_bool_to_int(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_BOOLTOINT, dst, src);
    return dst;
}

int code_emit_bool_to_float(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_BOOLTOFLOAT, dst, src);
    return dst;
}

int code_emit_int_to_bool(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_INTTOBOOL, dst, src);
    return dst;
}

int code_emit_int_to_float(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_INTTOFLOAT, dst, src);
    return dst;
}

int code_emit_float_to_bool(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_FLOATTOBOOL, dst, src);
    return dst;
}

int code_emit_float_to_int(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_FLOATTOINT, dst, src);
    return dst;
}

/* program control */
void code_emit_halt(struct code_bytecode *code)
{
    push_inst_op(code, OP_HALT);
}

/* back-patches */
void code_back_patch(struct code_bytecode *code, int64_t operand_addr)
{
    int64_t next_addr = code_get_next_addr(code);
    uint32_t inst = code_read(code, operand_addr);

    inst = (inst & 0xFFFF0000) | (next_addr & 0x0000FFFF);
    code_write(code, operand_addr, inst);
}

void code_back_patch_breaks(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->breaks)) {
        int64_t addr = data_intstack_pop(&code->breaks);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_back_patch_else_ends(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->ors)) {
        int64_t addr = data_intstack_pop(&code->ors);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_back_patch_continues(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->continues)) {
        int64_t addr = data_intstack_pop(&code->continues);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_backpatch_case_ends(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->casecloses)) {
        int64_t addr = data_intstack_pop(&code->casecloses);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

/* read/write/address */
uint32_t code_read(const struct code_bytecode *code, Int addr)
{
    if (addr < 0 || addr >= code_get_size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", code_get_size(code));

    return code->insts.data[addr];
}

void code_write(const struct code_bytecode *code, Int addr, uint32_t inst)
{
    if (addr < 0 || addr >= code_get_size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", code_get_size(code));

    code->insts.data[addr] = inst;
}

int64_t code_get_size(const struct code_bytecode *code)
{
    return code->insts.len;
}

int64_t code_get_next_addr(const struct code_bytecode *code)
{
    return code_get_size(code);
}

/* functions */
void code_register_function(struct code_bytecode *code, int func_index, int argc)
{
    const int next_index = code->funcs.len;

    if (func_index != next_index) {
        InternalError(__FILE__, __LINE__,
                "function index %d and next index %d should match\n",
                func_index, next_index);
    }

    const Int next_addr = code_get_next_addr(code);
    code_push_function(&code->funcs, func_index, argc, next_addr);
}

void code_set_max_register_count(struct code_bytecode *code, int func_index)
{
    if (func_index >= code->funcs.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    code->funcs.data[func_index].reg_count = code->max_reg + 1;
}

int code_get_max_register_count(const struct code_bytecode *code, int func_index)
{
    if (func_index >= code->funcs.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    return code->funcs.data[func_index].reg_count;
}

int64_t code_get_function_address(const struct code_bytecode *code, int func_index)
{
    assert_range(&code->funcs, func_index);
    return code->funcs.data[func_index].addr;
}

int64_t code_get_function_arg_count(const struct code_bytecode *code, int func_index)
{
    assert_range(&code->funcs, func_index);
    return code->funcs.data[func_index].argc;
}

/* print */
static Int print_op__(const struct code_bytecode *code, Int addr, const struct code_instruction *inst, int *imm_size);
void PrintInstruction__(const struct code_bytecode *code,
        Int addr, const struct code_instruction *inst, int *imm_size)
{
    print_op__(code, addr, inst, imm_size);
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

void PrintBytecode(const struct code_bytecode *code)
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
    for (int i = 0; i < code->funcs.len; i++) {
        const struct code_function *info = &code->funcs.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < code_get_size(code)) {
        const uint32_t instcode = code_read(code, addr);
        struct code_instruction inst = {0};
        int inc = 1;

        code_decode_instruction(instcode, &inst);
        int imm_size = 0;
        PrintInstruction__(code, addr, &inst, &imm_size);
        inc += imm_size;

        //addr++;
        addr += inc;

        /* TODO come up with better way */
        const struct code_opcode_info *info = code_lookup_opecode_info(inst.op);
        if (info->extend)
            addr += 2;
    }
}

static void print_operand__(const struct code_bytecode *code,
        int addr, int operand, bool separator, int *imm_size)
{
    switch (operand) {

    case IMMEDIATE_INT32:
    case IMMEDIATE_INT64:
        {
            struct runtime_value val;
            val = code_read_immediate_value(code, addr + 1, operand, imm_size);
            printf("$%lld", val.inum);
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            struct runtime_value val;
            val = code_read_immediate_value(code, addr + 1, operand, imm_size);
            printf("$%g", val.fpnum);
        }
        break;

    case IMMEDIATE_STRING:
        {
            struct runtime_value val;
            val = code_read_immediate_value(code, addr + 1, operand, imm_size);
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

static void print_operand16__(const struct code_bytecode *code, int operand)
{
    printf("$%d", operand);
}

static Int print_op__(const struct code_bytecode *code, Int addr, const struct code_instruction *inst, int *imm_size)
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
        int64_t lo = code_read(code, addr + 1);
        int64_t hi = code_read(code, addr + 2);
        int64_t immediate = (hi << 32) | lo;
        printf(" $%lld", immediate);
    }

    printf("\n");
    return addr;
}
