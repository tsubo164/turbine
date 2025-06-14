#include "code_print.h"
/* TODO can remove this? */
#include "runtime_string.h"
#include "data_vec.h"
#include <stdio.h>

void code_print_bytecode(const struct code_bytecode *code, bool print_builtin)
{
    /* constant pool */
    if (code_constant_pool_get_int_count(&code->const_pool) > 0) {
        printf("* constant int:\n");
        int count = code_constant_pool_get_int_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct runtime_value val = code_constant_pool_get_int(&code->const_pool, i);
            printf("[%6d] %" PRIival "\n", i, val.inum);
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
            printf("[%6d] \"%s\"\n", i, runtime_string_get_cstr(val.string));
        }
    }

    /* enum fields */
    if (code_get_enum_field_count(code) > 0) {
        printf("* enum fields:\n");
        int nfields = code_get_enum_field_count(code);
        for (int i = 0; i < nfields; i++) {
            struct runtime_value val = code_get_enum_field(code, i);
            printf("[%6d] ", i);
            if (code_is_enum_field_int(code, i)) {
                printf("%-10" PRIival " (int)\n", val.inum);
            }
            else if (code_is_enum_field_float(code, i)) {
                printf("%-10g (float)\n", val.fpnum);
            }
            else if (code_is_enum_field_string(code, i)) {
                printf("%-10s (string)\n", runtime_string_get_cstr(val.string));
            }
        }
    }

    /* function address */
    printf("* function address:\n");
    struct data_intvec labels = {0};
    value_int_t code_size = code_get_size(code);

    data_intvec_resize(&labels, code_size);
    for (int i = 0; i < labels.len; i++)
        labels.data[i] = -1;

    for (int i = 0; i < code->funcs.len; i++) {
        const struct code_function *func = &code->funcs.data[i];

        if (func->addr == -1 && !print_builtin) {
            /* skip builtin functions */
        }
        else {
            printf("[%6d] %-10" PRId64 " (%s)\n", func->id, func->addr, func->fullname);
        }

        if (func->addr >= 0)
            labels.data[func->addr] = func->id;
    }
    printf("\n");

    /* function code */
    printf("* function code:\n");
    value_addr_t addr = 0;

    while (addr < code_size) {

        int func_id = labels.data[addr];

        if (func_id != -1) {
            const struct code_function *func;
            func = code_lookup_const_function(&code->funcs, func_id);
            printf("\n");
            printf("%s @%" PRId64 " (id:%d)\n", func->fullname, addr, func->id);
        }

        int32_t instcode = code_read(code, addr);
        struct code_instruction inst = {0};
        int inc = 1;

        code_decode_instruction(instcode, &inst);
        int imm_size = 0;
        code_print_instruction(code, addr, &inst, &imm_size);
        inc += imm_size;

        addr += inc;
    }

    data_intvec_free(&labels);
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
            printf("$%" PRIival, val.inum);
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
            printf("\"%s\"", runtime_string_get_cstr(val.string));
        }
        break;

    default:
        if (code_is_smallint_register(operand)) {
            struct runtime_value val;
            val = code_read_immediate_value(code, addr, operand, imm_size);
            printf("$%" PRIival, val.inum);
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

static void print_operand_funcname(const struct code_bytecode *code, int operand)
{
    int func_id = operand;
    const struct code_function *func = code_lookup_const_function(&code->funcs, func_id);
    printf("%s", func->fullname);
}

void code_print_instruction(const struct code_bytecode *code,
        value_addr_t addr, const struct code_instruction *inst, int *imm_size)
{
    const struct code_opcode_info *info = code_lookup_opecode_info(inst->op);

    /* address */
    if (addr >= 0)
        printf("[%6" PRId64 "] ", addr);

    /* mnemonic */
    if (info->operand != OPERAND____)
        printf("%-14s", info->mnemonic);
    else
        printf("%s", info->mnemonic);

    /* operands for call instructions */
    if (inst->op == OP_CALL || inst->op == OP_CALLNATIVE) {
        print_operand(code, addr, inst->A, 1, imm_size);
        print_operand_funcname(code, inst->BB);
        printf("\n");
        return;
    }

    /* operands for other instructions */
    switch (info->operand) {

    case OPERAND____:
        break;

    case OPERAND_A__:
        print_operand(code, addr, inst->A, 0, imm_size);
        break;

    case OPERAND_AB_:
        print_operand(code, addr, inst->A, 1, imm_size);
        print_operand(code, addr, inst->B, 0, imm_size);
        break;

    case OPERAND_ABB:
        print_operand(code, addr, inst->A, 1, imm_size);
        print_operand16(code, inst->BB);
        break;

    case OPERAND_ABC:
        print_operand(code, addr, inst->A, 1, imm_size);
        print_operand(code, addr, inst->B, 1, imm_size);
        print_operand(code, addr, inst->C, 0, imm_size);
        break;
    }

    printf("\n");
}
