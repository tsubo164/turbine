#include "code_generate.h"
#include "code_bytecode.h"
#include "parser_ast.h"
#include "parser_ast_eval.h"
#include "parser_symbol.h"
#include "parser_type.h"

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

static int gen_array_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int index = 0;
    int len = 0;
    int dst = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make array */
    len = gen_expr(code, e->l);
    code_emit_new_array(code, dst, len);

    /* set elements */
    for (elem = e->r; elem; elem = elem->next) {
        int src = gen_expr(code, elem);
        int idx = code_emit_load_int(code, index);
        code_emit_store_array(code, dst, idx, src);
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

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make map */
    len = gen_expr(code, e->l);
    code_emit_new_map(code, dst, len);

    /* set elements */
    for (elem = e->r; elem; elem = elem->next) {
        int key = gen_expr(code, elem->l);
        int src = gen_expr(code, elem->r);
        code_emit_store_map(code, dst, key, src);
    }

    return dst;
}

static int gen_struct_lit(struct code_bytecode *code,
        const struct parser_expr *e, int dst_reg)
{
    const struct parser_expr *elem;
    int len = 0;
    int dst = 0;

    /* dst register */
    if (dst_reg == -1)
        dst = code_allocate_temporary_register(code);
    else
        dst = dst_reg;

    /* make struct */
    len = parser_struct_get_field_count(e->type->strct);
    code_emit_new_struct(code, dst, len);

    /* set elements */
    for (elem = e->l; elem; elem = elem->next) {
        int idx = gen_addr(code, elem->l);
        int src = gen_expr(code, elem->r);
        code_emit_store_struct(code, dst, idx, src);
    }

    return dst;
}

static int gen_init(struct code_bytecode *code, const struct parser_expr *e)
{
    if (parser_ast_is_global(e->l)) {
        int dst = gen_addr(code, e->l);
        int tmp = gen_expr(code, e->r);
        return code_emit_store_global(code, dst, tmp);
    }

    if (e->r->kind == NOD_EXPR_ARRAYLIT) {
        int dst = gen_addr(code, e->l);
        return gen_array_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_MAPLIT) {
        int dst = gen_addr(code, e->l);
        return gen_map_lit(code, e->r, dst);
    }

    if (e->r->kind == NOD_EXPR_STRUCTLIT) {
        int dst = gen_addr(code, e->l);
        return gen_struct_lit(code, e->r, dst);
    }
    {
        int dst = gen_addr(code, e->l);
        int src = gen_expr(code, e->r);
        return code_emit_move(code, dst, src);
    }
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

static int gen_assign(struct code_bytecode *code, const struct parser_expr *e)
{
    const struct parser_expr *lval = e->l;
    const struct parser_expr *rval = e->r;

    if (lval->kind == NOD_EXPR_INDEX) {
        /* a[b] = x */
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        int src = gen_expr(code, rval);
        return code_emit_store_array(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_MAPINDEX) {
        /* a[b] = x */
        int obj = gen_expr(code, lval->l);
        int key = gen_expr(code, lval->r);
        int src = gen_expr(code, rval);
        return code_emit_store_map(code, obj, key, src);
    }

    if (lval->kind == NOD_EXPR_STRUCTACCESS) {
        /* a.b = x */
        int obj = gen_expr(code, lval->l);
        int fld = gen_addr(code, lval->r);
        int src = gen_expr(code, rval);
        return code_emit_store_struct(code, obj, fld, src);
    }

    if (lval->kind == NOD_EXPR_MODULEACCESS) {
        /* m.a = x */
        int reg0 = gen_addr(code, lval);
        int reg1 = gen_expr(code, rval);
        code_emit_store_global(code, reg0, reg1);
        return reg0;
    }

    if (parser_ast_is_global(lval)) {
        /* _a_ = x */
        int dst = gen_addr(code, lval);
        int src = gen_expr(code, rval);
        code_emit_store_global(code, dst, src);
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
            int dst = gen_addr(code, lval);
            int src1 = gen_expr(code, rval->l);
            int src2 = gen_expr(code, rval->r);
            return gen_binop(code, e->type, rval->kind, dst, src1, src2);
        }
    }
    {
        /* a = x */
        int dst = gen_addr(code, lval);
        int src = gen_expr(code, rval);
        return code_emit_move(code, dst, src);
    }

    return -1;
}

static int gen_binop_assign(struct code_bytecode *code, const struct parser_expr *e)
{
    const struct parser_expr *lval = e->l;
    const struct parser_expr *rval = e->r;

    if (lval->kind == NOD_EXPR_INDEX) {
        /* a[b] += x */
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_array(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_MAPINDEX) {
        /* a[b] += x */
        int obj = gen_expr(code, lval->l);
        int idx = gen_expr(code, lval->r);
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
        gen_binop(code, e->type, e->kind, src, tmp1, tmp2);
        return code_emit_store_map(code, obj, idx, src);
    }

    if (lval->kind == NOD_EXPR_STRUCTACCESS) {
        /* a.b += x */
        int obj = gen_expr(code, lval->l);
        int fld = gen_addr(code, lval->r);
        int tmp1 = gen_expr(code, lval);
        int tmp2 = gen_expr(code, rval);
        int src = gen_dst_register2(code, tmp1, tmp2);
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
    {
        /* a += x */
        int dst = gen_addr(code, lval);
        int src1 = gen_expr(code, lval);
        int src2 = gen_expr(code, rval);
        return gen_binop(code, e->type, e->kind, dst, src1, src2);
    }
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
    int64_t func_id = 0;
    if (parser_eval_expr(call->l, &func_id)) {
        code_emit_call_function(code, retval_reg, func_id, func_sig->is_builtin);
    }
    else {
        int src = gen_expr(code, call->l);
        code_emit_call_function_pointer(code, retval_reg, src);
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

    case NOD_EXPR_ARRAYLIT:
        return gen_array_lit(code, e, -1);

    case NOD_EXPR_STRUCTLIT:
        return gen_struct_lit(code, e, -1);

    case NOD_EXPR_ENUMLIT:
        return code_emit_load_int(code, e->ival);

    case NOD_EXPR_FUNCLIT:
        return code_emit_load_int(code, e->func->id);

    case NOD_EXPR_CONV:
        return gen_convert(code, e);

    case NOD_EXPR_VAR:
        if (e->var->is_global) {
            int id = code_emit_load_int(code, e->var->offset);
            int dst = code_allocate_temporary_register(code);
            return code_emit_load_global(code, dst, id);
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
            code_emit_load_array(code, dst, obj, idx);
            return dst;
        }

    case NOD_EXPR_MAPINDEX:
        {
            int obj = gen_expr(code, e->l);
            int idx = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, obj, idx);
            code_emit_load_map(code, dst, obj, idx);
            return dst;
        }

    case NOD_EXPR_STRUCTACCESS:
        {
            int obj = gen_expr(code, e->l);
            int idx = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, obj, idx);
            return code_emit_load_struct(code, dst, obj, idx);
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

    case NOD_EXPR_CALL:
        return gen_call(code, e);

    case NOD_EXPR_LOGOR:
        {
            /* eval */
            int src = gen_expr(code, e->l);
            int64_t els = code_emit_jump_if_zero(code, src, -1);

            /* true */
            int dst = gen_dst_register1(code, src);
            code_emit_move(code, dst, src);
            int64_t exit = code_emit_jump(code, -1);

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
            int64_t els = code_emit_jump_if_zero(code, src1, -1);

            /* true */
            int src2 = gen_expr(code, e->r);
            int dst = gen_dst_register2(code, src1, src2);
            code_emit_move(code, dst, src2);
            int64_t exit = code_emit_jump(code, -1);

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
            int64_t next = 0;

            if (s->cond) {
                int cond = gen_expr(code, s->cond);
                next = code_emit_jump_if_zero(code, cond, -1);
            }

            /* true */
            gen_stmt(code, s->body);

            if (s->cond) {
                /* exit */
                int64_t addr = code_emit_jump(code, -1);
                code_push_else_end(code, addr);
                code_back_patch(code, next);
            }
        }
        break;

    case NOD_STMT_WHILE:
        {
            bool infinite_loop = false;
            int64_t result = 0;
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
            int64_t init = code_emit_fornum_begin(code, iter);
            int64_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_fornum_end(code, iter, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORARRAY:
        {
            code_begin_for(code);

            int idx = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 2 registers away from the iterator */
            code_emit_move(code, idx + 2, obj);

            /* begin */
            int64_t init = code_emit_forarray_begin(code, idx);
            int64_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_forarray_end(code, idx, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_FORMAP:
        {
            code_begin_for(code);

            int itr = gen_addr(code, s->expr);
            int obj = gen_expr(code, s->cond);

            /* the collection is located 4 registers away from the iterator */
            code_emit_move(code, itr + 4, obj);

            /* begin */
            int64_t init = code_emit_formap_begin(code, itr);
            int64_t begin = code_get_next_addr(code);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            code_back_patch_continues(code);
            code_emit_formap_end(code, itr, begin);

            code_back_patch(code, init);
            code_back_patch_breaks(code);
        }
        break;

    case NOD_STMT_BREAK:
        {
            int64_t addr = code_emit_jump(code, -1);
            code_push_break(code, addr);
        }
        break;

    case NOD_STMT_CONTINUE:
        {
            int64_t addr = code_emit_jump(code, -1);
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
                int64_t tru = code_emit_jump_if_not_zero(code, dst, -1);
                data_intvec_push(&trues, tru);
            }
            /* all conds false -> close case */
            int64_t exit = code_emit_jump(code, -1);
            /* one of cond true -> go to body */
            for (int i = 0; i < trues.len; i++)
                code_back_patch(code, trues.data[i]);
            data_intvec_free(&trues);

            /* body */
            gen_stmt(code, s->body);

            /* end */
            int64_t addr = code_emit_jump(code, -1);
            code_push_case_end(code, addr);
            code_back_patch(code, exit);
        }
        break;

    case NOD_STMT_DEFAULT:
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
    int64_t func_addr = code_get_next_addr(code);
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
        if (!func->sig->is_builtin)
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

static void gen_start_func_body(struct code_bytecode *code, const struct parser_module *mod)
{
    /* TODO TEST emitting call module init functions */
    {
        for (int i = 0; i < mod->scope->syms.len; i++) {
            const struct parser_symbol *sym = mod->scope->syms.data[i];

            if (sym->kind == SYM_MODULE) {
                const struct parser_module *m = sym->module;
                int64_t init_func_id = -1;
                int min_var_offset = INT32_MAX;
                int gvar_count = 0;

                for (int j = 0; j < m->scope->syms.len; j++) {
                    const struct parser_symbol *s = m->scope->syms.data[j];

                    if (s->kind == SYM_FUNC) {
                        const struct parser_func *f = s->func;
                        if (!strncmp(f->fullname, "_builtin:_init", 14)) {
                            init_func_id = f->id;
                        }
                    }
                    else if (s->kind == SYM_VAR) {
                        const struct parser_var *v = s->var;
                        if (!v->is_global)
                            continue;

                        gvar_count++;
                        if (min_var_offset > v->offset)
                            min_var_offset = v->offset;
                    }
                }

                if (init_func_id < 0)
                    continue;

                int ret_reg = min_var_offset;
                bool is_builtin = true;
                code_emit_call_function(code, ret_reg, init_func_id, is_builtin);
            }
        }
    }

    /* emit global vars */
    gen_gvars(code, mod);

    /* TODO maybe better to search "main" module and "main" func in there */
    /* instead of holding main_func */
    /* Call main */
    int reg0 = code_allocate_temporary_register(code);
    /* push args for main() */
    code_emit_move(code, reg0, 0);
    code_emit_call_function(code, reg0, mod->main_func->id,
            mod->main_func->sig->is_builtin);
    code_emit_return(code, reg0);
}

static void gen_start_func(struct code_bytecode *code, const struct parser_module *mod,
        int func_id)
{
    /* local var register for args []string */
    int lvar_count = 1;
    code_init_registers(code, lvar_count);

    /* function body */
    int64_t func_addr = code_get_next_addr(code);
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
    code_emit_call_function(code, 0, start_id, false);
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

        case SYM_TABLE:
            {
                struct parser_enum *enm = sym->enm;
                int nfields = parser_get_enum_field_count(enm);
                int nmembers = parser_get_enum_member_count(enm);

                for (int x = 0; x < nfields; x++) {
                    int field_offset = 0;
                    struct parser_enum_field *field;
                    field = parser_get_enum_field(enm, x);

                    for (int y = 0; y < nmembers; y++) {
                        struct parser_enum_value val = parser_get_enum_value(enm, x, y);
                        int tmp_offset = 0;

                        if (parser_is_string_type(field->type)) {
                            tmp_offset = code_push_enum_field_string(code, val.sval);
                        }
                        else if (parser_is_int_type(field->type)) {
                            tmp_offset = code_push_enum_field_int(code, val.ival);
                        }

                        if (y == 0)
                            field_offset = tmp_offset;
                    }

                    field->offset = field_offset;
                }
            }
            break;

            /*
        case SYM_MODULE:
            register_functions(code, sym->module->scope);
            break;
            */
        }
    }
}

static void register_functions(struct code_bytecode *code, struct parser_scope *scope)
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
                    code_set_native_function_pointer(code,
                            func->id,
                            (runtime_native_function_t) func->native_func_ptr);

                    code_set_function_variadic(code, func->id, func->sig->is_variadic);
                }
            }
            break;

        case SYM_MODULE:
            register_functions(code, sym->module->scope);
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

    /* functions */
    register_functions(code, mod->scope->parent);

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
