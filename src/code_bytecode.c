#include "code_bytecode.h"

#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

static void push_immediate_value(struct code_bytecode *code, int operand);

static void push_inst_op(struct code_bytecode *code, int op)
{
    code_push_instruction__(&code->insts, op);
}

static void push_inst_a(struct code_bytecode *code, int op, int a)
{
    code_push_instruction_a(&code->insts, op, a);
    push_immediate_value(code, a);
}

static void push_inst_ab(struct code_bytecode *code, int op, int a, int b)
{
    code_push_instruction_ab(&code->insts, op, a, b);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
}

static void push_inst_abc(struct code_bytecode *code, int op, int a, int b, int c)
{
    code_push_instruction_abc(&code->insts, op, a, b, c);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
    push_immediate_value(code, c);
}

static void push_inst_abb(struct code_bytecode *code, int op, int a, int bb)
{
    code_push_instruction_abb(&code->insts, op, a, bb);
    push_immediate_value(code, a);
}

static bool is_localreg_full(const struct code_bytecode *code)
{
    return code->curr_reg == IMMEDIATE_SMALLINT_BEGIN - 1;
}

void code_init_registers(struct code_bytecode *code, int lvar_count)
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

bool code_is_smallint_register(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN && id <= IMMEDIATE_SMALLINT_END;
}

static bool can_fit_smallint(int64_t val)
{
    int SMALLINT_SIZE = IMMEDIATE_SMALLINT_END - IMMEDIATE_SMALLINT_BEGIN + 1;
    return val >= 0 && val < SMALLINT_SIZE;
}

static int register_to_smallint(int id)
{
    return id - IMMEDIATE_SMALLINT_BEGIN;
}

static int smallint_to_register(int64_t val)
{
    return val + IMMEDIATE_SMALLINT_BEGIN;
}

static bool is_constpool_register(int id)
{
    return id > IMMEDIATE_SMALLINT_END && id <= 0xFF;
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

    if (code_is_smallint_register(id)) {
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

/* allocate */
void code_emit_allocate(struct code_bytecode *code, int count)
{
    if (count == 0)
        return;

    push_inst_a(code, OP_ALLOCATE, count);
}

void code_emit_allocate_global(struct code_bytecode *code, int count)
{
    if (count == 0)
        return;

    push_inst_abb(code, OP_ALLOCGLOBAL, 0, count);
}

/* load/store/move */
int code_emit_move(struct code_bytecode *code, int dst, int src)
{
    if (dst == src)
        return dst;

    push_inst_ab(code, OP_MOVE, dst, src);
    return dst;
}

static bool can_fit_int32(int64_t val)
{
    return val >= INT32_MIN && val <= INT32_MAX;
}

bool code_is_immediate_value(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN;
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
    push_inst_ab(code, OP_LOADGLOBAL, dst, src);
    return dst;
}

int code_emit_store_global(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_STOREGLOBAL, dst, src);
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
    push_inst_abb(code, OP_LOADTYPEID, dst, VAL_NIL);
    return dst;
}

int code_emit_load_type_bool(struct code_bytecode *code, int dst)
{
    push_inst_abb(code, OP_LOADTYPEID, dst, VAL_BOOL);
    return dst;
}

int code_emit_load_type_int(struct code_bytecode *code, int dst)
{
    push_inst_abb(code, OP_LOADTYPEID, dst, VAL_INT);
    return dst;
}

int code_emit_load_type_float(struct code_bytecode *code, int dst)
{
    push_inst_abb(code, OP_LOADTYPEID, dst, VAL_FLOAT);
    return dst;
}

int code_emit_load_type_string(struct code_bytecode *code, int dst)
{
    push_inst_abb(code, OP_LOADTYPEID, dst, VAL_STRING);
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
    push_inst_abb(code, OP_NEWSTRUCT, dst, len);
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

void code_begin_while(struct code_bytecode *code)
{
    data_intstack_push(&code->breaks, -1);
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

/*
 * jump and loop instructions return the address
 * where the destination address is stored.
 */
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

/* loop */
int64_t code_emit_fornum_begin(struct code_bytecode *code, int itr)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORNUMBEGIN, itr, -1);
    return operand_addr;
}

int64_t code_emit_fornum_end(struct code_bytecode *code, int itr, int64_t begin)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORNUMEND, itr, begin);
    return operand_addr;
}

int64_t code_emit_forarray_begin(struct code_bytecode *code, int itr)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORARRAYBEGIN, itr, -1);
    return operand_addr;
}

int64_t code_emit_forarray_end(struct code_bytecode *code, int itr, int64_t begin)
{
    int64_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORARRAYEND, itr, begin);
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

void code_emit_nop(struct code_bytecode *code)
{
    push_inst_op(code, OP_NOP);
}

/* back-patches */
void code_back_patch(struct code_bytecode *code, int64_t operand_addr)
{
    int64_t next_addr = code_get_next_addr(code);
    int32_t inst = code_read(code, operand_addr);

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

void code_back_patch_continues(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->continues)) {
        int64_t addr = data_intstack_pop(&code->continues);
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
int32_t code_read(const struct code_bytecode *code, int64_t addr)
{
    assert(addr >= 0 && addr < code_get_size(code));

    return code->insts.data[addr];
}

void code_write(const struct code_bytecode *code, int64_t addr, int32_t inst)
{
    assert(addr >= 0 && addr < code_get_size(code));

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
int code_register_function(struct code_bytecode *code, const char *fullname, int argc)
{
    int new_id = code_push_function(&code->funcs, fullname, argc);

    return new_id;
}

void code_set_function_register_count(struct code_bytecode *code, int func_id)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->reg_count = code->max_reg + 1;
}

int code_get_function_register_count(const struct code_bytecode *code, int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->reg_count;
}

void code_set_native_function_pointer(struct code_bytecode *code,
        int func_id, runtime_native_function_t fp)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->native_func_ptr = fp;
}

runtime_native_function_t code_get_native_function_pointer(
        const struct code_bytecode *code,
        int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->native_func_ptr;
}

void code_set_function_address(struct code_bytecode *code, int func_id, int64_t addr)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->addr = addr;
}

int64_t code_get_function_address(const struct code_bytecode *code, int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->addr;
}

int64_t code_get_function_arg_count(const struct code_bytecode *code, int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->argc;
}

void code_set_function_variadic(struct code_bytecode *code, int func_id, bool is_variadic)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->is_variadic = is_variadic;
}

bool code_is_function_variadic(const struct code_bytecode *code, int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->is_variadic;
}
