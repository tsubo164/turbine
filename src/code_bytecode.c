#include "code_bytecode.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

/* immediate value */
#define IMM_MIN   (-32)
#define IMM_MAX   (31)
/* span = 64 */
#define IMM_SPAN  (IMM_MAX - IMM_MIN + 1)
/* register = 0..255 */
#define REG_COUNT (256)
/* 256 - 64 = 192 */
#define IMM_START (REG_COUNT - IMM_SPAN)

/* stackmap mark */
static void mark_ref(struct code_bytecode *code, int slot, bool is_ref)
{
    value_addr_t addr = code_get_next_addr(code);
    code_stackmap_push_current(&code->stackmap, addr);

    code_stackmap_mark(&code->stackmap, slot, is_ref);
}

static int register_to_smallint(int reg);
static void mark_global_ref(struct code_bytecode *code, int slot, bool is_ref)
{
    /* TODO register_to_smallint() will not work bigger global ids */
    int id = register_to_smallint(slot);
    code_globalmap_mark(&code->globalmap, id, is_ref);
}

static void mark_ref2(struct code_bytecode *code, int slot, bool is_ref0, bool is_ref1)
{
    value_addr_t addr = code_get_next_addr(code);
    code_stackmap_push_current(&code->stackmap, addr);

    code_stackmap_mark(&code->stackmap, slot + 0, is_ref0);
    code_stackmap_mark(&code->stackmap, slot + 1, is_ref1);
}

static void mark_ref3(struct code_bytecode *code, int slot, bool is_ref0, bool is_ref1, bool is_ref2)
{
    value_addr_t addr = code_get_next_addr(code);
    code_stackmap_push_current(&code->stackmap, addr);

    code_stackmap_mark(&code->stackmap, slot + 0, is_ref0);
    code_stackmap_mark(&code->stackmap, slot + 1, is_ref1);
    code_stackmap_mark(&code->stackmap, slot + 2, is_ref2);
}

static void mark_ref4(struct code_bytecode *code, int slot, bool is_ref0, bool is_ref1, bool is_ref2, bool is_ref3)
{
    value_addr_t addr = code_get_next_addr(code);
    code_stackmap_push_current(&code->stackmap, addr);

    code_stackmap_mark(&code->stackmap, slot + 0, is_ref0);
    code_stackmap_mark(&code->stackmap, slot + 1, is_ref1);
    code_stackmap_mark(&code->stackmap, slot + 2, is_ref2);
    code_stackmap_mark(&code->stackmap, slot + 3, is_ref3);
}
/* --- */

void code_bytecode_init(struct code_bytecode *code)
{
    struct code_bytecode init = {
        .gc_collect_func_id = -1,
    };

    *code = init;
}

void code_bytecode_clear(struct code_bytecode *code)
{
    /* instructions */
    code_instructionvec_free(&code->insts);

    /* constants */
    code_constant_pool_free(&code->const_pool);

    /* functions */
    code_functionvec_free(&code->funcs);

    /* structs */
    code_structvec_free(&code->strcts);

    /* back patches */
    data_intstack_free(&code->ors);
    data_intstack_free(&code->breaks);
    data_intstack_free(&code->continues);
    data_intstack_free(&code->casecloses);
    data_intstack_free(&code->forrests);

    /* stackmap */
    code_stackmap_free(&code->stackmap);
}

static void push_inst_op(struct code_bytecode *code, int op)
{
    code_push_instruction__(&code->insts, op);
}

static void push_inst_a(struct code_bytecode *code, int op, int a)
{
    code_push_instruction_a(&code->insts, op, a);
}

static void push_inst_ab(struct code_bytecode *code, int op, int a, int b)
{
    code_push_instruction_ab(&code->insts, op, a, b);
}

static void push_inst_abc(struct code_bytecode *code, int op, int a, int b, int c)
{
    code_push_instruction_abc(&code->insts, op, a, b, c);
}

static void push_inst_abb(struct code_bytecode *code, int op, int a, int bb)
{
    code_push_instruction_abb(&code->insts, op, a, bb);
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

    code_stackmap_reset_current(&code->stackmap);
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

/* globals */
void code_set_global_count(struct code_bytecode *code, int count)
{
    assert(count >= 0);
    code->global_count = count;
}

int code_get_global_count(const struct code_bytecode *code)
{
    return code->global_count;
}

/* immediate value */
bool code_is_smallint_register(int reg)
{
    return reg >= IMM_START;
}

static bool can_fit_smallint(value_int_t val)
{
    return val >= IMM_MIN && val <= IMM_MAX;
}

static int register_to_smallint(int reg)
{
    return (reg - IMM_START) + IMM_MIN;
}

static int smallint_to_register(value_int_t val)
{
    return (val - IMM_MIN) + IMM_START;
}

bool code_is_immediate_value(int reg)
{
    return reg >= IMM_START;
}

struct runtime_value code_read_immediate_value(const struct code_bytecode *code,
        value_addr_t addr, int id, int *imm_size)
{
    struct runtime_value value;

    if (code_is_immediate_value(id)) {
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
            int32_t id = code_read(code, addr);
            value = code_constant_pool_get_string(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;
    }

    return value;
}

/* constants */
struct runtime_value code_get_const_value(const struct code_bytecode *code, int id)
{
    return code_constant_pool_get(&code->const_pool, id);
}

int code_get_const_value_type(const struct code_bytecode *code, int id)
{
    return code_constant_pool_get_type(&code->const_pool, id);
}

int code_get_const_value_count(const struct code_bytecode *code)
{
    return code_constant_pool_get_count(&code->const_pool);
}

/* load, store, move */
int code_emit_move(struct code_bytecode *code, int dst, int src)
{
    if (dst == src)
        return dst;

    mark_ref(code, dst, false);
    push_inst_ab(code, OP_MOVE, dst, src);
    return dst;
}

int code_emit_move_ref(struct code_bytecode *code, int dst, int src)
{
    if (dst == src)
        return dst;

    mark_ref(code, dst, true);
    push_inst_ab(code, OP_MOVE, dst, src);
    return dst;
}

int code_emit_load_int(struct code_bytecode *code, value_int_t val)
{
    if (can_fit_smallint(val)) {
        return smallint_to_register(val);
    }
    else {
        int dst = code_allocate_temporary_register(code);
        int id = code_constant_pool_push_int(&code->const_pool, val);
        push_inst_abb(code, OP_LOADCONST, dst, id);
        return dst;
    }
}

int code_emit_load_float(struct code_bytecode *code, value_float_t val)
{
    int dst = code_allocate_temporary_register(code);
    int id = code_constant_pool_push_float(&code->const_pool, val);
    push_inst_abb(code, OP_LOADCONST, dst, id);
    return dst;
}

int code_emit_load_string(struct code_bytecode *code, const char *cstr)
{
    int dst = code_allocate_temporary_register(code);
    int id = code_constant_pool_push_string(&code->const_pool, cstr);
    push_inst_abb(code, OP_LOADCONST, dst, id);
    return dst;
}

int code_emit_load_global(struct code_bytecode *code, int dst, int src)
{
    /* always mark local slot as false because globals are always traced separately */
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_LOADGLOBAL, dst, src);
    return dst;
}

int code_emit_store_global(struct code_bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_STOREGLOBAL, dst, src);
    return dst;
}

int code_emit_store_global_ref(struct code_bytecode *code, int dst, int src)
{
    /* globals marked only when ref */
    mark_global_ref(code, dst, true);
    push_inst_ab(code, OP_STOREGLOBAL, dst, src);
    return dst;
}

int code_emit_load_vec(struct code_bytecode *code, int dst, int src, int idx)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LOADVEC, dst, src, idx);
    return dst;
}

int code_emit_load_vec_ref(struct code_bytecode *code, int dst, int src, int idx)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_LOADVEC, dst, src, idx);
    return dst;
}

int code_emit_store_vec(struct code_bytecode *code, int dst, int idx, int src)
{
    push_inst_abc(code, OP_STOREVEC, dst, idx, src);
    return dst;
}

int code_emit_load_map(struct code_bytecode *code, int dst, int src, int key)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LOADMAP, dst, src, key);
    return dst;
}

int code_emit_load_map_ref(struct code_bytecode *code, int dst, int src, int key)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_LOADMAP, dst, src, key);
    return dst;
}

int code_emit_store_map(struct code_bytecode *code, int dst, int key, int src)
{
    push_inst_abc(code, OP_STOREMAP, dst, key, src);
    return dst;
}

int code_emit_load_struct(struct code_bytecode *code, int dst, int src, int field_idx)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LOADSTRUCT, dst, src, field_idx);
    return dst;
}

int code_emit_load_struct_ref(struct code_bytecode *code, int dst, int src, int field_idx)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_LOADSTRUCT, dst, src, field_idx);
    return dst;
}

int code_emit_store_struct(struct code_bytecode *code, int dst, int field_idx, int src)
{
    push_inst_abc(code, OP_STORESTRUCT, dst, field_idx, src);
    return dst;
}

int code_emit_load_enum(struct code_bytecode *code, int dst, int src, int field_offset)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LOADENUM, dst, src, field_offset);
    return dst;
}

/* vec, map, set, stack, queue, struct */
int code_emit_new_vec(struct code_bytecode *code, int dst, int type, int len)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_NEWVEC, dst, type, len);
    return dst;
}

int code_emit_new_map(struct code_bytecode *code, int dst, int type, int len)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_NEWMAP, dst, type, len);
    return dst;
}

int code_emit_new_set(struct code_bytecode *code, int dst, int type, int len)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_NEWSET, dst, type, len);
    return dst;
}

int code_emit_new_stack(struct code_bytecode *code, int dst, int type, int len)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_NEWSTACK, dst, type, len);
    return dst;
}

int code_emit_new_queue(struct code_bytecode *code, int dst, int type, int len)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_NEWQUEUE, dst, type, len);
    return dst;
}

int code_emit_new_struct(struct code_bytecode *code, int dst, int len)
{
    mark_ref(code, dst, true);
    push_inst_abb(code, OP_NEWSTRUCT, dst, len);
    return dst;
}

/* arithmetic */
int code_emit_add_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_ADDINT, dst, src0, src1);
    return dst;
}

int code_emit_add_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_ADDFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_sub_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_SUBINT, dst, src0, src1);
    return dst;
}

int code_emit_sub_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_SUBFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_mul_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_MULINT, dst, src0, src1);
    return dst;
}

int code_emit_mul_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_MULFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_div_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_DIVINT, dst, src0, src1);
    return dst;
}

int code_emit_div_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_DIVFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_rem_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_REMINT, dst, src0, src1);
    return dst;
}

int code_emit_rem_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_REMFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_EQINT, dst, src0, src1);
    return dst;
}

int code_emit_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_EQFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_NEQINT, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_NEQFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_less_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LTINT, dst, src0, src1);
    return dst;
}

int code_emit_less_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LTFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_less_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LTEINT, dst, src0, src1);
    return dst;
}

int code_emit_less_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_LTEFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_greater_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_GTINT, dst, src0, src1);
    return dst;
}

int code_emit_greater_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_GTFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_greater_equal_int(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_GTEINT, dst, src0, src1);
    return dst;
}

int code_emit_greater_equal_float(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_GTEFLOAT, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_and(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_BITWISEAND, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_or(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_BITWISEOR, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_xor(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_BITWISEXOR, dst, src0, src1);
    return dst;
}

int code_emit_bitwise_not(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_BITWISENOT, dst, src);
    return dst;
}

int code_emit_shift_left(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_SHL, dst, src0, src1);
    return dst;
}

int code_emit_shift_right(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_SHR, dst, src0, src1);
    return dst;
}

int code_emit_negate_int(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_NEGINT, dst, src);
    return dst;
}

int code_emit_negate_float(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_NEGFLOAT, dst, src);
    return dst;
}

int code_emit_set_if_zero(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_SETIFZERO, dst, src);
    return dst;
}

int code_emit_set_if_not_zero(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_SETIFNOTZ, dst, src);
    return dst;
}

/* string */
int code_emit_concat_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, true);
    push_inst_abc(code, OP_CATSTRING, dst, src0, src1);
    return dst;
}

int code_emit_equal_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_EQSTRING, dst, src0, src1);
    return dst;
}

int code_emit_not_equal_string(struct code_bytecode *code, int dst, int src0, int src1)
{
    mark_ref(code, dst, false);
    push_inst_abc(code, OP_NEQSTRING, dst, src0, src1);
    return dst;
}

/* function call */
static int emit_call(struct code_bytecode *code, int ret_reg, int func_index, bool is_native)
{
    if (code->gc_collect_func_id == func_index) {
        /* OP_INTRINSICGC */
        code_emit_intrinsic_gc(code);
        return ret_reg;
    }

    int reg0 = ret_reg;

    if (is_native)
        push_inst_abb(code, OP_CALLNATIVE, reg0, func_index);
    else
        push_inst_abb(code, OP_CALL, reg0, func_index);

    return reg0;
}

int code_emit_call_function(struct code_bytecode *code, int ret_reg,
        int func_index, bool is_native)
{
    mark_ref(code, ret_reg, false);
    return emit_call(code, ret_reg, func_index, is_native);
}

int code_emit_call_function_ref(struct code_bytecode *code, int ret_reg,
        int func_index, bool is_native)
{
    mark_ref(code, ret_reg, true);
    return emit_call(code, ret_reg, func_index, is_native);
}

int code_emit_call_function_pointer(struct code_bytecode *code, int ret_reg, int src)
{
    mark_ref(code, ret_reg, false);
    push_inst_ab(code, OP_CALLPOINTER, ret_reg, src);
    return ret_reg;
}

int code_emit_call_function_pointer_ref(struct code_bytecode *code, int ret_reg, int src)
{
    mark_ref(code, ret_reg, true);
    push_inst_ab(code, OP_CALLPOINTER, ret_reg, src);
    return ret_reg;
}

void code_emit_return(struct code_bytecode *code, int id)
{
    /* return always writes to r0 */
    mark_ref(code, 0, false);
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

void code_push_else_end(struct code_bytecode *code, value_addr_t addr)
{
    data_intstack_push(&code->ors, addr);
}

void code_push_break(struct code_bytecode *code, value_addr_t addr)
{
    data_intstack_push(&code->breaks, addr);
}

void code_push_continue(struct code_bytecode *code, value_addr_t addr)
{
    data_intstack_push(&code->continues, addr);
}

void code_push_case_end(struct code_bytecode *code, value_addr_t addr)
{
    data_intstack_push(&code->casecloses, addr);
}

/*
 * jump and loop instructions return the address
 * where the destination address is stored.
 */
value_addr_t code_emit_jump(struct code_bytecode *code, value_addr_t addr)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMP, 0, addr);
    return operand_addr;
}

value_addr_t code_emit_jump_if_zero(struct code_bytecode *code, int id, value_addr_t addr)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMPIFZERO, id, addr);
    return operand_addr;
}

value_addr_t code_emit_jump_if_not_zero(struct code_bytecode *code, int id, value_addr_t addr)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_JUMPIFNOTZ, id, addr);
    return operand_addr;
}

/* loop */
/* for loop instructions: no need to mark elements of collections, even if they are references,
 * since they will be traced from the collections themselves */
value_addr_t code_emit_fornum_begin(struct code_bytecode *code, int itr)
{
    mark_ref4(code, itr, false, false, false, false);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORNUMBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_fornum_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORNUMEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_forvec_begin(struct code_bytecode *code, int itr)
{
    mark_ref3(code, itr, false, false, true);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORVECBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_forvec_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORVECEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_formap_begin(struct code_bytecode *code, int itr)
{
    mark_ref4(code, itr, false, false, false, true);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORMAPBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_formap_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORMAPEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_forset_begin(struct code_bytecode *code, int itr)
{
    mark_ref3(code, itr, false, false, true);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORSETBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_forset_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORSETEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_forstack_begin(struct code_bytecode *code, int itr)
{
    mark_ref3(code, itr, false, false, true);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORSTACKBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_forstack_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORSTACKEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_forqueue_begin(struct code_bytecode *code, int itr)
{
    mark_ref3(code, itr, false, false, true);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORQUEUEBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_forqueue_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORQUEUEEND, itr, begin);
    return operand_addr;
}

value_addr_t code_emit_forenum_begin(struct code_bytecode *code, int itr)
{
    mark_ref2(code, itr, false, false);
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORENUMBEGIN, itr, -1);
    return operand_addr;
}

value_addr_t code_emit_forenum_end(struct code_bytecode *code, int itr, value_addr_t begin)
{
    value_addr_t operand_addr = code_get_next_addr(code);
    push_inst_abb(code, OP_FORENUMEND, itr, begin);
    return operand_addr;
}

/* conversion */
int code_emit_bool_to_int(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_BOOLTOINT, dst, src);
    return dst;
}

int code_emit_bool_to_float(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_BOOLTOFLOAT, dst, src);
    return dst;
}

int code_emit_int_to_bool(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_INTTOBOOL, dst, src);
    return dst;
}

int code_emit_int_to_float(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_INTTOFLOAT, dst, src);
    return dst;
}

int code_emit_float_to_bool(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_FLOATTOBOOL, dst, src);
    return dst;
}

int code_emit_float_to_int(struct code_bytecode *code, int dst, int src)
{
    mark_ref(code, dst, false);
    push_inst_ab(code, OP_FLOATTOINT, dst, src);
    return dst;
}

/* program control */
void code_emit_intrinsic_gc(struct code_bytecode *code)
{
    push_inst_op(code, OP_INTRINSICGC);
}

void code_emit_safepoint(struct code_bytecode *code)
{
    push_inst_op(code, OP_SAFEPOINTPOLL);
}

void code_emit_halt(struct code_bytecode *code)
{
    push_inst_op(code, OP_HALT);
}

void code_emit_nop(struct code_bytecode *code)
{
    push_inst_op(code, OP_NOP);
}

/* back-patches */
void code_back_patch(struct code_bytecode *code, value_addr_t operand_addr)
{
    value_addr_t next_addr = code_get_next_addr(code);
    int32_t inst = code_read(code, operand_addr);

    inst = (inst & 0xFFFF0000) | (next_addr & 0x0000FFFF);
    code_write(code, operand_addr, inst);
}

void code_back_patch_breaks(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->breaks)) {
        value_addr_t addr = data_intstack_pop(&code->breaks);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_back_patch_continues(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->continues)) {
        value_addr_t addr = data_intstack_pop(&code->continues);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_back_patch_else_ends(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->ors)) {
        value_addr_t addr = data_intstack_pop(&code->ors);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

void code_backpatch_case_ends(struct code_bytecode *code)
{
    while (!data_intstack_is_empty(&code->casecloses)) {
        value_addr_t addr = data_intstack_pop(&code->casecloses);
        if (addr == -1)
            break;
        code_back_patch(code, addr);
    }
}

/* read/write/address */
int32_t code_read(const struct code_bytecode *code, value_addr_t addr)
{
    assert(addr >= 0 && addr < code_get_size(code));

    return code->insts.data[addr];
}

void code_write(const struct code_bytecode *code, value_addr_t addr, int32_t inst)
{
    assert(addr >= 0 && addr < code_get_size(code));

    code->insts.data[addr] = inst;
}

value_int_t code_get_size(const struct code_bytecode *code)
{
    return code->insts.len;
}

value_addr_t code_get_next_addr(const struct code_bytecode *code)
{
    return code_get_size(code);
}

/* functions */
int code_register_function(struct code_bytecode *code, const char *fullname, int argc)
{
    int new_id = code_push_function(&code->funcs, fullname, argc);

    if (!strcmp("gc:collect", fullname)) {
        /* OP_INTRINSICGC */
        code->gc_collect_func_id = new_id;
    }

    return new_id;
}

int code_find_builtin_function(struct code_bytecode *code, const char *name)
{
    for (int i = 0; i < code->funcs.len; i++) {
        const struct code_function *func = code_lookup_const_function(&code->funcs, i);

        if (strncmp("_builtin:", func->fullname, 9))
            continue;
        if (strcmp(name, func->fullname + 9))
            continue;
        return i;
    }

    return -1;
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
        int func_id, native_func_t fp)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->native_func_ptr = fp;
}

native_func_t code_get_native_function_pointer(const struct code_bytecode *code,
        int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->native_func_ptr;
}

void code_set_function_address(struct code_bytecode *code, int func_id, value_addr_t addr)
{
    struct code_function *func = code_lookup_function(&code->funcs, func_id);
    assert(func);

    func->addr = addr;
}

value_addr_t code_get_function_address(const struct code_bytecode *code, int func_id)
{
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    assert(func);

    return func->addr;
}

int code_get_function_arg_count(const struct code_bytecode *code, int func_id)
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

/* structs */
int code_register_struct(struct code_bytecode *code, const char *fullname, int field_count)
{
    int new_id = code_push_struct(&code->strcts, fullname, field_count);

    return new_id;
}

void code_push_struct_field_type(struct code_bytecode *code, int struct_id, int val_type)
{
    struct code_struct *st = code_lookup_struct(&code->strcts, struct_id);
    code_struct_push_value_type(st, val_type);
}

int code_get_struct_field_type(const struct code_bytecode *code, int struct_id, int field_idx)
{
    const struct code_struct *st = code_lookup_const_struct(&code->strcts, struct_id);
    int val_type = code_struct_get_value_type(st, field_idx);

    return val_type;
}

int code_get_struct_field_count(const struct code_bytecode *code, int struct_id)
{
    const struct code_struct *st = code_lookup_const_struct(&code->strcts, struct_id);
    int count = code_struct_get_field_count(st);

    return count;
}

/* enum fields */
int code_push_enum_field_int(struct code_bytecode *code, value_int_t ival)
{
    return code_constant_pool_push_literal_int(&code->const_pool, ival);
}

int code_push_enum_field_float(struct code_bytecode *code, value_float_t fval)
{
    return code_constant_pool_push_literal_float(&code->const_pool, fval);
}

int code_push_enum_field_string(struct code_bytecode *code, const char *sval)
{
    return code_constant_pool_push_literal_string(&code->const_pool, sval);
}

struct runtime_value code_get_enum_field(const struct code_bytecode *code, int id)
{
    return code_constant_pool_get_literal(&code->const_pool, id);
}

bool code_is_enum_field_int(const struct code_bytecode *code, int id)
{
    return code_constant_pool_is_literal_int(&code->const_pool, id);
}

bool code_is_enum_field_float(const struct code_bytecode *code, int id)
{
    return code_constant_pool_is_literal_float(&code->const_pool, id);
}

bool code_is_enum_field_string(const struct code_bytecode *code, int id)
{
    return code_constant_pool_is_literal_string(&code->const_pool, id);
}

int code_get_enum_field_count(const struct code_bytecode *code)
{
    return code_constant_pool_get_literal_count(&code->const_pool);
}
