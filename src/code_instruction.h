#ifndef CODE_INSTRUCTION_H
#define CODE_INSTRUCTION_H

#include <stdint.h>
#include <stdbool.h>

enum code_opcode {
    OP_NOP,
    /* load/store/move */
    OP_MOVE,
    OP_LOADGLOBAL,
    OP_STOREGLOBAL,
    OP_LOADARRAY,
    OP_STOREARRAY,
    OP_LOADMAP,
    OP_STOREMAP,
    OP_LOADSTRUCT,
    OP_STORESTRUCT,
    OP_LOADENUM,
    /* ------------------------------ */
    /* array, map, struct */
    OP_NEWARRAY,
    OP_NEWMAP,
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
    OP_CALLBUILTIN,
    OP_RETURN,
    /* jump */
    OP_JUMP,
    OP_JUMPIFZERO,
    OP_JUMPIFNOTZ,
    /* loop */
    OP_FORNUMBEGIN,
    OP_FORNUMEND,
    OP_FORARRAYBEGIN,
    OP_FORARRAYEND,
    OP_FORMAPBEGIN,
    OP_FORMAPEND,
    /* conversion */
    OP_BOOLTOINT,
    OP_BOOLTOFLOAT,
    OP_INTTOBOOL,
    OP_INTTOFLOAT,
    OP_FLOATTOBOOL,
    OP_FLOATTOINT,
    /* program control */
    OP_HALT,
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
    int cap;
    int len;
};

const struct code_opcode_info *code_lookup_opecode_info(int op);

void code_decode_instruction(int32_t instcode, struct code_instruction *inst);

void code_push_instruction__(struct code_instructionvec *v, int op);
void code_push_instruction_a(struct code_instructionvec *v, int op, int a);
void code_push_instruction_ab(struct code_instructionvec *v, int op, int a, int b);
void code_push_instruction_abc(struct code_instructionvec *v, int op, int a, int b, int c);
void code_push_instruction_abb(struct code_instructionvec *v, int op, int a, int bb);
void code_push_immediate_value(struct code_instructionvec *v, int32_t val);

#endif /* _H */
