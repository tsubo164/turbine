#include "code_generate.h"
#include "code_bytecode.h"
#include "parser_symbol.h"
#include "parser_eval.h"
#include "parser_type.h"
#include "parser_ast.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>

static int gen_dst_register1(struct code_bytecode *code, int reg1)
{
    /* determine the destination register */
    if (code_is_temporary_register(code, reg1))
        return reg1;

    return code_allocate_temporary_register(code);
}

static int gen_dst_register2(struct code_bytecode *code, int reg1, int reg2)
{
    /* determine the destination register */
    if (code_is_temporary_register(code, reg1))
        return reg1;

    if (code_is_temporary_register(code, reg2))
        return reg2;

    return code_allocate_temporary_register(code);
}

static int gen_expr(struct code_bytecode *code, const struct parser_expr *e);
static int gen_addr(struct code_bytecode *code, const struct parser_expr *e);

static int gen_convert(struct code_bytecode *code, const struct parser_expr *e)
{
    int src = gen_expr(code, e->l);
    int dst = gen_dst_register1(code, src);
    int from = e->l->type->kind;
    int to = e->type->kind;

    switch (from) {

    case TYP_BOOL:
        switch (to) {

        case TYP_BOOL:
            return src;

        case TYP_INT:
            return code_emit_bool_to_int(code, dst, src);

        case TYP_FLOAT:
            return code_emit_bool_to_float(code, dst, src);
        }
        break;

    case TYP_INT:
        switch (to) {

        case TYP_BOOL:
            return code_emit_int_to_bool(code, dst, src);

        case TYP_INT:
            return src;

        case TYP_FLOAT:
            return code_emit_int_to_float(code, dst, src);
        }
        break;

    case TYP_FLOAT:
        switch (to) {

        case TYP_BOOL:
            return code_emit_float_to_bool(code, dst, src);

        case TYP_INT:
            return code_emit_float_to_int(code, dst, src);

        case TYP_FLOAT:
            return src;
        }
        break;
    }

    return src;
}

static int parser_type_to_value_type(const struct parser_type *t)
{
    switch ((enum parser_type_kind) t->kind) {
    case TYP_NIL:      return VAL_NIL;
    case TYP_BOOL:     return VAL_INT;
    case TYP_INT:      return VAL_INT;
    case TYP_FLOAT:    return VAL_FLOAT;
    case TYP_STRING:   return VAL_STRING;
    case TYP_FUNC:     return VAL_NIL;
    case TYP_VEC:      return VAL_VEC;
    case TYP_MAP:      return VAL_MAP;
    case TYP_SET:      return VAL_SET;
    case TYP_STACK:    return VAL_STACK;
    case TYP_QUEUE:    return VAL_QUEUE;
    case TYP_STRUCT:   return VAL_STRUCT;
    case TYP_ENUM:     return VAL_NIL;
    case TYP_MODULE:   return VAL_NIL;
    case TYP_ANY:      return VAL_NIL;
    case TYP_TEMPLATE: return VAL_NIL;
    }
    return VAL_NIL;
}

static int gen_vec_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int index = 0;
    int len = 0;
    int dst = 0;
    int val_type = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make vec */
    val_type = parser_type_to_value_type(e->type->underlying);
    len = gen_expr(code, e->l);
    code_emit_new_vec(code, dst, val_type, len);

    /* set elements */
    for (elem = e->r; elem; elem = elem->next) {
        int src = gen_expr(code, elem);
        int idx = code_emit_load_int(code, index);
        code_emit_store_vec(code, dst, idx, src);
        index++;
    }

    return dst;
}

static int gen_map_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int len = 0;
    int dst = 0;
    int val_type = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make map */
    val_type = parser_type_to_value_type(e->type->underlying);
    len = gen_expr(code, e->l);
    code_emit_new_map(code, dst, val_type, len);

    /* set elements */
    for (elem = e->r; elem; elem = elem->next) {
        int key = gen_expr(code, elem->l);
        int src = gen_expr(code, elem->r);
        code_emit_store_map(code, dst, key, src);
    }

    return dst;
}

static int gen_set_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int len = 0;
    int dst = 0;
    int val_type = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make set */
    val_type = parser_type_to_value_type(e->type->underlying);
    len = gen_expr(code, e->l);
    code_emit_new_set(code, dst, val_type, len);

    /* set elements */
    /* TODO remove code_find_builtin_function() when OP_SETADD available */
    int func_id = code_find_builtin_function(code, "setadd");
    bool is_native = true;
    int ret_reg = code_allocate_temporary_register(code);
    int src_reg = code_allocate_temporary_register(code);
    for (elem = e->r; elem; elem = elem->next) {
        code_emit_move(code, ret_reg, dst);
        int src = gen_expr(code, elem);
        code_emit_move(code, src_reg, src);
        code_emit_call_function(code, ret_reg, func_id, is_native);
    }

    return dst;
}

static int gen_stack_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int len = 0;
    int dst = 0;
    int val_type = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make stack */
    val_type = parser_type_to_value_type(e->type->underlying);
    val_type = code_emit_load_int(code, val_type);
    len = gen_expr(code, e->l);
    code_emit_new_stack(code, dst, val_type, len);

    /* set elements */
    /* TODO remove code_find_builtin_function() when OP_STACKPUSH available */
    int func_id = code_find_builtin_function(code, "stackpush");
    bool is_native = true;
    int ret_reg = code_allocate_temporary_register(code);
    int src_reg = code_allocate_temporary_register(code);
    for (elem = e->r; elem; elem = elem->next) {
        code_emit_move(code, ret_reg, dst);
        int src = gen_expr(code, elem);
        code_emit_move(code, src_reg, src);
        code_emit_call_function(code, ret_reg, func_id, is_native);
    }

    return dst;
}

static int gen_queue_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int len = 0;
    int dst = 0;
    int val_type = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make queue */
    val_type = parser_type_to_value_type(e->type->underlying);
    val_type = code_emit_load_int(code, val_type);
    len = gen_expr(code, e->l);
    code_emit_new_queue(code, dst, val_type, len);

    /* set elements */
    /* TODO remove code_find_builtin_function() when OP_QUEUEPUSH available */
    int func_id = code_find_builtin_function(code, "queuepush");
    bool is_native = true;
    int ret_reg = code_allocate_temporary_register(code);
    int src_reg = code_allocate_temporary_register(code);
    for (elem = e->r; elem; elem = elem->next) {
        code_emit_move(code, ret_reg, dst);
        int src = gen_expr(code, elem);
        code_emit_move(code, src_reg, src);
        code_emit_call_function(code, ret_reg, func_id, is_native);
    }

    return dst;
}

static int gen_struct_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int dst = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make struct */
    int id = e->type->strct->id;
    code_emit_new_struct(code, dst, id);

    /* set elements */
    for (elem = e->l; elem; elem = elem->next) {
        int idx = gen_addr(code, elem->l);
        int src = gen_expr(code, elem->r);
        code_emit_store_struct(code, dst, idx, src);
    }

    return dst;
}

#define BINOP(code, ty, op, r0, r1, r2) \
    do { \
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty))) \
        code_emit_##op##_int((code), (r0), (r1), (r2)); \
    else if (parser_is_float_type((ty))) \
        code_emit_##op##_float((code), (r0), (r1), (r2)); \
    } while (0)

#define BINOP_S(code, ty, op, ops, r0, r1, r2) \
    do { \
    if (parser_is_int_type((ty)) || parser_is_bool_type((ty)) || parser_is_enum_type((ty))) \
        code_emit_##op##_int((code), (r0), (r1), (r2)); \
    else if (parser_is_float_type((ty))) \
        code_emit_##op##_float((code), (r0), (r1), (r2)); \
    else if (parser_is_string_type((ty))) \
        code_emit_##ops##_string((code), (r0), (r1), (r2)); \
    } while (0)

static int gen_binop(struct code_bytecode *code, const struct parser_type *type, int kind,
        int reg0, int reg1, int reg2)
{
    switch (kind) {
    case NOD_EXPR_ADD:
    case NOD_EXPR_ADDASSIGN:
        BINOP_S(code, type, add, concat, reg0, reg1, reg2);
        break;

    case NOD_EXPR_SUB:
    case NOD_EXPR_SUBASSIGN:
        BINOP(code, type, sub, reg0, reg1, reg2);
        break;

    case NOD_EXPR_MUL:
    case NOD_EXPR_MULASSIGN:
        BINOP(code, type, mul, reg0, reg1, reg2);
        break;

    case NOD_EXPR_DIV:
    case NOD_EXPR_DIVASSIGN:
        BINOP(code, type, div, reg0, reg1, reg2);
        break;

    case NOD_EXPR_REM:
    case NOD_EXPR_REMASSIGN:
        BINOP(code, type, rem, reg0, reg1, reg2);
        break;

    case NOD_EXPR_AND:
    case NOD_EXPR_ANDASSIGN:
        code_emit_bitwise_and(code, reg0, reg1, reg2);
        break;

    case NOD_EXPR_OR:
    case NOD_EXPR_ORASSIGN:
        code_emit_bitwise_or(code, reg0, reg1, reg2);
        break;

    case NOD_EXPR_XOR:
    case NOD_EXPR_XORASSIGN:
        code_emit_bitwise_xor(code, reg0, reg1, reg2);
        break;

    case NOD_EXPR_SHL:
    case NOD_EXPR_SHLASSIGN:
        code_emit_shift_left(code, reg0, reg1, reg2);
        break;

    case NOD_EXPR_SHR:
    case NOD_EXPR_SHRASSIGN:
        code_emit_shift_right(code, reg0, reg1, reg2);
        break;

    case NOD_EXPR_EQ:
        BINOP_S(code, type, equal, equal, reg0, reg1, reg2);
        break;

    case NOD_EXPR_NEQ:
        BINOP_S(code, type, not_equal, not_equal, reg0, reg1, reg2);
        break;

    case NOD_EXPR_LT:
        BINOP(code, type, less, reg0, reg1, reg2);
        break;

    case NOD_EXPR_LTE:
        BINOP(code, type, less_equal, reg0, reg1, reg2);
        break;

    case NOD_EXPR_GT:
        BINOP(code, type, greater, reg0, reg1, reg2);
        break;

    case NOD_EXPR_GTE:
        BINOP(code, type, greater_equal, reg0, reg1, reg2);
        break;

    }
    return reg0;
}

/* emit_ wrappers */
static bool is_ref(const struct parser_type *type)
{
    if (parser_is_nil_type(type) ||
        parser_is_bool_type(type) ||
        parser_is_int_type(type) ||
        parser_is_float_type(type) ||
        parser_is_func_type(type) ||
        parser_is_enum_type(type) ||
        parser_is_any_type(type)) {
        return false;
    }
    else {
        return true;
    }
}

static int emit_move(struct code_bytecode *code, int dst, int src, const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_move_ref(code, dst, src);
    else
        return code_emit_move(code, dst, src);
}

static int emit_load_vec(struct code_bytecode *code, int dst, int src, int idx, const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_load_vec_ref(code, dst, src, idx);
    else
        return code_emit_load_vec(code, dst, src, idx);
}

static int emit_load_map(struct code_bytecode *code, int dst, int src, int idx, const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_load_map_ref(code, dst, src, idx);
    else
        return code_emit_load_map(code, dst, src, idx);
}

static int emit_load_struct(struct code_bytecode *code, int dst, int src, int idx, const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_load_struct_ref(code, dst, src, idx);
    else
        return code_emit_load_struct(code, dst, src, idx);
}

static int emit_call_function(struct code_bytecode *code, int ret_reg, int func_id, bool is_native,
        const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_call_function_ref(code, ret_reg, func_id, is_native);
    else
        return code_emit_call_function(code, ret_reg, func_id, is_native);
}

static int emit_call_function_pointer(struct code_bytecode *code, int ret_reg, int src,
        const struct parser_type *type)
{
    if (is_ref(type))
        return code_emit_call_function_pointer_ref(code, ret_reg, src);
    else
        return code_emit_call_function_pointer(code, ret_reg, src);
}

/* emit_ wrappers */

static int gen_assign(struct code_bytecode *code, const struct parser_expr *e)
{
    const struct parser_expr *lval = e->l;
    const struct parser_expr *rval = e->r;
    /* for a = x, evaluate x first, update a next */

    if (lval->kind == NOD_EXPR_INDEX) {
        /* a[b] = x */
        int src = gen_expr(code, rval);
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        return code_emit_store_vec(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_MAPINDEX) {
        /* a[b] = x */
        int src = gen_expr(code, rval);
        int obj = gen_expr(code, lval->l);
        int key = gen_expr(code, lval->r);
        return code_emit_store_map(code, obj, key, src);
    }

    if (lval->kind == NOD_EXPR_STRUCTACCESS) {
        /* a.b = x */
        int src = gen_expr(code, rval);
        int obj = gen_expr(code, lval->l);
        int fld = gen_addr(code, lval->r);
        return code_emit_store_struct(code, obj, fld, src);
    }

    if (lval->kind == NOD_EXPR_MODULEACCESS) {
        /* m.a = x */
        int src = gen_expr(code, rval);
        int dst = gen_addr(code, lval);
        return code_emit_store_global(code, dst, src);
    }

    if (parser_ast_is_global(lval)) {
        /* _a_ = x */
        int src = gen_expr(code, rval);
        int dst = gen_addr(code, lval);
        return code_emit_store_global(code, dst, src);
    }

    if (parser_ast_is_outparam(lval)) {
        /* &a = x */
        int src = gen_expr(code, rval);
        int dst = gen_addr(code, lval);
        return code_emit_store_indirect(code, dst, src);
    }

    /* check the rvalue expression to see if `a = x + y` can be applied */
    /* e.g. a = x + y */
    /*            ^ here */
    switch (rval->kind) {
    case NOD_EXPR_ADD:
    case NOD_EXPR_SUB:
    case NOD_EXPR_MUL:
    case NOD_EXPR_DIV:
    case NOD_EXPR_REM:
    case NOD_EXPR_AND:
    case NOD_EXPR_OR:
    case NOD_EXPR_XOR:
    case NOD_EXPR_SHL:
    case NOD_EXPR_SHR:
    case NOD_EXPR_EQ:
    case NOD_EXPR_NEQ:
    case NOD_EXPR_LT:
    case NOD_EXPR_LTE:
    case NOD_EXPR_GT:
    case NOD_EXPR_GTE:
        {
            /* a = x + y */
            int src1 = gen_expr(code, rval->l);
            int src2 = gen_expr(code, rval->r);
            int dst = gen_addr(code, lval);
            return gen_binop(code, e->type, rval->kind, dst, src1, src2);
        }
    }
    {
        /* a = x */
        int src = gen_expr(code, rval);
        int dst = gen_addr(code, lval);
        return emit_move(code, dst, src, lval->type);
    }

    return -1;
}

static int gen_binop_assign(struct code_bytecode *code, const struct parser_expr *e)
{
    const struct parser_expr *lval = e->l;
    const struct parser_expr *rval = e->r;
    /* for a += x, evaluate x first, update a next */

    if (lval->kind == NOD_EXPR_INDEX) {
        /* a[b] += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_vec(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_MAPINDEX) {
        /* a[b] += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_map(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_STRUCTACCESS) {
        /* a.b += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        int obj = gen_expr(code, lval->l);
        int fld = gen_addr(code, lval->r);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_struct(code, obj, fld, src);
    }

    if (parser_ast_is_global(lval)) {
        /* _a_ += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        int dst = gen_addr(code, lval);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_global(code, dst, src);
    }

    if (parser_ast_is_outparam(lval)) {
        /* &a += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        int dst = gen_addr(code, lval);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_indirect(code, dst, src);
    }
    {
        /* a += x */
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int dst = gen_addr(code, lval);
        return gen_binop(code, e->type, e->kind, dst, tmp1, tmp2);
    }
}

static int gen_init(struct code_bytecode *code, const struct parser_expr *e)
{
    if (parser_ast_is_global(e->l)) {
        int dst = gen_addr(code, e->l);
        int tmp = gen_expr(code, e->r);
        /* gloablmap needs to be set only when init */
        if (is_ref(e->type))
            return code_emit_store_global_ref(code, dst, tmp);
        else
            return code_emit_store_global(code, dst, tmp);
    }

    if (e->r->kind == NOD_EXPR_VECLIT) {
        int dst = gen_addr(code, e->l);
        return gen_vec_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_MAPLIT) {
        int dst = gen_addr(code, e->l);
        return gen_map_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_SETLIT) {
        int dst = gen_addr(code, e->l);
        return gen_set_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_STACKLIT) {
        int dst = gen_addr(code, e->l);
        return gen_stack_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_QUEUELIT) {
        int dst = gen_addr(code, e->l);
        return gen_queue_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_STRUCTLIT) {
        int dst = gen_addr(code, e->l);
        return gen_struct_lit(code, e->r, dst);
    }

    return gen_assign(code, e);
}

static int gen_call(struct code_bytecode *code, const struct parser_expr *call)
{
    const struct parser_func_sig *func_sig = call->l->type->func_sig;

    /* save the current register right before evaluating args */
    int curr_reg = code_get_register_pointer(code);
    int retval_reg = code_set_register_pointer(code, curr_reg + 1);

    int reg_ptr = curr_reg;

    /* args */
    for (const struct parser_expr *arg = call->r; arg; arg = arg->next) {
        int src = gen_expr(code, arg);
        int dst = code_set_register_pointer(code, ++reg_ptr);
        code_emit_move(code, dst, src);
    }

    /* call */
    value_int_t func_id = 0;
    if (parser_eval_expr(call->l, &func_id)) {
        bool is_native = func_sig->is_native;
        const struct parser_type *return_type = func_sig->return_type;
        emit_call_function(code, retval_reg, func_id, is_native, return_type);
    }
    else {
        int src = gen_expr(code, call->l);
        const struct parser_type *return_type = func_sig->return_type;
        emit_call_function_pointer(code, retval_reg, src, return_type);
    }

    return code_set_register_pointer(code, retval_reg);
}

static int gen_expr(struct code_bytecode *code, const struct parser_expr *e)
{
    if (!e)
        return -1;

    switch (e->kind) {

    case NOD_EXPR_NILLIT:
        return code_emit_load_int(code, 0);

    case NOD_EXPR_BOOLLIT:
    case NOD_EXPR_INTLIT:
        return code_emit_load_int(code, e->ival);

    case NOD_EXPR_FLOATLIT:
        return code_emit_load_float(code, e->fval);

    case NOD_EXPR_STRINGLIT:
        return code_emit_load_string(code, e->sval);

    case NOD_EXPR_VECLIT:
        return gen_vec_lit(code, e, -1);

    case NOD_EXPR_STRUCTLIT:
        return gen_struct_lit(code, e, -1);

    case NOD_EXPR_ENUMLIT:
        {
            const struct parser_enum *enm = e->type->enm;
            return code_emit_load_int(code, e->ival + enm->member_begin);
        }

    case NOD_EXPR_FUNCLIT:
        return code_emit_load_int(code, e->func->id);

    case NOD_EXPR_CONV:
        return gen_convert(code, e);

    case NOD_EXPR_VAR:
        if (e->var->is_global) {
            int id = gen_addr(code, e);
            int dst = code_allocate_temporary_register(code);
            return code_emit_load_global(code, dst, id);
        }
        else if (e->var->is_outparam) {
            int id = gen_addr(code, e);
            int dst = code_allocate_temporary_register(code);
            return code_emit_load_indirect(code, dst, id);
        }
        else {
            return e->var->offset;
        }

    case NOD_EXPR_STRUCTFIELD:
        return e->struct_field->offset;

    case NOD_EXPR_ENUMFIELD:
        return code_emit_load_int(code, e->enum_field->offset);

    case NOD_EXPR_INDEX:
        {
            int obj = gen_expr(code, e->l);
            int idx = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, obj, idx);
            return emit_load_vec(code, dst, obj, idx, e->type);
        }

    case NOD_EXPR_MAPINDEX:
        {
            int obj = gen_expr(code, e->l);
            int idx = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, obj, idx);
            return emit_load_map(code, dst, obj, idx, e->type);
        }

    case NOD_EXPR_STRUCTACCESS:
        {
            int obj = gen_expr(code, e->l);
            int idx = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, obj, idx);
            return emit_load_struct(code, dst, obj, idx, e->type);
        }

    case NOD_EXPR_ENUMACCESS:
        {
            int enm = gen_expr(code, e->l);
            int fld = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, enm, fld);
            return code_emit_load_enum(code, dst, enm, fld);
        }

    case NOD_EXPR_MODULEACCESS:
        return gen_expr(code, e->r);

    case NOD_EXPR_OUTARG:
        if (parser_ast_is_outparam(e->l)) {
            return gen_addr(code, e->l);
        }
        else {
            int src = gen_addr(code, e->l);
            int dst = gen_dst_register1(code, src);
            return code_emit_load_addr(code, dst, src);
        }

    case NOD_EXPR_CALL:
        return gen_call(code, e);

    case NOD_EXPR_LOGOR:
        {
            /* eval */
            int src = gen_expr(code, e->l);
            value_addr_t els = code_emit_jump_if_zero(code, src, -1);

            /* true */
            int dst = gen_dst_register1(code, src);
            code_emit_move(code, dst, src);
            value_addr_t exit = code_emit_jump(code, -1);

            /* false */
            code_back_patch(code, els);
            dst = gen_expr(code, e->r);
            code_back_patch(code, exit);
            return dst;
        }

    case NOD_EXPR_LOGAND:
        {
            /* eval */
            int src1 = gen_expr(code, e->l);
            value_addr_t els = code_emit_jump_if_zero(code, src1, -1);

            /* true */
            int src2 = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, src1, src2);
            code_emit_move(code, dst, src2);
            value_addr_t exit = code_emit_jump(code, -1);

            /* false */
            code_back_patch(code, els);
            code_emit_move(code, dst, src1);
            code_back_patch(code, exit);
            return dst;
        }

    case NOD_EXPR_ADD:
    case NOD_EXPR_SUB:
    case NOD_EXPR_MUL:
    case NOD_EXPR_DIV:
    case NOD_EXPR_REM:
    case NOD_EXPR_AND:
    case NOD_EXPR_OR:
    case NOD_EXPR_XOR:
    case NOD_EXPR_SHL:
    case NOD_EXPR_SHR:
        {
            int src1 = gen_expr(code, e->l);
            int src2 = gen_expr(code, e->r);
            int dst  = gen_dst_register2(code, src1, src2);
            gen_binop(code, e->type, e->kind, dst, src1, src2);
            return dst;
        }

    case NOD_EXPR_EQ:
    case NOD_EXPR_NEQ:
    case NOD_EXPR_LT:
    case NOD_EXPR_LTE:
    case NOD_EXPR_GT:
    case NOD_EXPR_GTE:
        {
            int src1 = gen_expr(code, e->l);
            int src2 = gen_expr(code, e->r);
            int dst  = gen_dst_register2(code, src1, src2);
            /* e->type is always result type bool. e->l->type for operand type. */
            gen_binop(code, e->l->type, e->kind, dst, src1, src2);
            return dst;
        }

    case NOD_EXPR_NOT:
        {
            int src = gen_expr(code, e->l);
            int dst = gen_dst_register1(code, src);
            return code_emit_bitwise_not(code, dst, src);
        }

    case NOD_EXPR_POS:
        return gen_expr(code, e->l);

    case NOD_EXPR_NEG:
        {
            int src = gen_expr(code, e->l);
            int dst = gen_dst_register1(code, src);

            if (parser_is_int_type(e->type))
                return code_emit_negate_int(code, dst, src);
            else if (parser_is_float_type(e->type))
                return code_emit_negate_float(code, dst, src);
        }

    case NOD_EXPR_LOGNOT:
        {
            int src = gen_expr(code, e->l);
            int dst = gen_dst_register1(code, src);
            return code_emit_set_if_zero(code, dst, src);
        }

    case NOD_EXPR_ASSIGN:
        return gen_assign(code, e);

    case NOD_EXPR_ADDASSIGN:
    case NOD_EXPR_SUBASSIGN:
    case NOD_EXPR_MULASSIGN:
    case NOD_EXPR_DIVASSIGN:
    case NOD_EXPR_REMASSIGN:
    case NOD_EXPR_ANDASSIGN:
    case NOD_EXPR_ORASSIGN:
    case NOD_EXPR_XORASSIGN:
    case NOD_EXPR_SHLASSIGN:
    case NOD_EXPR_SHRASSIGN:
        return gen_binop_assign(code, e);

    case NOD_EXPR_INIT:
        return gen_init(code, e);
    }

    return -1;
}

static int gen_addr(struct code_bytecode *code, const struct parser_expr *e)
{
    if (!e)
        return -1;

    switch (e->kind) {

    case NOD_EXPR_VAR:
        if (e->var->is_global)
            return code_emit_load_int(code, e->var->offset);
        else
            return e->var->offset;

    case NOD_EXPR_STRUCTFIELD:
        return e->struct_field->offset;

    case NOD_EXPR_MODULEACCESS:
        return gen_addr(code, e->r);
    }

    return -1;
}

static void gen_stmt(struct code_bytecode *code, const struct parser_stmt *s)
{
    if (!s)
        return;

    switch (s->kind) {

    case NOD_STMT_NOP:
        break;

    case NOD_STMT_BLOCK:
        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);
        break;

    case NOD_STMT_IF:
        code_begin_if(code);

        for (struct parser_stmt *stmt = s->children; stmt; stmt = stmt->next)
            gen_stmt(code, stmt);

        /* exit */
        code_back_patch_else_ends(code);
        break;

    case NOD_STMT_ELSE:
        {
            value_addr_t next = 0;

            if (s->cond) {
                int cond = gen_expr(code, s->cond);
                next = code_emit_jump_if_zero(code, cond, -1);
            }

            /* true */
            gen_stmt(code, s->body);

            if (s->cond) {
                /* exit */
                value_addr_t addr = code_emit_jump(code, -1);
                code_push_else_end(code, addr);
                code_back_patch(code, next);
            }
        }
        break;

    case NOD_STMT_WHILE:
        {
            bool infinite_loop = false;
            value_int_t result = 0;
            if (parser_eval_expr(s->cond, &result))
                infinite_loop = result != 0;

            code_begin_while(code);

            /* cond */
            int begin;
            int jump;

            begin = code_get_next_addr(code);
            if (!infinite_loop) {
                int cond = gen_expr(code, s->cond);
                jump = code_emit_jump_if_zero(code, cond, -1);
            }

            /* true */
            gen_stmt(code, s->body);

            /* exit */
            code_emit_safepoint(code);
            code_emit_jump(code, begin);
            code_back_patch_breaks(code);
            if (!infinite_loop)
                code_back_patch(code, jump);
        }
        break;

    case NOD_STMT_FORNUM:
        {
            code_begin_for(code);

            int iter = gen_addr(code, s->expr);
            int start = gen_expr(code, s->cond);
            int stop = gen_expr(code, s->cond->next);
            int step = gen_expr(code, s->cond->next->next);

            code_emit_move(code, iter + 1, start);
            code_emit_move(code, iter + 2, stop);
            code_emit_move(code, iter + 3, step);

            /* begin */
            value_addr_t init = code_emit_fornum_begin(code, iter);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_fornum_end(code, iter, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORVEC:
        {
            code_begin_for(code);

            int idx = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 2 registers away from the iterator */
            code_emit_move(code, idx + 2, obj);

            /* begin */
            value_addr_t init = code_emit_forvec_begin(code, idx);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forvec_end(code, idx, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORMAP:
        {
            code_begin_for(code);

            int itr = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 3 registers away from the iterator */
            code_emit_move(code, itr + 3, obj);

            /* begin */
            value_addr_t init = code_emit_formap_begin(code, itr);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_formap_end(code, itr, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORSET:
        {
            code_begin_for(code);

            int itr = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 2 registers away from the iterator */
            code_emit_move(code, itr + 2, obj);

            /* begin */
            value_addr_t init = code_emit_forset_begin(code, itr);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forset_end(code, itr, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORSTACK:
        {
            code_begin_for(code);

            int idx = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 2 registers away from the iterator */
            code_emit_move(code, idx + 2, obj);

            /* begin */
            value_addr_t init = code_emit_forstack_begin(code, idx);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forstack_end(code, idx, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORQUEUE:
        {
            code_begin_for(code);

            int idx = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 2 registers away from the iterator */
            code_emit_move(code, idx + 2, obj);

            /* begin */
            value_addr_t init = code_emit_forqueue_begin(code, idx);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forqueue_end(code, idx, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORENUM:
        {
            code_begin_for(code);

            const struct parser_enum *enm = s->cond->type->enm;
            int idx = gen_addr(code, s->expr);
            int start = code_emit_load_int(code, enm->member_begin);
            int stop = code_emit_load_int(code, enm->member_end);

            /* the enum is located 1 registers away from the iterator */
            code_emit_move(code, idx, start);
            code_emit_move(code, idx + 1, stop);

            /* begin */
            value_addr_t init = code_emit_forenum_begin(code, idx);
            value_addr_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forenum_end(code, idx, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_BREAK:
        {
            value_addr_t addr = code_emit_jump(code, -1);
            code_push_break(code, addr);
        }
        break;

    case NOD_STMT_CONTINUE:
        {
            value_addr_t addr = code_emit_jump(code, -1);
            code_push_continue(code, addr);
        }
        break;


    case NOD_STMT_SWITCH:
        {
            /* init */
            code_begin_switch(code);
            int curr = code_get_register_pointer(code);
            int dst = code_allocate_temporary_register(code);
            int src = gen_expr(code, s->cond);
            dst = code_emit_move(code, dst, src);

            /* cases */
            for (struct parser_stmt *cas = s->children; cas; cas = cas->next) {
                /* set current where expr is stored */
                code_set_register_pointer(code, dst);
                gen_stmt(code, cas);
            }

            /* quit */
            code_backpatch_case_ends(code);

            /* remove cond val */
            code_set_register_pointer(code, curr);
        }
        break;

    case NOD_STMT_CASE:
        {
            /* cond */
            int src1 = code_get_register_pointer(code);

            /* backpatch address for each expression */
            struct data_intvec trues = {0};
            data_intvec_init(&trues);

            /* eval conds */
            for (struct parser_expr *cond = s->cond; cond; cond = cond->next) {
                /* cond test */
                int src2 = gen_expr(code, cond);
                int dst = code_set_register_pointer(code, src1 + 1);
                code_emit_equal_int(code, dst, src1, src2);

                /* jump if true otherwise fallthrough */
                value_addr_t tru = code_emit_jump_if_not_zero(code, dst, -1);
                data_intvec_push(&trues, tru);
            }
            /* all conds false -> close case */
            value_addr_t exit = code_emit_jump(code, -1);
            /* one of cond true -> go to body */
            for (int i = 0; i < trues.len; i++)
                code_back_patch(code, trues.data[i]);
            data_intvec_free(&trues);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            value_addr_t addr = code_emit_jump(code, -1);
            code_push_case_end(code, addr);
            code_back_patch(code, exit);
        }
        break;

    case NOD_STMT_OTHERS:
        /* body */
        gen_stmt(code, s->body);
        break;

    case NOD_STMT_RETURN:
        {
            int src = gen_expr(code, s->expr);
            code_emit_return(code, src);
        }
        break;

    case NOD_STMT_EXPR:
        gen_expr(code, s->expr);
        break;

        /* XXX need NOD_STMT_ASSNSTMT? */
    case NOD_STMT_ASSIGN:
    case NOD_STMT_INIT:
        gen_expr(code, s->expr);
        break;
    }

    code_clear_temporary_registers(code);
}

static void gen_func(struct code_bytecode *code, const struct parser_func *func, int func_id)
{
    /* TODO solve param count and reg count at a time */
    /* Local var registers */
    int param_count = parser_required_param_count(func->sig);
    int lvar_count = func->scope->size;
    /* TODO rename code_reset_register_pointer() */
    code_init_registers(code, lvar_count + param_count);

    /* Function body */
    value_addr_t func_addr = code_get_next_addr(code);
    gen_stmt(code, func->body);

    /* Back patch used registers */
    /* TODO rename code_set_function_register_count() */
    code_set_function_address(code, func_id, func_addr);
    code_set_function_register_count(code, func_id);
}

static void gen_funcs(struct code_bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    /* imported modules first */
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_funcs(code, sym->module);
    }

    /* self module next */
    for (int i = 0; i < mod->funcs.len; i++) {
        struct parser_func *func = mod->funcs.data[i];
        if (!func->sig->is_native)
            gen_func(code, func, func->id);
    }
}

static void gen_gvars(struct code_bytecode *code, const struct parser_module *mod)
{
    struct parser_scope *scope = mod->scope;

    /* imported modules first */
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        if (sym->kind == SYM_MODULE)
            gen_gvars(code, sym->module);
    }

    /* self module next */
    for (const struct parser_stmt *gvar = mod->gvars; gvar; gvar = gvar->next)
        gen_stmt(code, gvar);
}

static bool is_module_init_name(const char *fullname)
{
    const char *sep = strrchr(fullname, ':');
    if (sep)
        return !strcmp(sep + 1, "init");

    return false;
}

static void gen_init_funcs(struct code_bytecode *code, const struct parser_module *mod, int ret_reg)
{
    for (int i = 0; i < mod->scope->syms.len; i++) {
        const struct parser_symbol *sym = mod->scope->syms.data[i];

        if (sym->kind == SYM_MODULE) {
            const struct parser_module *m = sym->module;
            const struct parser_func *init_func = NULL;

            /* find init func */
            for (int j = 0; j < m->scope->syms.len; j++) {
                const struct parser_symbol *s = m->scope->syms.data[j];

                if (s->kind == SYM_FUNC) {
                    if (is_module_init_name(s->func->fullname)) {
                        init_func = s->func;
                        break;
                    }
                }
            }
            if (!init_func)
                continue;

            /* call init func */
            bool is_native = true;
            code_emit_call_function(code, ret_reg, init_func->id, is_native);
        }
    }
}

static void gen_start_func_body(struct code_bytecode *code, const struct parser_module *mod)
{
    /* module init funcs */
    int ret_reg = code_allocate_temporary_register(code);
    gen_init_funcs(code, mod, ret_reg);

    /* global vars */
    gen_gvars(code, mod);

    /* args vec{string} for main */
    int dst = gen_dst_register1(code, ret_reg);
    code_emit_move_ref(code, dst, 0);

    /* TODO maybe better to search "main" module and "main" func in there */
    /* instead of holding main_func */

    /* main func */
    int main_func_id = mod->main_func->id;
    bool is_native = mod->main_func->sig->is_native;
    code_emit_call_function(code, dst, main_func_id, is_native);
    code_emit_return(code, dst);
}

static void gen_start_func(struct code_bytecode *code, const struct parser_module *mod,
        int func_id)
{
    /* local var register for args []string */
    int lvar_count = 1;
    code_init_registers(code, lvar_count);

    /* function body */
    value_addr_t func_addr = code_get_next_addr(code);
    gen_start_func_body(code, mod);

    /* back patch register count */
    code_set_function_address(code, func_id, func_addr);
    code_set_function_register_count(code, func_id);
}

static void gen_module(struct code_bytecode *code, const struct parser_module *mod)
{
    if (!mod->main_func) {
        fprintf(stderr, "error: 'main' function not found");
    }

    /* start func */
    int start_id = code_register_function(code, "_start", 0);
    bool is_native = false;
    code_emit_call_function(code, 0, start_id, is_native);
    code_emit_halt(code);

    gen_start_func(code, mod, start_id);

    /* Global funcs */
    gen_funcs(code, mod);
}

static void gen_enum_values(struct code_bytecode *code, struct parser_scope *scope)
{
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        switch (sym->kind) {

        case SYM_ENUM:
            {
                struct parser_enum *enm = sym->enm;
                int nfields = parser_get_enum_field_count(enm);
                int nmembers = parser_get_enum_member_count(enm);

                for (int x = 0; x < nfields; x++) {
                    struct parser_enum_field *field;
                    field = parser_get_enum_field(enm, x);

                    for (int y = 0; y < nmembers; y++) {
                        struct parser_enum_value val = parser_get_enum_value(enm, x, y);
                        int field_index = 0;

                        if (parser_is_int_type(field->type)) {
                            field_index = code_push_enum_field_int(code, val.ival);
                        }
                        else if (parser_is_float_type(field->type)) {
                            field_index = code_push_enum_field_float(code, val.fval);
                        }
                        else if (parser_is_string_type(field->type)) {
                            field_index = code_push_enum_field_string(code, val.sval);
                        }

                        if (x == 0 && y == 0) {
                            enm->member_begin = field_index;
                            enm->member_end = field_index + nmembers;
                        }
                    }

                    field->offset = x * nmembers;
                }
            }
            break;

        case SYM_MODULE:
            gen_enum_values(code, sym->module->scope);
            break;
        }
    }
}

static void register_definitions(struct code_bytecode *code, struct parser_scope *scope)
{
    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        switch (sym->kind) {

        case SYM_FUNC:
            {
                struct parser_func *func = sym->func;
                int param_count = parser_required_param_count(func->sig);

                func->id = code_register_function(code, func->fullname, param_count);

                if (func->native_func_ptr) {
                    code_set_native_function_pointer(code, func->id,
                            (native_func_t) func->native_func_ptr);

                    code_set_function_variadic(code, func->id, func->sig->is_variadic);
                }
            }
            break;

        case SYM_STRUCT:
            {
                struct parser_struct *strct = sym->strct;
                int field_count = parser_struct_get_field_count(strct);

                strct->id = code_register_struct(code, strct->name, field_count);

                for (int i = 0; i < field_count; i++) {
                    const struct parser_struct_field *f = parser_get_struct_field(strct, i);
                    int val_type = parser_type_to_value_type(f->type);
                    code_push_struct_field_type(code, strct->id, val_type);
                }
            }
            break;

        case SYM_MODULE:
            register_definitions(code, sym->module->scope);
            break;
        }
    }
}

void code_generate(struct code_bytecode *code, const struct parser_module *mod)
{
    /* globals */
    code_set_global_count(code, mod->scope->size);

    /* enums */
    gen_enum_values(code, mod->scope);

    /* functions, structs */
    register_definitions(code, mod->scope->parent);

    /* modules */
    gen_module(code, mod);

    /* halt */
    code_emit_halt(code);
}

static int max(int a, int b)
{
    return a < b ? b : a;
}

static int resolve_offset(struct parser_scope *scope, int start_offset)
{
    int cur_offset = start_offset;
    int max_offset = start_offset;
    int cur_size = 0;
    int max_size = 0;

    for (int i = 0; i < scope->syms.len; i++) {
        struct parser_symbol *sym = scope->syms.data[i];

        switch (sym->kind) {

        case SYM_VAR:
            {
                struct parser_var *var = sym->var;
                /* offset */
                var->offset = cur_offset;
                cur_offset++;
                max_offset = max(max_offset, cur_offset);
                /* size */
                if (!var->is_param)
                    cur_size++;
                max_size = max(max_size, cur_size);
            }
            break;

        case SYM_FUNC:
            {
                struct parser_scope *child = sym->func->scope;
                /* start over from offset 0 */
                resolve_offset(child, 0);
            }
            break;

        case SYM_SCOPE:
            {
                struct parser_scope *child = sym->scope;
                int child_max = resolve_offset(child, cur_offset);
                /* offset */
                max_offset = max(max_offset, child_max);
                /* size */
                max_size = max(max_size, cur_size + child->size);
            }
            break;

        case SYM_MODULE:
            {
                struct parser_scope *child = sym->module->scope;
                /* offset */
                cur_offset = resolve_offset(child, cur_offset);
                max_offset = max(max_offset, cur_offset);
                /* size */
                cur_size += child->size;
                max_size = max(max_size, cur_size);
            }
            break;
        }
    }

    scope->size = max_size;
    return max_offset;
}

void code_resolve_offset(struct parser_module *mod)
{
    resolve_offset(mod->scope, 0);
}
