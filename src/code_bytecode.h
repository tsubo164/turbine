#ifndef CODE_BYTECODE_H
#define CODE_BYTECODE_H

#include <stdbool.h>

#include "code_instruction.h"
#include "code_constant_pool.h"
#include "code_function.h"
#include "native_module.h"
#include "runtime_value.h"
#include "code_stackmap.h"
#include "data_vec.h"

#define IMMEDIATE_QUEUE_SIZE 16

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

    /* globals */
    int global_count;

    /* constants */
    struct code_constant_pool const_pool;
    int immediate_queue[IMMEDIATE_QUEUE_SIZE];
    int qfront;
    int qback;
    int qlen;

    /* functions */
    struct code_functionvec funcs;

    /* back patches */
    struct data_intstack ors;
    struct data_intstack breaks;
    struct data_intstack continues;
    struct data_intstack casecloses;
    struct data_intstack forrests;

    /* TODO */
    struct code_stackmap stackmap;
    struct code_globalmap globalmap;
};

void code_free_bytecode(struct code_bytecode *code);

/* registers */
void code_init_registers(struct code_bytecode *code, int lvar_count);
void code_clear_temporary_registers(struct code_bytecode *code);
int code_allocate_temporary_register(struct code_bytecode *code);
int code_get_register_pointer(const struct code_bytecode *code);
int code_set_register_pointer(struct code_bytecode *code, int dst);
bool code_is_temporary_register(const struct code_bytecode *code, int id);

/* globals */
void code_set_global_count(struct code_bytecode *code, int count);
int code_get_global_count(const struct code_bytecode *code);

/* immediate value */
bool code_is_smallint_register(int id);
bool code_is_immediate_value(int id);
struct runtime_value code_read_immediate_value(const struct code_bytecode *code,
        value_addr_t addr, int id, int *imm_size);

/* load, store, move */
int code_emit_move(struct code_bytecode *code, int dst, int src);
int code_emit_move_ref(struct code_bytecode *code, int dst, int src);
int code_emit_load_int(struct code_bytecode *code, value_int_t val);
int code_emit_load_float(struct code_bytecode *code, value_float_t val);
int code_emit_load_string(struct code_bytecode *code, const char *cstr);
int code_emit_load_global(struct code_bytecode *code, int dst, int src);
int code_emit_store_global(struct code_bytecode *code, int dst, int src);
int code_emit_store_global_ref(struct code_bytecode *code, int dst, int src);
int code_emit_load_vec(struct code_bytecode *code, int dst, int src, int idx);
int code_emit_store_vec(struct code_bytecode *code, int dst, int idx, int src);
int code_emit_load_map(struct code_bytecode *code, int dst, int src, int key);
int code_emit_store_map(struct code_bytecode *code, int dst, int key, int src);
int code_emit_load_struct(struct code_bytecode *code, int dst, int src, int field_idx);
int code_emit_store_struct(struct code_bytecode *code, int dst, int field_idx, int src);
int code_emit_load_enum(struct code_bytecode *code, int dst, int src, int field_offset);

/* vec, map, set, stack, queue, struct */
int code_emit_new_vec(struct code_bytecode *code, int dst, int type, int len);
int code_emit_new_map(struct code_bytecode *code, int dst, int len);
int code_emit_new_set(struct code_bytecode *code, int dst, int type, int len);
int code_emit_new_stack(struct code_bytecode *code, int dst, int type, int len);
int code_emit_new_queue(struct code_bytecode *code, int dst, int type, int len);
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

/* string */
int code_emit_concat_string(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_equal_string(struct code_bytecode *code, int dst, int src0, int src1);
int code_emit_not_equal_string(struct code_bytecode *code, int dst, int src0, int src1);

/* function call */
int code_emit_call_function(struct code_bytecode *code, int ret_reg,
        int func_id, bool is_native);
int code_emit_call_function_pointer(struct code_bytecode *code, int ret, int src);
void code_emit_return(struct code_bytecode *code, int id);

/* branch */
void code_begin_if(struct code_bytecode *code);
void code_begin_for(struct code_bytecode *code);
void code_begin_while(struct code_bytecode *code);
void code_begin_switch(struct code_bytecode *code);
void code_push_else_end(struct code_bytecode *code, value_addr_t addr);
void code_push_break(struct code_bytecode *code, value_addr_t addr);
void code_push_continue(struct code_bytecode *code, value_addr_t addr);
void code_push_case_end(struct code_bytecode *code, value_addr_t addr);

/*
 * jump and loop instructions return the address
 * where the destination address is stored.
 */
value_addr_t code_emit_jump(struct code_bytecode *code, value_addr_t addr);
value_addr_t code_emit_jump_if_zero(struct code_bytecode *code, int src, value_addr_t addr);
value_addr_t code_emit_jump_if_not_zero(struct code_bytecode *code, int src, value_addr_t addr);

/* loop */
value_addr_t code_emit_fornum_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_fornum_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_forvec_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_forvec_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_formap_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_formap_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_forset_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_forset_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_forstack_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_forstack_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_forqueue_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_forqueue_end(struct code_bytecode *code, int itr, value_addr_t begin);
value_addr_t code_emit_forenum_begin(struct code_bytecode *code, int itr);
value_addr_t code_emit_forenum_end(struct code_bytecode *code, int itr, value_addr_t begin);

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
void code_back_patch(struct code_bytecode *code, value_addr_t operand_addr);
void code_back_patch_breaks(struct code_bytecode *code);
void code_back_patch_continues(struct code_bytecode *code);
void code_back_patch_else_ends(struct code_bytecode *code);
void code_backpatch_case_ends(struct code_bytecode *code);

/* read/write/address */
int32_t code_read(const struct code_bytecode *code, value_addr_t addr);
void code_write(const struct code_bytecode *code, value_addr_t addr, int32_t inst);
value_int_t code_get_size(const struct code_bytecode *code);
value_addr_t code_get_next_addr(const struct code_bytecode *code);

/* functions */
/* TODO remove each setter/getter by exposing struct code_function? */
int code_register_function(struct code_bytecode *code, const char *fullname, int argc);
int code_find_builtin_function(struct code_bytecode *code, const char *name);

void code_set_function_register_count(struct code_bytecode *code, int func_id);
int code_get_function_register_count(const struct code_bytecode *code, int func_id);

void code_set_native_function_pointer(struct code_bytecode *code,
        int func_id, native_func_t fp);
native_func_t code_get_native_function_pointer(const struct code_bytecode *code,
        int func_id);

void code_set_function_address(struct code_bytecode *code, int func_id, value_addr_t addr);
value_addr_t code_get_function_address(const struct code_bytecode *code, int func_id);
int code_get_function_arg_count(const struct code_bytecode *code, int func_id);
void code_set_function_variadic(struct code_bytecode *code, int func_id, bool is_variadic);
bool code_is_function_variadic(const struct code_bytecode *code, int func_id);

/* enum fields */
int code_push_enum_field_int(struct code_bytecode *code, value_int_t ival);
int code_push_enum_field_float(struct code_bytecode *code, value_float_t fval);
int code_push_enum_field_string(struct code_bytecode *code, const char *sval);
struct runtime_value code_get_enum_field(const struct code_bytecode *code, int id);

bool code_is_enum_field_int(const struct code_bytecode *code, int id);
bool code_is_enum_field_float(const struct code_bytecode *code, int id);
bool code_is_enum_field_string(const struct code_bytecode *code, int id);
int code_get_enum_field_count(const struct code_bytecode *code);

#endif /* _H */
