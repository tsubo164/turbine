#ifndef CODE_BYTECODE_H
#define CODE_BYTECODE_H

#include <stdint.h>
#include <stdbool.h>

#include "code_instruction.h"
#include "code_constant_pool.h"
#include "code_function.h"
#include "runtime_value.h"
#include "runtime_function.h"
#include "data_vec.h"

enum immediate_value_register {
    IMMEDIATE_INT32   = 255,
    IMMEDIATE_INT64   = 254,
    IMMEDIATE_FLOAT   = 253,
    IMMEDIATE_STRING  = 252,
    IMMEDIATE_SMALLINT_END   = 251,
    IMMEDIATE_SMALLINT_BEGIN = 192,
};

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
    struct data_intstack forrests;
};

/* registers */
void code_init_registers(struct code_bytecode *code, int lvar_count);
void code_clear_temporary_registers(struct code_bytecode *code);
int code_allocate_temporary_register(struct code_bytecode *code);
int code_get_register_pointer(const struct code_bytecode *code);
int code_set_register_pointer(struct code_bytecode *code, int dst);
bool code_is_temporary_register(const struct code_bytecode *code, int id);

/* immediate value */
bool code_is_smallint_register(int id);
bool code_is_immediate_value(int id);
struct runtime_value code_read_immediate_value(const struct code_bytecode *code,
        int64_t addr, int id, int *imm_size);

/* allocate */
void code_emit_allocate(struct code_bytecode *code, int count);
void code_emit_allocate_global(struct code_bytecode *code, int count);

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
int code_emit_call_function(struct code_bytecode *code, int ret_reg,
        int func_id, bool builtin);
int code_emit_call_function_pointer(struct code_bytecode *code, int ret, int src);
void code_emit_return(struct code_bytecode *code, int id);

/* branch */
void code_begin_if(struct code_bytecode *code);
void code_begin_for(struct code_bytecode *code);
void code_begin_while(struct code_bytecode *code);
void code_begin_switch(struct code_bytecode *code);
void code_push_else_end(struct code_bytecode *code, int64_t addr);
void code_push_break(struct code_bytecode *code, int64_t addr);
void code_push_continue(struct code_bytecode *code, int64_t addr);
void code_push_case_end(struct code_bytecode *code, int64_t addr);

/*
 * jump and loop instructions return the address
 * where the destination address is stored.
 */
int64_t code_emit_jump(struct code_bytecode *code, int64_t addr);
int64_t code_emit_jump_if_zero(struct code_bytecode *code, int src, int64_t addr);
int64_t code_emit_jump_if_not_zero(struct code_bytecode *code, int src, int64_t addr);

/* loop */
int64_t code_emit_fornum_begin(struct code_bytecode *code, int itr);
int64_t code_emit_fornum_end(struct code_bytecode *code, int itr, int64_t begin);
int64_t code_emit_forarray_begin(struct code_bytecode *code, int itr);
int64_t code_emit_forarray_end(struct code_bytecode *code, int itr, int64_t begin);

/* conversion */
int code_emit_bool_to_int(struct code_bytecode *code, int dst, int src);
int code_emit_bool_to_float(struct code_bytecode *code, int dst, int src);
int code_emit_int_to_bool(struct code_bytecode *code, int dst, int src);
int code_emit_int_to_float(struct code_bytecode *code, int dst, int src);
int code_emit_float_to_bool(struct code_bytecode *code, int dst, int src);
int code_emit_float_to_int(struct code_bytecode *code, int dst, int src);

/* program control */
void code_emit_halt(struct code_bytecode *code);
void code_emit_nop(struct code_bytecode *code);

/* back-patches */
void code_back_patch(struct code_bytecode *code, int64_t operand_addr);
void code_back_patch_breaks(struct code_bytecode *code);
void code_back_patch_continues(struct code_bytecode *code);
void code_back_patch_else_ends(struct code_bytecode *code);
void code_backpatch_case_ends(struct code_bytecode *code);

/* read/write/address */
int32_t code_read(const struct code_bytecode *code, int64_t addr);
void code_write(const struct code_bytecode *code, int64_t addr, int32_t inst);
int64_t code_get_size(const struct code_bytecode *code);
int64_t code_get_next_addr(const struct code_bytecode *code);

/* functions */
/* TODO remove each setter/getter by exposing struct code_function? */
int code_register_function(struct code_bytecode *code, const char *fullname, int argc);

void code_set_function_register_count(struct code_bytecode *code, int func_id);
int code_get_function_register_count(const struct code_bytecode *code, int func_id);

void code_set_native_function_pointer(struct code_bytecode *code,
        int func_id, runtime_native_function_t fp);
runtime_native_function_t code_get_native_function_pointer(
        const struct code_bytecode *code,
        int func_id);

void code_set_function_address(struct code_bytecode *code, int func_id, int64_t addr);
int64_t code_get_function_address(const struct code_bytecode *code, int func_id);
int64_t code_get_function_arg_count(const struct code_bytecode *code, int func_id);
void code_set_function_variadic(struct code_bytecode *code, int func_id, bool is_variadic);
bool code_is_function_variadic(const struct code_bytecode *code, int func_id);

#endif /* _H */
