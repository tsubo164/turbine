#include "code_print.h"
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


void code_print_bytecode(const struct code_bytecode *code)
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
        int32_t instcode = code_read(code, addr);
        struct code_instruction inst = {0};
        int inc = 1;

        code_decode_instruction(instcode, &inst);
        int imm_size = 0;
        code_print_instruction(code, addr, &inst, &imm_size);
        inc += imm_size;

        addr += inc;

        /* TODO come up with better way */
        const struct code_opcode_info *info = code_lookup_opecode_info(inst.op);
        if (info->extend)
            addr += 2;
    }
}

static void print_operand(const struct code_bytecode *code,
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
        if (code_is_smallint_register(operand)) {
            struct runtime_value val;
            val = code_read_immediate_value(code, addr, operand, imm_size);
            printf("$%lld", val.inum);
        }
        else {
            printf("r%d", operand);
        }
        break;
    }

    if (separator)
        printf(", ");
}

static void print_operand16(const struct code_bytecode *code, int operand)
{
    printf("$%d", operand);
}

void code_print_instruction(const struct code_bytecode *code,
        int64_t addr, const struct code_instruction *inst, int *imm_size)
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
            print_operand16(code, inst->A);
        else
            print_operand(code, addr, inst->A, 0, NULL);
        break;

    case OPERAND_AB_:
        print_operand(code, addr, inst->A, 1, NULL);
        print_operand(code, addr, inst->B, 0, imm_size);
        break;

    case OPERAND_ABB:
        print_operand(code, addr, inst->A, 1, NULL);
        print_operand16(code, inst->BB);
        break;

    case OPERAND_ABC:
        print_operand(code, addr, inst->A, 1, imm_size);
        print_operand(code, addr, inst->B, 1, imm_size);
        print_operand(code, addr, inst->C, 0, imm_size);
        break;
    }

    if (info->extend) {
        int64_t lo = code_read(code, addr + 1);
        int64_t hi = code_read(code, addr + 2);
        int64_t immediate = (hi << 32) | lo;
        printf(" $%lld", immediate);
    }

    printf("\n");
}
