#include "code_instruction.h"
#include <assert.h>
#include <stdlib.h>

static const struct code_opcode_info opecode_table[] = {
    [OP_NOP]            = { "nop",         OPERAND____ },
    /* load/store/move */
    [OP_MOVE]           = { "move",        OPERAND_AB_ },
    [OP_LOAD]           = { "load",        OPERAND_AB_ },
    [OP_STORE]          = { "store",       OPERAND_AB_ },
    [OP_LOADARRAY]      = { "loadarray",   OPERAND_ABC },
    [OP_STOREARRAY]     = { "storearray",  OPERAND_ABC },
    [OP_LOADSTRUCT]     = { "loadstruct",  OPERAND_ABC },
    [OP_STORESTRUCT]    = { "storestruct", OPERAND_ABC },
    [OP_LOADTYPEID]     = { "loadtypeid",  OPERAND_AB_ },
    [OP_LOADADDR]       = { "loadaddr",    OPERAND_AB_ },
    [OP_DEREF]          = { "deref",       OPERAND_AB_ },
    /* array/struct */
    [OP_NEWARRAY]       = { "newarray",    OPERAND_AB_ },
    [OP_NEWSTRUCT]      = { "newstruct",   OPERAND_AB_ },
    /* arithmetic */
    [OP_ADDINT]         = { "addint",      OPERAND_ABC },
    [OP_ADDFLOAT]       = { "addfloat",    OPERAND_ABC },
    [OP_SUBINT]         = { "subint",      OPERAND_ABC },
    [OP_SUBFLOAT]       = { "subfloat",    OPERAND_ABC },
    [OP_MULINT]         = { "mulint",      OPERAND_ABC },
    [OP_MULFLOAT]       = { "mulfloat",    OPERAND_ABC },
    [OP_DIVINT]         = { "divint",      OPERAND_ABC },
    [OP_DIVFLOAT]       = { "divfloat",    OPERAND_ABC },
    [OP_REMINT]         = { "remint",      OPERAND_ABC },
    [OP_REMFLOAT]       = { "remfloat",    OPERAND_ABC },
    [OP_EQINT]          = { "eqint",       OPERAND_ABC },
    [OP_EQFLOAT]        = { "eqfloat",     OPERAND_ABC },
    [OP_NEQINT]         = { "neqint",      OPERAND_ABC },
    [OP_NEQFLOAT]       = { "neqfloat",    OPERAND_ABC },
    [OP_LTINT]          = { "ltint",       OPERAND_ABC },
    [OP_LTFLOAT]        = { "ltfloat",     OPERAND_ABC },
    [OP_LTEINT]         = { "lteint",      OPERAND_ABC },
    [OP_LTEFLOAT]       = { "ltefloat",    OPERAND_ABC },
    [OP_GTINT]          = { "gtint",       OPERAND_ABC },
    [OP_GTFLOAT]        = { "gtfloat",     OPERAND_ABC },
    [OP_GTEINT]         = { "gteint",      OPERAND_ABC },
    [OP_GTEFLOAT]       = { "gtefloat",    OPERAND_ABC },
    [OP_BITWISEAND]     = { "bitwiseand",  OPERAND_ABC },
    [OP_BITWISEOR]      = { "bitwiseor",   OPERAND_ABC },
    [OP_BITWISEXOR]     = { "bitwisexor",  OPERAND_ABC },
    [OP_BITWISENOT]     = { "bitwisenot",  OPERAND_AB_ },
    [OP_SHL]            = { "shl",         OPERAND_ABC },
    [OP_SHR]            = { "shr",         OPERAND_ABC },
    [OP_NEGINT]         = { "negint",      OPERAND_AB_ },
    [OP_NEGFLOAT]       = { "negfloat",    OPERAND_AB_ },
    [OP_SETIFZERO]      = { "setifzero",   OPERAND_AB_ },
    [OP_SETIFNOTZ]      = { "setifnotz",   OPERAND_AB_ },
    [OP_INC]            = { "inc",         OPERAND_A__ },
    [OP_DEC]            = { "dec",         OPERAND_A__ },
    /* string */
    [OP_CATSTRING]      = { "catstring",   OPERAND_ABC },
    [OP_EQSTRING]       = { "eqstring",    OPERAND_ABC },
    [OP_NEQSTRING]      = { "neqstring",   OPERAND_ABC },
    /* function call */
    [OP_CALL]           = { "call",        OPERAND_ABB },
    [OP_CALLPOINTER]    = { "callpointer", OPERAND_AB_ },
    [OP_CALLBUILTIN]    = { "callbuiltin", OPERAND_ABB },
    [OP_RETURN]         = { "return",      OPERAND_A__ },
    /* jump */
    [OP_JUMP]           = { "jump",        OPERAND_ABB },
    [OP_JUMPIFZERO]     = { "jumpifzero",  OPERAND_ABB },
    [OP_JUMPIFNOTZ]     = { "jumpifnotz",  OPERAND_ABB },
    /* loop */
    [OP_FORNUMINIT]     = { "fornuminit",  OPERAND_ABB },
    [OP_FORNUMREST]     = { "fornumrest",  OPERAND_ABB },
    /* stack operation */
    [OP_ALLOCATE]       = { "allocate",    OPERAND_A__ },
    /* conversion */
    [OP_BOOLTOINT]      = { "booltoint",   OPERAND_AB_ },
    [OP_BOOLTOFLOAT]    = { "booltofloat", OPERAND_AB_ },
    [OP_INTTOBOOL]      = { "inttobool",   OPERAND_AB_ },
    [OP_INTTOFLOAT]     = { "inttofloat",  OPERAND_AB_ },
    [OP_FLOATTOBOOL]    = { "floattobool", OPERAND_AB_ },
    [OP_FLOATTOINT]     = { "floattoint",  OPERAND_AB_ },
    /* program control */
    [OP_HALT]           = { "halt",        OPERAND____ },
    [END_OF_OPCODE]     = { NULL },
};

const struct code_opcode_info *code_lookup_opecode_info(int op)
{
    assert(op >= 0 && op < END_OF_OPCODE);
    return &opecode_table[op];
}

/* instruction vector */

#define MIN_CAP 128

void push_inst(struct code_instructionvec *v, int32_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

/* decode */
static int decode_op(int32_t inst)
{
    return inst >> 24;
}

static int decode_a(int32_t inst)
{
    return (inst >> 16) & 0xFF;
}

static int decode_b(int32_t inst)
{
    return (inst >>  8) & 0xFF;
}

static int decode_c(int32_t inst)
{
    return inst & 0xFF;
}

static int decode_bb(int32_t inst)
{
    return inst & 0xFFFF;
}

/* encode */
static int32_t encode_op(int op)
{
    return (op & 0xFF) << 24;
}

static int32_t encode_a(int op, int a)
{
    return
        ((op & 0xFF) << 24) |
        ( (a & 0xFF) << 16);
}

static int32_t encode_ab(int op, int a, int b)
{
    return
        ((op & 0xFF) << 24) |
        (( a & 0xFF) << 16) |
        (( b & 0xFF) <<  8);
}

static int32_t encode_abc(int op, int a, int b, int c)
{
    return
        ((op & 0xFF) << 24) |
        (( a & 0xFF) << 16) |
        (( b & 0xFF) <<  8) |
        (( c & 0xFF));
}

static int32_t encode_abb(int op, int a, int bb)
{
    return
        ((op & 0xFF) << 24) |
        (( a & 0xFF) << 16) |
        ((bb & 0xFFFF));
}

void code_push_instruction__(struct code_instructionvec *v, int op)
{
    int32_t inst = encode_op(op);
    push_inst(v, inst);
}

void code_push_instruction_a(struct code_instructionvec *v, int op, int a)
{
    int32_t inst = encode_a(op, a);
    push_inst(v, inst);
}

void code_push_instruction_ab(struct code_instructionvec *v, int op, int a, int b)
{
    int32_t inst = encode_ab(op, a, b);
    push_inst(v, inst);
}

void code_push_instruction_abc(struct code_instructionvec *v, int op, int a, int b, int c)
{
    int32_t inst = encode_abc(op, a, b, c);
    push_inst(v, inst);
}

void code_push_instruction_abb(struct code_instructionvec *v, int op, int a, int bb)
{
    int32_t inst = encode_abb(op, a, bb);
    push_inst(v, inst);
}

void code_push_immediate_value(struct code_instructionvec *v, int32_t val)
{
    push_inst(v, val);
}

void code_decode_instruction(int32_t instcode, struct code_instruction *inst)
{
    int op = decode_op(instcode);
    const struct code_opcode_info *info = code_lookup_opecode_info(op);

    inst->op = op;

    switch (info->operand) {

    case OPERAND____:
        break;

    case OPERAND_A__:
        inst->A = decode_a(instcode);
        break;

    case OPERAND_AB_:
        inst->A = decode_a(instcode);
        inst->B = decode_b(instcode);
        break;

    case OPERAND_ABC:
        inst->A = decode_a(instcode);
        inst->B = decode_b(instcode);
        inst->C = decode_c(instcode);
        break;

    case OPERAND_ABB:
        inst->A = decode_a(instcode);
        inst->BB = decode_bb(instcode);
        break;
    }
}
