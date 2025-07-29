#include "code_instruction.h"
#include <assert.h>
#include <stdlib.h>

static const struct code_opcode_info opecode_table[] = {
    /* program control */
    [OP_NOP]            = { "nop",            OPERAND____ },
    [OP_HALT]           = { "halt",           OPERAND____ },
    [OP_SAFEPOINTPOLL]  = { "safepointpoll",  OPERAND____ },
    /* load, store, move */
    [OP_MOVE]           = { "move",           OPERAND_AB_ },
    [OP_LOADGLOBAL]     = { "loadglobal",     OPERAND_AB_ },
    [OP_STOREGLOBAL]    = { "storeglobal",    OPERAND_AB_ },
    [OP_LOADVEC]        = { "loadvec",        OPERAND_ABC },
    [OP_STOREVEC]       = { "storevec",       OPERAND_ABC },
    [OP_LOADMAP]        = { "loadmap",        OPERAND_ABC },
    [OP_STOREMAP]       = { "storemap",       OPERAND_ABC },
    [OP_LOADSTRUCT]     = { "loadstruct",     OPERAND_ABC },
    [OP_STORESTRUCT]    = { "storestruct",    OPERAND_ABC },
    [OP_LOADENUM]       = { "loadenum",       OPERAND_ABC },
    /* vec, map, set, stack, queue, struct */
    [OP_NEWVEC]         = { "newvec",         OPERAND_ABC },
    [OP_NEWMAP]         = { "newmap",         OPERAND_ABC },
    [OP_NEWSET]         = { "newset",         OPERAND_ABC },
    [OP_NEWSTACK]       = { "newstack",       OPERAND_ABC },
    [OP_NEWQUEUE]       = { "newqueue",       OPERAND_ABC },
    [OP_NEWSTRUCT]      = { "newstruct",      OPERAND_ABB },
    /* arithmetic */
    [OP_ADDINT]         = { "addint",         OPERAND_ABC },
    [OP_ADDFLOAT]       = { "addfloat",       OPERAND_ABC },
    [OP_SUBINT]         = { "subint",         OPERAND_ABC },
    [OP_SUBFLOAT]       = { "subfloat",       OPERAND_ABC },
    [OP_MULINT]         = { "mulint",         OPERAND_ABC },
    [OP_MULFLOAT]       = { "mulfloat",       OPERAND_ABC },
    [OP_DIVINT]         = { "divint",         OPERAND_ABC },
    [OP_DIVFLOAT]       = { "divfloat",       OPERAND_ABC },
    [OP_REMINT]         = { "remint",         OPERAND_ABC },
    [OP_REMFLOAT]       = { "remfloat",       OPERAND_ABC },
    [OP_EQINT]          = { "eqint",          OPERAND_ABC },
    [OP_EQFLOAT]        = { "eqfloat",        OPERAND_ABC },
    [OP_NEQINT]         = { "neqint",         OPERAND_ABC },
    [OP_NEQFLOAT]       = { "neqfloat",       OPERAND_ABC },
    [OP_LTINT]          = { "ltint",          OPERAND_ABC },
    [OP_LTFLOAT]        = { "ltfloat",        OPERAND_ABC },
    [OP_LTEINT]         = { "lteint",         OPERAND_ABC },
    [OP_LTEFLOAT]       = { "ltefloat",       OPERAND_ABC },
    [OP_GTINT]          = { "gtint",          OPERAND_ABC },
    [OP_GTFLOAT]        = { "gtfloat",        OPERAND_ABC },
    [OP_GTEINT]         = { "gteint",         OPERAND_ABC },
    [OP_GTEFLOAT]       = { "gtefloat",       OPERAND_ABC },
    [OP_BITWISEAND]     = { "bitwiseand",     OPERAND_ABC },
    [OP_BITWISEOR]      = { "bitwiseor",      OPERAND_ABC },
    [OP_BITWISEXOR]     = { "bitwisexor",     OPERAND_ABC },
    [OP_BITWISENOT]     = { "bitwisenot",     OPERAND_AB_ },
    [OP_SHL]            = { "shl",            OPERAND_ABC },
    [OP_SHR]            = { "shr",            OPERAND_ABC },
    [OP_NEGINT]         = { "negint",         OPERAND_AB_ },
    [OP_NEGFLOAT]       = { "negfloat",       OPERAND_AB_ },
    [OP_SETIFZERO]      = { "setifzero",      OPERAND_AB_ },
    [OP_SETIFNOTZ]      = { "setifnotz",      OPERAND_AB_ },
    /* string */
    [OP_CATSTRING]      = { "catstring",      OPERAND_ABC },
    [OP_EQSTRING]       = { "eqstring",       OPERAND_ABC },
    [OP_NEQSTRING]      = { "neqstring",      OPERAND_ABC },
    /* function call */
    [OP_CALL]           = { "call",           OPERAND_ABB },
    [OP_CALLPOINTER]    = { "callpointer",    OPERAND_AB_ },
    [OP_CALLNATIVE]     = { "callnative",     OPERAND_ABB },
    [OP_RETURN]         = { "return",         OPERAND_A__ },
    /* jump */
    [OP_JUMP]           = { "jump",           OPERAND_ABB },
    [OP_JUMPIFZERO]     = { "jumpifzero",     OPERAND_ABB },
    [OP_JUMPIFNOTZ]     = { "jumpifnotz",     OPERAND_ABB },
    /* loop */
    [OP_FORNUMBEGIN]    = { "fornumbegin",    OPERAND_ABB },
    [OP_FORNUMEND]      = { "fornumend",      OPERAND_ABB },
    [OP_FORVECBEGIN]    = { "forvecbegin",    OPERAND_ABB },
    [OP_FORVECEND]      = { "forvecend",      OPERAND_ABB },
    [OP_FORMAPBEGIN]    = { "formapbegin",    OPERAND_ABB },
    [OP_FORMAPEND]      = { "formapend",      OPERAND_ABB },
    [OP_FORSETBEGIN]    = { "forsetbegin",    OPERAND_ABB },
    [OP_FORSETEND]      = { "forsetend",      OPERAND_ABB },
    [OP_FORSTACKBEGIN]  = { "forstackbegin",  OPERAND_ABB },
    [OP_FORSTACKEND]    = { "forstackend",    OPERAND_ABB },
    [OP_FORQUEUEBEGIN]  = { "forqueuebegin",  OPERAND_ABB },
    [OP_FORQUEUEEND]    = { "forqueueend",    OPERAND_ABB },
    [OP_FORENUMBEGIN]   = { "forenumbegin",   OPERAND_ABB },
    [OP_FORENUMEND]     = { "forenumend",     OPERAND_ABB },
    /* conversion */
    [OP_BOOLTOINT]      = { "booltoint",      OPERAND_AB_ },
    [OP_BOOLTOFLOAT]    = { "booltofloat",    OPERAND_AB_ },
    [OP_INTTOBOOL]      = { "inttobool",      OPERAND_AB_ },
    [OP_INTTOFLOAT]     = { "inttofloat",     OPERAND_AB_ },
    [OP_FLOATTOBOOL]    = { "floattobool",    OPERAND_AB_ },
    [OP_FLOATTOINT]     = { "floattoint",     OPERAND_AB_ },
    /* eoc */
    [END_OF_OPCODE]     = { NULL },
};

const struct code_opcode_info *code_lookup_opecode_info(int op)
{
    assert(op >= 0 && op < END_OF_OPCODE);
    return &opecode_table[op];
}

/* instruction vector */

#define MIN_CAP 128

static void push_inst(struct code_instructionvec *v, int32_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

void code_instructionvec_free(struct code_instructionvec *v)
{
    free(v->data);
    v->data = NULL;
    v->cap = 0;
    v->len = 0;
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
