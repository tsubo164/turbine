#ifndef CODE_INSTRUCTION_H
#define CODE_INSTRUCTION_H

#include "value_types.h"
#include <stdbool.h>

enum code_opcode {
    /* program control */
    OP_NOP,
    OP_HALT,
    OP_SAFEPOINTPOLL,
    OP_INTRINSICGC,
    /* load, store, move */
    OP_MOVE,
    OP_LOADCONST,
    OP_LOADGLOBAL,
    OP_STOREGLOBAL,
    OP_LOADVEC,
    OP_STOREVEC,
    OP_LOADMAP,
    OP_STOREMAP,
    OP_LOADSTRUCT,
    OP_STORESTRUCT,
    OP_LOADENUM,
    /* vec, map, set, stack, queue, struct */
    OP_NEWVEC,
    OP_NEWMAP,
    OP_NEWSET,
    OP_NEWSTACK,
    OP_NEWQUEUE,
    OP_NEWSTRUCT,
    /* arithmetic */
    OP_ADDINT,
    OP_ADDFLOAT,
    OP_SUBINT,
    OP_SUBFLOAT,
    OP_MULINT,
    OP_MULFLOAT,
    OP_DIVINT,
    OP_DIVFLOAT,
    OP_REMINT,
    OP_REMFLOAT,
    OP_EQINT,
    OP_EQFLOAT,
    OP_NEQINT,
    OP_NEQFLOAT,
    OP_LTINT,
    OP_LTFLOAT,
    OP_LTEINT,
    OP_LTEFLOAT,
    OP_GTINT,
    OP_GTFLOAT,
    OP_GTEINT,
    OP_GTEFLOAT,
    OP_BITWISEAND,
    OP_BITWISEOR,
    OP_BITWISEXOR,
    OP_BITWISENOT,
    OP_SHL,
    OP_SHR,
    OP_NEGINT,
    OP_NEGFLOAT,
    OP_SETIFZERO,
    OP_SETIFNOTZ,
    /* string */
    OP_CATSTRING,
    OP_EQSTRING,
    OP_NEQSTRING,
    /* function call */
    OP_CALL,
    OP_CALLPOINTER,
    OP_CALLNATIVE,
    OP_RETURN,
    /* jump */
    OP_JUMP,
    OP_JUMPIFZERO,
    OP_JUMPIFNOTZ,
    /* loop */
    OP_FORNUMBEGIN,
    OP_FORNUMEND,
    OP_FORVECBEGIN,
    OP_FORVECEND,
    OP_FORMAPBEGIN,
    OP_FORMAPEND,
    OP_FORSETBEGIN,
    OP_FORSETEND,
    OP_FORSTACKBEGIN,
    OP_FORSTACKEND,
    OP_FORQUEUEBEGIN,
    OP_FORQUEUEEND,
    OP_FORENUMBEGIN,
    OP_FORENUMEND,
    /* conversion */
    OP_BOOLTOINT,
    OP_BOOLTOFLOAT,
    OP_INTTOBOOL,
    OP_INTTOFLOAT,
    OP_FLOATTOBOOL,
    OP_FLOATTOINT,
    /* eoc */
    END_OF_OPCODE,
};

enum operand_format {
    OPERAND____,
    OPERAND_A__,
    OPERAND_AB_,
    OPERAND_ABC,
    OPERAND_ABB,
};

struct code_opcode_info {
    const char *mnemonic;
    enum operand_format operand;
};

struct code_instruction {
    int op;
    int A, B, C;
    int BB;
};

struct code_instructionvec {
    int32_t *data;
    value_int_t cap;
    value_int_t len;
};

void code_instructionvec_free(struct code_instructionvec *v);

const struct code_opcode_info *code_lookup_opecode_info(int op);

void code_decode_instruction(int32_t instcode, struct code_instruction *inst);

void code_push_instruction__(struct code_instructionvec *v, int op);
void code_push_instruction_a(struct code_instructionvec *v, int op, int a);
void code_push_instruction_ab(struct code_instructionvec *v, int op, int a, int b);
void code_push_instruction_abc(struct code_instructionvec *v, int op, int a, int b, int c);
void code_push_instruction_abb(struct code_instructionvec *v, int op, int a, int bb);
void code_push_immediate_value(struct code_instructionvec *v, int32_t val);

#endif /* _H */
