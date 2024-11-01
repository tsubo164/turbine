#include "bytecode.h"
#include "error.h"
#include "vec.h"
#include "mem.h"
// TODO can remove this?
#include "gc.h"

#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

enum immediate_value_register {
    IMMEDIATE_INT32   = 255,
    IMMEDIATE_INT64   = 254,
    IMMEDIATE_FLOAT   = 253,
    IMMEDIATE_STRING  = 252,
    IMMEDIATE_SMALLINT_END   = 251,
    IMMEDIATE_SMALLINT_BEGIN = 192,
};

enum OperandSize {
    OPERAND_NONE,
    OPERAND_BYTE,
    OPERAND_WORD,
    OPERAND_WORD2,
    OPERAND_WORD3,
    OPERAND_QUAD,
    // XXX TEST register machine
    OPERAND____,
    OPERAND_A__,
    OPERAND_AB_,
    OPERAND_ABC,
    OPERAND_ABB,
};

static const struct OpcodeInfo opcode_table[] = {
    { OP_NOP,          "NOP",          OPERAND_NONE },
    // local and arg
    { OP_LOADB,        "LOADB",        OPERAND_BYTE },
    { OP_LOADI,        "LOADI",        OPERAND_QUAD },
    { OP_LOADF,        "LOADF",        OPERAND_QUAD },
    { OP_LOADS,        "LOADS",        OPERAND_WORD },
    { OP_LOADLOCAL,    "LOADLOCAL",    OPERAND_BYTE },
    { OP_LOADGLOBAL,   "LOADGLOBAL",   OPERAND_WORD },
    { OP_STORELOCAL,   "STORELOCAL",   OPERAND_BYTE },
    { OP_STOREGLOBAL,  "STOREGLOBAL",  OPERAND_WORD },
    { OP_LOAD,         "LOAD",         OPERAND_NONE },
    { OP_STORE,        "STORE",        OPERAND_NONE },
    { OP_INCLOCAL,     "INCLOCAL",     OPERAND_BYTE },
    { OP_INCGLOBAL,    "INCGLOBAL",    OPERAND_WORD },
    { OP_DECLOCAL,     "DECLOCAL",     OPERAND_BYTE },
    { OP_DECGLOBAL,    "DECGLOBAL",    OPERAND_WORD },
    { OP_ALLOC,        "ALLOC",        OPERAND_BYTE },
    // clear
    { OP_CLEAR_LOCAL,  "CLEAR_LOCAL",  OPERAND_WORD2 },
    { OP_CLEAR_GLOBAL, "CLEAR_GLOBAL", OPERAND_WORD2 },
    { OP_COPY_LOCAL,   "COPY_LOCAL",   OPERAND_WORD3 },
    { OP_COPY_GLOBAL,  "COPY_GLOBAL",  OPERAND_WORD3 },
    // address
    { OP_LOADA,        "LOADA",        OPERAND_WORD },
    { OP_DEREF,        "DEREF",        OPERAND_NONE },
    { OP_INDEX,        "INDEX",        OPERAND_NONE },
    // arg type spec
    { OP_LOADTYPEN,    "LOADTYPEN",    OPERAND_NONE },
    { OP_LOADTYPEB,    "LOADTYPEB",    OPERAND_NONE },
    { OP_LOADTYPEI,    "LOADTYPEI",    OPERAND_NONE },
    { OP_LOADTYPEF,    "LOADTYPEF",    OPERAND_NONE },
    { OP_LOADTYPES,    "LOADTYPES",    OPERAND_NONE },
    // jump and function
    { OP_CALL,         "CALL",         OPERAND_WORD },
    { OP_CALL_POINTER, "CALL_POINTER", OPERAND_NONE },
    { OP_CALL_BUILTIN, "CALL_BUILTIN", OPERAND_BYTE },
    { OP_RET,          "RET",          OPERAND_NONE },
    { OP_JMP,          "JMP",          OPERAND_WORD },
    { OP_JEQ,          "JEQ",          OPERAND_WORD },
    // arithmetic
    { OP_ADD,          "ADD",          OPERAND_NONE },
    { OP_ADDF,         "ADDF",         OPERAND_NONE },
    { OP_CATS,         "CATS",         OPERAND_NONE },
    { OP_SUB,          "SUB",          OPERAND_NONE },
    { OP_SUBF,         "SUBF",         OPERAND_NONE },
    { OP_MUL,          "MUL",          OPERAND_NONE },
    { OP_MULF,         "MULF",         OPERAND_NONE },
    { OP_DIV,          "DIV",          OPERAND_NONE },
    { OP_DIVF,         "DIVF",         OPERAND_NONE },
    { OP_REM,          "REM",          OPERAND_NONE },
    { OP_REMF,         "REMF",         OPERAND_NONE },
    // relational
    { OP_EQ,           "EQ",           OPERAND_NONE },
    { OP_EQF,          "EQF",          OPERAND_NONE },
    { OP_EQS,          "EQS",          OPERAND_NONE },
    { OP_NEQ,          "NEQ",          OPERAND_NONE },
    { OP_NEQF,         "NEQF",         OPERAND_NONE },
    { OP_NEQS,         "NEQS",         OPERAND_NONE },
    { OP_LT,           "LT",           OPERAND_NONE },
    { OP_LTF,          "LTF",          OPERAND_NONE },
    { OP_LTE,          "LTE",          OPERAND_NONE },
    { OP_LTEF,         "LTEF",         OPERAND_NONE },
    { OP_GT,           "GT",           OPERAND_NONE },
    { OP_GTF,          "GTF",          OPERAND_NONE },
    { OP_GTE,          "GTE",          OPERAND_NONE },
    { OP_GTEF,         "GTEF",         OPERAND_NONE },
    { OP_AND,          "AND",          OPERAND_NONE },
    { OP_OR,           "OR",           OPERAND_NONE },
    { OP_XOR,          "XOR",          OPERAND_NONE },
    { OP_NOT,          "NOT",          OPERAND_NONE },
    { OP_SHL,          "SHL",          OPERAND_NONE },
    { OP_SHR,          "SHR",          OPERAND_NONE },
    { OP_NEG,          "NEG",          OPERAND_NONE },
    { OP_NEGF,         "NEGF",         OPERAND_NONE },
    { OP_SETZ,         "SETZ",         OPERAND_NONE },
    { OP_SETNZ,        "SETNZ",        OPERAND_NONE },
    { OP_POP,          "POP",          OPERAND_NONE },
    { OP_DUP,          "DUP",          OPERAND_NONE },
    // conversion
    { OP_BTOI,         "BTOI",         OPERAND_NONE },
    { OP_BTOF,         "BTOF",         OPERAND_NONE },
    { OP_ITOB,         "ITOB",         OPERAND_NONE },
    { OP_ITOF,         "ITOF",         OPERAND_NONE },
    { OP_FTOB,         "FTOB",         OPERAND_NONE },
    { OP_FTOI,         "FTOI",         OPERAND_NONE },
    // array
    { OP_ARRAYLOCAL,   "ARRAYLOCAL",   OPERAND_NONE },
    // debug
    { OP_PUSH_CHECK_NUM,    "PUSH_CHECK_NUM",   OPERAND_QUAD },
    { OP_POP_CHECK_NUM,     "POP_CHECK_NUM",    OPERAND_QUAD },
    // exit
    { OP_EXIT,         "EXIT",         OPERAND_NONE },
    { OP_EOC,          "EOC",          OPERAND_NONE },
};

// XXX TEST register machine
static_assert(sizeof(opcode_table)/sizeof(opcode_table[0])==OP_EOC + 1, "MISSING_OPCODE_ENTRY");

struct OpcodeInfo__ {
    const char *mnemonic;
    int operand;
    bool extend;
};

static const struct OpcodeInfo__ opcode_table__[] = {
    [OP_NOP__]            = { "nop",      OPERAND____ },
    // load/store/move
    [OP_MOVE__]           = { "move",        OPERAND_AB_ },
    [OP_LOADINT__]        = { "loadint",     OPERAND_A__, true },
    [OP_LOADFLOAT__]      = { "loadfloat",   OPERAND_A__, true },
    [OP_LOAD__]           = { "load",        OPERAND_AB_ },
    [OP_STORE__]          = { "store",       OPERAND_AB_ },
    [OP_LOADARRAY__]      = { "loadarray",   OPERAND_ABC },
    [OP_STOREARRAY__]     = { "storearray",  OPERAND_ABC },
    [OP_LOADSTRUCT__]     = { "loadstruct",  OPERAND_ABC },
    [OP_STORESTRUCT__]    = { "storestruct", OPERAND_ABC },
    [OP_LOADTYPENIL__]    = { "loadtypen",   OPERAND_A__ },
    [OP_LOADTYPEBOOL__]   = { "loadtypeb",   OPERAND_A__ },
    [OP_LOADTYPEINT__]    = { "loadtypei",   OPERAND_A__ },
    [OP_LOADTYPEFLOAT__]  = { "loadtypef",   OPERAND_A__ },
    [OP_LOADTYPESTRING__] = { "loadtypes",   OPERAND_A__ },
/* TODO remove address operations */
    [OP_LOADADDR__]       = { "loadaddr",    OPERAND_AB_ },
    [OP_DEREF__]          = { "deref",       OPERAND_AB_ },
/* ------------------------------ */
    // array/struct
    [OP_NEWARRAY__]       = { "newarray",    OPERAND_AB_ },
    [OP_NEWSTRUCT__]      = { "newstruct",   OPERAND_AB_ },
    // arithmetic
    [OP_ADDINT__]         = { "addint",      OPERAND_ABC },
    [OP_ADDFLOAT__]       = { "addfloat",    OPERAND_ABC },
    [OP_SUBINT__]         = { "subint",      OPERAND_ABC },
    [OP_SUBFLOAT__]       = { "subfloat",    OPERAND_ABC },
    [OP_MULINT__]         = { "mulint",      OPERAND_ABC },
    [OP_MULFLOAT__]       = { "mulfloat",    OPERAND_ABC },
    [OP_DIVINT__]         = { "divint",      OPERAND_ABC },
    [OP_DIVFLOAT__]       = { "divfloat",    OPERAND_ABC },
    [OP_REMINT__]         = { "remint",      OPERAND_ABC },
    [OP_REMFLOAT__]       = { "remfloat",    OPERAND_ABC },
    [OP_EQINT__]          = { "eqint",       OPERAND_ABC },
    [OP_EQFLOAT__]        = { "eqfloat",     OPERAND_ABC },
    [OP_NEQINT__]         = { "neqint",      OPERAND_ABC },
    [OP_NEQFLOAT__]       = { "neqfloat",    OPERAND_ABC },
    [OP_LTINT__]          = { "ltint",       OPERAND_ABC },
    [OP_LTFLOAT__]        = { "ltfloat",     OPERAND_ABC },
    [OP_LTEINT__]         = { "lteint",      OPERAND_ABC },
    [OP_LTEFLOAT__]       = { "ltefloat",    OPERAND_ABC },
    [OP_GTINT__]          = { "gtint",       OPERAND_ABC },
    [OP_GTFLOAT__]        = { "gtfloat",     OPERAND_ABC },
    [OP_GTEINT__]         = { "gteint",      OPERAND_ABC },
    [OP_GTEFLOAT__]       = { "gtefloat",    OPERAND_ABC },
    [OP_BITWISEAND__]     = { "bitwiseand",  OPERAND_ABC },
    [OP_BITWISEOR__]      = { "bitwiseor",   OPERAND_ABC },
    [OP_BITWISEXOR__]     = { "bitwisexor",  OPERAND_ABC },
    [OP_BITWISENOT__]     = { "bitwisenot",  OPERAND_AB_ },
    [OP_SHL__]            = { "shl",         OPERAND_ABC },
    [OP_SHR__]            = { "shr",         OPERAND_ABC },
    [OP_NEGINT__]         = { "negint",      OPERAND_AB_ },
    [OP_NEGFLOAT__]       = { "negfloat",    OPERAND_AB_ },
    [OP_SETIFZERO__]      = { "setifzero",   OPERAND_AB_ },
    [OP_SETIFNOTZ__]      = { "setifnotz",   OPERAND_AB_ },
    [OP_INC__]            = { "inc",         OPERAND_A__ },
    [OP_DEC__]            = { "dec",         OPERAND_A__ },
    // string
    [OP_CATSTRING__]      = { "catstring",   OPERAND_ABC },
    [OP_EQSTRING__]       = { "eqstring",    OPERAND_ABC },
    [OP_NEQSTRING__]      = { "neqstring",   OPERAND_ABC },
    // function call
    [OP_CALL__]           = { "call",        OPERAND_ABB },
    [OP_CALLBUILTIN__]    = { "callbuiltin", OPERAND_ABB },
    [OP_RETURN__]         = { "return",      OPERAND_A__ },
    // jump
    [OP_JUMP__]           = { "jump",        OPERAND_ABB },
    [OP_JUMPIFZERO__]     = { "jumpifzero",  OPERAND_ABB },
    [OP_JUMPIFNOTZ__]     = { "jumpifnotz",  OPERAND_ABB },
    // stack operation
    [OP_ALLOCATE__]       = { "allocate",    OPERAND_A__ },
    // conversion
    [OP_BOOLTOINT__]      = { "booltoint",   OPERAND_AB_ },
    [OP_BOOLTOFLOAT__]    = { "booltofloat", OPERAND_AB_ },
    [OP_INTTOBOOL__]      = { "inttobool",   OPERAND_AB_ },
    [OP_INTTOFLOAT__]     = { "inttofloat",  OPERAND_AB_ },
    [OP_FLOATTOBOOL__]    = { "floattobool", OPERAND_AB_ },
    [OP_FLOATTOINT__]     = { "floattoint",  OPERAND_AB_ },
    // program control
    [OP_EXIT__]           = { "exit",        OPERAND____ },
    [OP_EOC__]            = { "eoc",         OPERAND____ },
    [END_OF_OPCODE__]     = { NULL },
};

static_assert(sizeof(opcode_table__)/sizeof(opcode_table__[0])==END_OF_OPCODE__+1, "MISSING_OPCODE_ENTRY");

static const struct OpcodeInfo__ *lookup_opcode_info__(Byte op)
{
    if (op >= END_OF_OPCODE__) {
        InternalError(__FILE__, __LINE__, "opcode out of range: %d\n", op);
    }

    return &opcode_table__[op];
}

const struct OpcodeInfo *LookupOpcodeInfo(Byte op)
{
    int N = sizeof(opcode_table)/sizeof(opcode_table[0]);

    for (int i = 0; i < N; i++) {
        if (op == opcode_table[i].opcode)
            return &opcode_table[i];
    }
    return &opcode_table[0];
}

const char *OpcodeString(Byte op)
{
    const struct OpcodeInfo *info = LookupOpcodeInfo(op);
    return info->mnemonic;
}

static int new_cap(int cur_cap, int min_cap)
{
    return cur_cap < min_cap ? min_cap : 2 * cur_cap;
}

static void push_byte(ByteVec *v, Byte data)
{
    if (v->len + 1 > v->cap) {
        v->cap = new_cap(v->cap, 128);
        // TODO Remove cast
        v->data = (Byte *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = data;
}

static void push_word(ByteVec *v, Word data)
{
    int sz = sizeof(data);
    if (v->len + sz > v->cap) {
        v->cap = new_cap(v->cap, 128);
        // TODO Remove cast
        v->data = (Byte *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    memcpy(&v->data[v->len], &data, sz);
    v->len += sz;
}

static void push_int(ByteVec *v, Int data)
{
    int sz = sizeof(data);
    if (v->len + sz > v->cap) {
        v->cap = new_cap(v->cap, 128);
        // TODO Remove cast
        v->data = (Byte *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    memcpy(&v->data[v->len], &data, sz);
    v->len += sz;
}

static void push_float(ByteVec *v, Float data)
{
    int sz = sizeof(data);
    if (v->len + sz > v->cap) {
        v->cap = new_cap(v->cap, 128);
        // TODO Remove cast
        v->data = (Byte *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    memcpy(&v->data[v->len], &data, sz);
    v->len += sz;
}

void PushPtr(PtrVec *v, void *data)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 8);
        // TODO Remove cast
        v->data = (char **) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = (char *) data;
}

static void push_info(FuncInfoVec *v, Word id, Byte argc, Int addr)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 8);
        // TODO Remove cast
        v->data = (FuncInfo *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    FuncInfo *info = &v->data[v->len++];
    info->id = id;
    info->argc = argc;
    info->addr = addr;
}

static void assert_range(const FuncInfoVec *v,  Word index)
{
    if (index >= v->len) {
        InternalError(__FILE__, __LINE__,
                "function index out of range: %d, function count: %d\n",
                index, v->len);
    }
}

// instruction vector
#define MIN_CAP 8
void PushInstVec(struct InstVec *v, uint32_t val)
{
    if (v->len == v->cap) {
        v->cap = v->cap < MIN_CAP ? MIN_CAP : 2 * v->cap;
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_immediate_value(struct Bytecode *code, int operand);

static void push_inst(struct Bytecode *code, uint32_t inst)
{
    PushInstVec(&code->insts, inst);
}

static void push_inst_op(struct Bytecode *code, uint8_t op)
{
    uint32_t inst = (op << 24);

    push_inst(code, inst);
}

static void push_inst_a(struct Bytecode *code, uint8_t op, uint8_t a)
{
    uint32_t inst = (op << 24) | (a << 16);

    push_inst(code, inst);
    push_immediate_value(code, a);
}

static void push_inst_ab(struct Bytecode *code, uint8_t op, uint8_t a, uint8_t b)
{
    uint32_t inst = (op << 24) | (a << 16) | (b << 8);

    push_inst(code, inst);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
}

static void push_inst_abc(struct Bytecode *code, uint8_t op, uint8_t a, uint8_t b, uint8_t c)
{
    uint32_t inst = (op << 24) | (a << 16) | (b << 8) | c;

    push_inst(code, inst);
    push_immediate_value(code, a);
    push_immediate_value(code, b);
    push_immediate_value(code, c);
}

static void push_inst_abb(struct Bytecode *code, uint8_t op, uint8_t a, uint16_t bb)
{
    uint32_t inst = ENCODE_ABB(op, a, bb);

    push_inst(code, inst);
    push_immediate_value(code, a);
}

/*
static void push_immediate_int(struct Bytecode *code, int64_t val)
{
    uint32_t lo = val & 0xFFFFFFFF;
    uint32_t hi = val >> 32;
    push_inst(code, lo);
    push_inst(code, hi);
}

static void push_immediate_float(struct Bytecode *code, double val)
{
    int64_t *vp = (int64_t*) &val;
    uint32_t lo = *vp & 0xFFFFFFFF;
    uint32_t hi = *vp >> 32;
    push_inst(code, lo);
    push_inst(code, hi);
}
*/

void LoadByte(Bytecode *code, Byte byte)
{
    push_byte(&code->bytes_, OP_LOADB);
    push_byte(&code->bytes_, byte);
}

void LoadInt(Bytecode *code, Int integer)
{
    if (integer >= 0 && integer <= UINT8_MAX) {
        push_byte(&code->bytes_, OP_LOADB);
        push_byte(&code->bytes_, integer);
    }
    else {
        push_byte(&code->bytes_, OP_LOADI);
        push_int(&code->bytes_, integer);
    }
}

void LoadFloat(Bytecode *code, Float fp)
{
    push_byte(&code->bytes_, OP_LOADF);
    push_float(&code->bytes_, fp);
}

void LoadString(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_LOADS);
    push_word(&code->bytes_, id);
}

void LoadLocal(Bytecode *code, Byte id)
{
    push_byte(&code->bytes_, OP_LOADLOCAL);
    push_byte(&code->bytes_, id);
}

void LoadGlobal(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_LOADGLOBAL);
    push_word(&code->bytes_, id);
}

void StoreLocal(Bytecode *code, Byte id)
{
    push_byte(&code->bytes_, OP_STORELOCAL);
    push_byte(&code->bytes_, id);
}

void StoreGlobal(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_STOREGLOBAL);
    push_word(&code->bytes_, id);
}

void Load(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOAD);
}

void Store(Bytecode *code)
{
    push_byte(&code->bytes_, OP_STORE);
}

void IncLocal(Bytecode *code, Byte id)
{
    push_byte(&code->bytes_, OP_INCLOCAL);
    push_byte(&code->bytes_, id);
}

void IncGlobal(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_INCGLOBAL);
    push_word(&code->bytes_, id);
}

void DecLocal(Bytecode *code, Byte id)
{
    push_byte(&code->bytes_, OP_DECLOCAL);
    push_byte(&code->bytes_, id);
}

void DecGlobal(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_DECGLOBAL);
    push_word(&code->bytes_, id);
}

void Allocate(Bytecode *code, Byte count)
{
    if (count == 0)
        return;

    push_byte(&code->bytes_, OP_ALLOC);
    push_byte(&code->bytes_, count);
}

void ClearLocal(Bytecode *code, uint16_t base, uint16_t count)
{
    if (count == 0)
        return;

    push_byte(&code->bytes_, OP_CLEAR_LOCAL);
    push_word(&code->bytes_, base);
    push_word(&code->bytes_, count);
}

void ClearGlobal(Bytecode *code, uint16_t base, uint16_t count)
{
    if (count == 0)
        return;

    push_byte(&code->bytes_, OP_CLEAR_GLOBAL);
    push_word(&code->bytes_, base);
    push_word(&code->bytes_, count);
}

void CopyLocal(Bytecode *code, uint16_t src, uint16_t dst, uint16_t count)
{
    if (count == 0)
        return;

    push_byte(&code->bytes_, OP_COPY_LOCAL);
    push_word(&code->bytes_, src);
    push_word(&code->bytes_, dst);
    push_word(&code->bytes_, count);
}

void CopyGlobal(Bytecode *code, uint16_t src, uint16_t dst, uint16_t count)
{
    if (count == 0)
        return;

    push_byte(&code->bytes_, OP_COPY_GLOBAL);
    push_word(&code->bytes_, src);
    push_word(&code->bytes_, dst);
    push_word(&code->bytes_, count);
}

void LoadAddress(Bytecode *code, Word id)
{
    push_byte(&code->bytes_, OP_LOADA);
    push_word(&code->bytes_, id);
}

void Dereference(Bytecode *code)
{
    push_byte(&code->bytes_, OP_DEREF);
}

void Index(Bytecode *code)
{
    push_byte(&code->bytes_, OP_INDEX);
}

void LoadTypeNil(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOADTYPEN);
}

void LoadTypeBool(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOADTYPEB);
}

void LoadTypeInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOADTYPEI);
}

void LoadTypeFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOADTYPEF);
}

void LoadTypeString(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LOADTYPES);
}

void CallFunc(Bytecode *code, const char *fullname, bool builtin)
{
    if (builtin) {
        // XXX builtin funcs are not registered in bytecode
        // come up with better idea
        uint16_t func_index = 0;
        if (!strcmp("fullname", "print"))
            func_index = 0;
        else if (!strcmp("fullname", "exit"))
            func_index = 1;
        // emit
        push_byte(&code->bytes_, OP_CALL_BUILTIN);
        push_byte(&code->bytes_, func_index);
        return;
    }

    // lookup
    struct MapEntry *ent = HashMapLookup(&code->funcnames, fullname);
    if (!ent) {
        InternalError(__FILE__, __LINE__, "no function registered: %s\n", fullname);
    }

    // emit
    uint64_t func_index = (uint64_t) ent->val;
    push_byte(&code->bytes_, OP_CALL);
    push_word(&code->bytes_, func_index);
}

void CallFunction(Bytecode *code, Word func_index, bool builtin)
{
    if (builtin) {
        push_byte(&code->bytes_, OP_CALL_BUILTIN);
        push_byte(&code->bytes_, func_index);
    }
    else {
        push_byte(&code->bytes_, OP_CALL);
        push_word(&code->bytes_, func_index);
    }
}

void CallFunctionPointer(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_CALL_POINTER);
}

Int JumpIfZero(Bytecode *code, Int addr)
{
    push_byte(&code->bytes_, OP_JEQ);
    const Int operand_addr = NextAddr(code);
    push_word(&code->bytes_, addr);

    return operand_addr;
}

Int Jump(Bytecode *code, Int addr)
{
    push_byte(&code->bytes_, OP_JMP);
    const Int operand_addr = NextAddr(code);
    push_word(&code->bytes_, addr);

    return operand_addr;
}

void Return(Bytecode *code)
{
    push_byte(&code->bytes_, OP_RET);
}

void AddInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_ADD);
}

void AddFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_ADDF);
}

void ConcatString(Bytecode *code)
{
    push_byte(&code->bytes_, OP_CATS);
}

void SubInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SUB);
}

void SubFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SUBF);
}

void MulInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_MUL);
}

void MulFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_MULF);
}

void DivInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_DIV);
}

void DivFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_DIVF);
}

void RemInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_REM);
}

void RemFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_REMF);
}

void EqualInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_EQ);
}

void EqualFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_EQF);
}

void EqualString(Bytecode *code)
{
    push_byte(&code->bytes_, OP_EQS);
}

void NotEqualInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NEQ);
}

void NotEqualFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NEQF);
}

void NotEqualString(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NEQS);
}

void LessInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LT);
}

void LessFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LTF);
}

void LessEqualInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LTE);
}

void LessEqualFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_LTEF);
}

void GreaterInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_GT);
}

void GreaterFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_GTF);
}

void GreaterEqualInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_GTE);
}

void GreaterEqualFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_GTEF);
}

void And(Bytecode *code)
{
    push_byte(&code->bytes_, OP_AND);
}

void Or(Bytecode *code)
{
    push_byte(&code->bytes_, OP_OR);
}

void Xor(Bytecode *code)
{
    push_byte(&code->bytes_, OP_XOR);
}

void Not(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NOT);
}

void ShiftLeft(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SHL);
}

void ShiftRight(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SHR);
}

void NegateInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NEG);
}

void NegateFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_NEGF);
}

void SetIfZero(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SETZ);
}

void SetIfNotZero(Bytecode *code)
{
    push_byte(&code->bytes_, OP_SETNZ);
}

void Pop(Bytecode *code)
{
    push_byte(&code->bytes_, OP_POP);
}

void DuplicateTop(Bytecode *code)
{
    push_byte(&code->bytes_, OP_DUP);
}

void BoolToInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_BTOI);
}

void BoolToFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_BTOF);
}

void IntToBool(Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOB);
}

void IntToFloat(Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOF);
}

void FloatToBool(Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOB);
}

void FloatToInt(Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOI);
}

void ArrayLocal(Bytecode *code, Byte id)
{
    push_byte(&code->bytes_, OP_ARRAYLOCAL);
}

void PushCheckNum(Bytecode *code, int64_t num)
{
    push_byte(&code->bytes_, OP_PUSH_CHECK_NUM);
    push_int(&code->bytes_, num);
}

void PopCheckNum(Bytecode *code, int64_t num)
{
    push_byte(&code->bytes_, OP_POP_CHECK_NUM);
    push_int(&code->bytes_, num);
}

void Exit(Bytecode *code)
{
    push_byte(&code->bytes_, OP_EXIT);
}

void End(Bytecode *code)
{
    push_byte(&code->bytes_, OP_EOC);
}

static bool is_localreg_full(const struct Bytecode *code)
{
    return code->curr_reg == IMMEDIATE_SMALLINT_BEGIN - 1;
}

void InitLocalVarRegister__(struct Bytecode *code, uint8_t lvar_count)
{
    code->base_reg = lvar_count - 1;
    code->curr_reg = code->base_reg;
    code->max_reg = code->base_reg;
}

void ResetCurrentRegister__(struct Bytecode *code)
{
    code->curr_reg = code->base_reg;
}

int NewRegister__(struct Bytecode *code)
{
    if (is_localreg_full(code)) {
        return -1;
    }

    code->curr_reg++;
    if (code->max_reg < code->curr_reg)
        code->max_reg = code->curr_reg;

    return code->curr_reg;
}

int GetCurrentRegister__(const struct Bytecode *code)
{
    return code->curr_reg;
}

int SetCurrentRegister__(struct Bytecode *code, int curr)
{
    code->curr_reg = curr;
    return curr;
}

int GetNextRegister__(struct Bytecode *code, int reg)
{
    int next = -1;

    if (reg == code->max_reg) {
        next = NewRegister__(code);
    }
    else {
        next = reg + 1;
        code->curr_reg = next;
    }

    return next;
}

bool IsTempRegister(const struct Bytecode *code, int id)
{
    return id > code->base_reg && !IsImmediateValue__(id);
}

// TODO remove and embed
static bool can_fit_smallint(int64_t val)
{
    int SMALLINT_SIZE = IMMEDIATE_SMALLINT_END - IMMEDIATE_SMALLINT_BEGIN + 1;
    return val >= 0 && val < SMALLINT_SIZE;
}

// TODO remove and embed
static bool can_fit_int32(int64_t val)
{
    return val >= INT32_MIN && val <= INT32_MAX;
}

bool IsImmediateValue__(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN;
}

static bool is_smallint_register(int id)
{
    return id >= IMMEDIATE_SMALLINT_BEGIN && id <= IMMEDIATE_SMALLINT_END;
}

static bool is_constpool_register(int id)
{
    return id > IMMEDIATE_SMALLINT_END && id <= 0xFF;
}

int register_to_smallint(int id)
{
    return id - IMMEDIATE_SMALLINT_BEGIN;
}

int smallint_to_register(int64_t val)
{
    return val + IMMEDIATE_SMALLINT_BEGIN;
}

static void push_immediate_value(struct Bytecode *code, int operand)
{
    if (!is_constpool_register(operand))
        return;

    int64_t val = PopInt(&code->immediate_ints);
    int32_t id = val & 0xFFFFFFFF;
    push_inst(code, id);
}

struct Value ReadImmediateValue__(const struct Bytecode *code, Int addr, int id, int *imm_size)
{
    struct Value value;

    if (is_smallint_register(id)) {
        value.inum = register_to_smallint(id);
        return value;
    }

    switch (id) {

    case IMMEDIATE_INT32:
        {
            int32_t imm = Read__(code, addr);
            value.inum = imm;
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_INT64:
        {
            int32_t id = Read__(code, addr);
            value = code_constant_pool_get_int(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            int32_t id = Read__(code, addr);
            value = code_constant_pool_get_float(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;

    case IMMEDIATE_STRING:
        {
            int64_t id = Read__(code, addr);
            value = code_constant_pool_get_string(&code->const_pool, id);
            if (imm_size)
                *imm_size += 1;
        }
        break;
    }

    return value;
}

// Load/store/move
int Move__(Bytecode *code, Byte dst, Byte src)
{
    if (dst == src)
        return dst;
    push_inst_ab(code, OP_MOVE__, dst, src);
    return dst;
}

int LoadInt__(struct Bytecode *code, int64_t val)
{
    if (can_fit_smallint(val)) {
        return smallint_to_register(val);
    }
    else if (can_fit_int32(val)) {
        PushInt(&code->immediate_ints, val);
        return IMMEDIATE_INT32;
    }
    else {
        int id = code_constant_pool_push_int(&code->const_pool, val);
        PushInt(&code->immediate_ints, id);
        return IMMEDIATE_INT64;
    }
}

int LoadFloat__(struct Bytecode *code, double val)
{
    int id = code_constant_pool_push_float(&code->const_pool, val);
    PushInt(&code->immediate_ints, id);
    return IMMEDIATE_FLOAT;
}

int LoadString__(struct Bytecode *code, const char *cstr)
{
    int id = code_constant_pool_push_string(&code->const_pool, cstr);
    PushInt(&code->immediate_ints, id);
    return IMMEDIATE_STRING;
}

int Load__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_LOAD__, dst, src);
    return dst;
}

int Store__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_STORE__, dst, src);
    return dst;
}

int LoadArray__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t idx)
{
    push_inst_abc(code, OP_LOADARRAY__, dst, src, idx);
    return dst;
}

int StoreArray__(struct Bytecode *code, uint8_t dst, uint8_t idx, uint8_t src)
{
    push_inst_abc(code, OP_STOREARRAY__, dst, idx, src);
    return dst;
}

int LoadStruct__(struct Bytecode *code, uint8_t dst, uint8_t src, uint8_t field_idx)
{
    push_inst_abc(code, OP_LOADSTRUCT__, dst, src, field_idx);
    return dst;
}

int StoreStruct__(struct Bytecode *code, uint8_t dst, uint8_t field_idx, uint8_t src)
{
    push_inst_abc(code, OP_STORESTRUCT__, dst, field_idx, src);
    return dst;
}

int LoadTypeNil__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPENIL__, dst);
    return dst;
}

int LoadTypeBool__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEBOOL__, dst);
    return dst;
}

int LoadTypeInt__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEINT__, dst);
    return dst;
}

int LoadTypeFloat__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPEFLOAT__, dst);
    return dst;
}

int LoadTypeString__(struct Bytecode *code, int dst)
{
    push_inst_a(code, OP_LOADTYPESTRING__, dst);
    return dst;
}

/* TODO remove address operations */
int LoadAddress__(struct Bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_LOADADDR__, dst, src);
    return dst;
}

int Dereference__(struct Bytecode *code, int dst, int src)
{
    push_inst_ab(code, OP_DEREF__, dst, src);
    return dst;
}
/* ------------------------------ */

// array/struct
int NewArray__(struct Bytecode *code, uint8_t dst, uint8_t len)
{
    push_inst_ab(code, OP_NEWARRAY__, dst, len);
    return dst;
}

int NewStruct__(struct Bytecode *code, uint8_t dst, uint8_t len)
{
    push_inst_ab(code, OP_NEWSTRUCT__, dst, len);
    return dst;
}

// arithmetic
int AddInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_ADDINT__, dst, src0, src1);
    return dst;
}

int AddFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_ADDFLOAT__, dst, src0, src1);
    return dst;
}

int SubInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SUBINT__, dst, src0, src1);
    return dst;
}

int SubFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SUBFLOAT__, dst, src0, src1);
    return dst;
}

int MulInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_MULINT__, dst, src0, src1);
    return dst;
}

int MulFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_MULFLOAT__, dst, src0, src1);
    return dst;
}

int DivInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_DIVINT__, dst, src0, src1);
    return dst;
}

int DivFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_DIVFLOAT__, dst, src0, src1);
    return dst;
}

int RemInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_REMINT__, dst, src0, src1);
    return dst;
}

int RemFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_REMFLOAT__, dst, src0, src1);
    return dst;
}

int EqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQINT__, dst, src0, src1);
    return dst;
}

int EqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQFLOAT__, dst, src0, src1);
    return dst;
}

int NotEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQINT__, dst, src0, src1);
    return dst;
}

int NotEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQFLOAT__, dst, src0, src1);
    return dst;
}

int LessInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTINT__, dst, src0, src1);
    return dst;
}

int LessFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTFLOAT__, dst, src0, src1);
    return dst;
}

int LessEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTEINT__, dst, src0, src1);
    return dst;
}

int LessEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_LTEFLOAT__, dst, src0, src1);
    return dst;
}

int GreaterInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTINT__, dst, src0, src1);
    return dst;
}

int GreaterFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTFLOAT__, dst, src0, src1);
    return dst;
}

int GreaterEqualInt__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTEINT__, dst, src0, src1);
    return dst;
}

int GreaterEqualFloat__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_GTEFLOAT__, dst, src0, src1);
    return dst;
}

int BitwiseAnd__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEAND__, dst, src0, src1);
    return dst;
}

int BitwiseOr__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEOR__, dst, src0, src1);
    return dst;
}

int BitwiseXor__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_BITWISEXOR__, dst, src0, src1);
    return dst;
}

int BitwiseNot__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_BITWISENOT__, dst, src);
    return dst;
}

int ShiftLeft__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SHL__, dst, src0, src1);
    return dst;
}

int ShiftRight__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_SHR__, dst, src0, src1);
    return dst;
}

int NegateInt__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_NEGINT__, dst, src);
    return dst;
}

int NegateFloat__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_NEGFLOAT__, dst, src);
    return dst;
}

int SetIfZero__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_SETIFZERO__, dst, src);
    return dst;
}

int SetIfNotZero__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_SETIFNOTZ__, dst, src);
    return dst;
}

int Inc__(struct Bytecode *code, uint8_t src)
{
    push_inst_a(code, OP_INC__, src);
    return src;
}

int Dec__(struct Bytecode *code, uint8_t src)
{
    push_inst_a(code, OP_DEC__, src);
    return src;
}

// string
int ConcatString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_CATSTRING__, dst, src0, src1);
    return dst;
}

int EqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_EQSTRING__, dst, src0, src1);
    return dst;
}

int NotEqualString__(struct Bytecode *code, uint8_t dst, uint8_t src0, uint8_t src1)
{
    push_inst_abc(code, OP_NEQSTRING__, dst, src0, src1);
    return dst;
}

// Function call
int CallFunction__(Bytecode *code, Byte ret_reg, Word func_index, bool builtin)
{
    int reg0 = ret_reg;

    if (builtin) {
        push_inst_abb(code, OP_CALLBUILTIN__, reg0, func_index);
    }
    else {
        push_inst_abb(code, OP_CALL__, reg0, func_index);
    }

    return reg0;
}

void Allocate__(Bytecode *code, Byte count)
{
    if (count == 0)
        return;

    push_inst_a(code, OP_ALLOCATE__, count);
}

void Return__(Bytecode *code, Byte id)
{
    push_inst_a(code, OP_RETURN__, id);
}

// branch
void BeginIf__(struct Bytecode *code)
{
    PushInt(&code->ors_, -1);
}

void code_begin_switch(struct Bytecode *code)
{
    PushInt(&code->casecloses_, -1);
}

void PushElseEnd__(struct Bytecode *code, Int addr)
{
    PushInt(&code->ors_, addr);
}

void PushBreak__(struct Bytecode *code, Int addr)
{
    PushInt(&code->breaks_, addr);
}

void PushContinue__(struct Bytecode *code, Int addr)
{
    PushInt(&code->continues_, addr);
}

void PushCaseEnd__(struct Bytecode *code, Int addr)
{
    PushInt(&code->casecloses_, addr);
}

void code_push_continue(struct Bytecode *code, Int addr)
{
    PushInt(&code->continues_, addr);
}

// jump instructions return the address
// where the destination address is stored.
Int Jump__(struct Bytecode *code, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMP__, 0, addr);
    return operand_addr;
}

Int JumpIfZero__(struct Bytecode *code, uint8_t id, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMPIFZERO__, id, addr);
    return operand_addr;
}

Int JumpIfNotZero__(struct Bytecode *code, uint8_t id, Int addr)
{
    Int operand_addr = NextAddr__(code);
    push_inst_abb(code, OP_JUMPIFNOTZ__, id, addr);
    return operand_addr;
}

// conversion
int BoolToInt__(struct Bytecode *code, uint8_t dst, uint8_t src)
{
    push_inst_ab(code, OP_BOOLTOINT__, dst, src);
    return dst;
}

/*
void BoolToFloat__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_BTOF);
}

void IntToBool__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOB);
}

void IntToFloat__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_ITOF);
}

void FloatToBool__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOB);
}

void FloatToInt__(struct Bytecode *code)
{
    push_byte(&code->bytes_, OP_FTOI);
}
*/

// program control
void Exit__(Bytecode *code)
{
    push_inst_op(code, OP_EXIT__);
}

void End__(Bytecode *code)
{
    push_inst_op(code, OP_EOC__);
}

// Functions
void RegisterFunction__(Bytecode *code, Word func_index, Byte argc)
{
    const Word next_index = code->funcs_.len;

    if (func_index != next_index) {
        InternalError(__FILE__, __LINE__,
                "function index %d and next index %d should match\n",
                func_index, next_index);
    }

    const Int next_addr = NextAddr__(code);
    push_info(&code->funcs_, func_index, argc, next_addr);
}

void SetMaxRegisterCount__(Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    code->funcs_.data[func_index].reg_count = code->max_reg + 1;
}

int GetMaxRegisterCount__(const struct Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.len) {
        InternalError(__FILE__, __LINE__, "function index out of range %d\n", func_index);
    }

    return code->funcs_.data[func_index].reg_count;
}

void BeginFor__(struct Bytecode *code)
{
    PushInt(&code->breaks_, -1);
    PushInt(&code->continues_, -1);
}

void BackPatch__(struct Bytecode *code, Int operand_addr)
{
    Int next_addr = NextAddr__(code);
    uint32_t inst = Read__(code, operand_addr);

    inst = (inst & 0xFFFF0000) | (next_addr & 0x0000FFFF);
    Write__(code, operand_addr, inst);
}

void BackPatchBreaks__(struct Bytecode *code)
{
    while (!IsEmptyInt(&code->breaks_)) {
        Int addr = PopInt(&code->breaks_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void BackPatchElseEnds__(struct Bytecode *code)
{
    while (!IsEmptyInt(&code->ors_)) {
        Int addr = PopInt(&code->ors_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void BackPatchContinues__(struct Bytecode *code)
{
    while (!IsEmptyInt(&code->continues_)) {
        Int addr = PopInt(&code->continues_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void code_backpatch_case_ends(Bytecode *code)
{
    while (!IsEmptyInt(&code->casecloses_)) {
        Int addr = PopInt(&code->casecloses_);
        if (addr == -1)
            break;
        BackPatch__(code, addr);
    }
}

void Decode__(uint32_t instcode, struct Instruction *inst)
{
    Byte op = DECODE_OP(instcode);
    const struct OpcodeInfo__ *info = lookup_opcode_info__(op);

    inst->op = op;

    switch (info->operand) {

    case OPERAND____:
        break;

    case OPERAND_A__:
        inst->A = DECODE_A(instcode);
        break;

    case OPERAND_AB_:
        inst->A = DECODE_A(instcode);
        inst->B = DECODE_B(instcode);
        break;

    case OPERAND_ABC:
        inst->A = DECODE_A(instcode);
        inst->B = DECODE_B(instcode);
        inst->C = DECODE_C(instcode);
        break;

    case OPERAND_ABB:
        inst->A = DECODE_A(instcode);
        inst->BB = DECODE_BB(instcode);
        break;
    }
}

static Int print_op__(const Bytecode *code, Int addr, const struct Instruction *inst, int *imm_size);
void PrintInstruction__(const struct Bytecode *code,
        Int addr, const struct Instruction *inst, int *imm_size)
{
    print_op__(code, addr, inst, imm_size);
}

uint32_t Read__(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size__(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size__(code));

    return code->insts.data[addr];
}

void Write__(const Bytecode *code, Int addr, uint32_t inst)
{
    if (addr < 0 || addr >= Size__(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size__(code));

    code->insts.data[addr] = inst;
}

Int Size__(const Bytecode *code)
{
    return code->insts.len;
}

Int NextAddr__(const struct Bytecode *code)
{
    return Size__(code);
}

Int GetFunctionAddress(const Bytecode *code, Word func_index)
{
    assert_range(&code->funcs_, func_index);
    return code->funcs_.data[func_index].addr;
}

Int GetFunctionArgCount(const Bytecode *code, Word func_index)
{
    assert_range(&code->funcs_, func_index);
    return code->funcs_.data[func_index].argc;
}

void BackPatchFuncAddr(struct Bytecode *code, const char *fullname)
{
    struct MapEntry *ent = HashMapLookup(&code->funcnames, fullname);
    if (!ent)
        return;

    uint64_t func_index = (uint64_t) ent->val;
    code->funcs_.data[func_index].addr = NextAddr(code);
}

uint16_t RegisterFunc(struct Bytecode *code, const char *fullname, uint8_t argc)
{
    struct MapEntry *ent = HashMapLookup(&code->funcnames, fullname);
    if (ent)
        return (uint64_t) ent->val;

    const uint64_t next_index = code->funcs_.len;
    HashMapInsert(&code->funcnames, fullname, (void *)next_index);

    const Int next_addr = NextAddr(code);
    push_info(&code->funcs_, next_index, argc, next_addr);

    return next_index;
}

void RegisterFunction(Bytecode *code, Word func_index, Byte argc)
{
    const Word next_index = code->funcs_.len;

    if (func_index != next_index) {
        InternalError(__FILE__, __LINE__,
                "function func_index %d and next index %d should match\n",
                func_index, next_index);
    }

    const Int next_addr = NextAddr(code);
    push_info(&code->funcs_, func_index, argc, next_addr);
}

Int RegisterConstString(Bytecode *code, const char *str)
{
    const Word next_index = code->strings_.len;

    PushPtr(&code->strings_, strdup(str));

    return next_index;
}

const char *GetConstString(const Bytecode *code, Word str_index)
{
    if (str_index < 0 || str_index >= code->strings_.len) {
        InternalError(__FILE__, __LINE__,
                "index out of range: %d", str_index);
    }
    return code->strings_.data[str_index];
}

Byte Read(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size(code));

    return code->bytes_.data[addr];
}

Word ReadWord(const Bytecode *code, Int addr)
{
    int sz = sizeof(Word);
    if (addr < 0 || addr + sz > code->bytes_.len) {
        InternalError(__FILE__, __LINE__,
                "address %d out of range: %d",
                addr, code->bytes_.len);
    }
    return *((Word*) &code->bytes_.data[addr]);
}

Int ReadInt(const Bytecode *code, Int addr)
{
    int sz = sizeof(Int);
    if (addr < 0 || addr + sz > code->bytes_.len) {
        InternalError(__FILE__, __LINE__,
                "address %d out of range: %d",
                addr, code->bytes_.len);
    }
    return *((Int*) &code->bytes_.data[addr]);
}

Float ReadFloat(const Bytecode *code, Int addr)
{
    int sz = sizeof(Float);
    if (addr < 0 || addr + sz > code->bytes_.len) {
        InternalError(__FILE__, __LINE__,
                "address %d out of range: %d",
                addr, code->bytes_.len);
    }
    return *((Float*) &code->bytes_.data[addr]);
}

Int NextAddr(const Bytecode *code)
{
    return Size(code);
}

Int Size(const Bytecode *code)
{
    return code->bytes_.len;
}

void BeginIf(Bytecode *code)
{
    PushInt(&code->ors_, -1);
}

void BeginFor(Bytecode *code)
{
    PushInt(&code->breaks_, -1);
    PushInt(&code->continues_, -1);
}

void BeginSwitch(Bytecode *code)
{
    PushInt(&code->casecloses_, -1);
}

void PushOrClose(Bytecode *code, Int addr)
{
    PushInt(&code->ors_, addr);
}

void PushBreak(Bytecode *code, Int addr)
{
    PushInt(&code->breaks_, addr);
}

void PushContinue(Bytecode *code, Int addr)
{
    PushInt(&code->continues_, addr);
}

void PushCaseClose(Bytecode *code, Int addr)
{
    PushInt(&code->casecloses_, addr);
}

void BackPatch(Bytecode *code, Int operand_addr)
{
    const Int next_addr = NextAddr(code);
    *((Word*) &code->bytes_.data[operand_addr]) = next_addr;
}

void BackPatchOrCloses(Bytecode *code)
{
    while (!IsEmptyInt(&code->ors_)) {
        Int addr = PopInt(&code->ors_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchBreaks(Bytecode *code)
{
    while (!IsEmptyInt(&code->breaks_)) {
        Int addr = PopInt(&code->breaks_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchContinues(Bytecode *code)
{
    while (!IsEmptyInt(&code->continues_)) {
        Int addr = PopInt(&code->continues_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchCaseCloses(Bytecode *code)
{
    while (!IsEmptyInt(&code->casecloses_)) {
        Int addr = PopInt(&code->casecloses_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

static Int print_op(const Bytecode *code, int op, int operand, Int address)
{
    const Int addr = address;
    Int inc = 0;

    const char *mnemonic = OpcodeString(op);

    // padding spaces
    if (operand != OPERAND_NONE)
        printf("%-12s", mnemonic);
    else
        printf("%s", mnemonic);

    char prefix;
    if (op == OP_LOADLOCAL || op == OP_LOADGLOBAL ||
        op == OP_STORELOCAL || op == OP_STOREGLOBAL)
        prefix = '@';
    else
        prefix = '$';

    // append operand
    switch (operand) {

    case OPERAND_BYTE:
        printf(" %c%d", prefix, Read(code, addr));
        inc = 1;
        break;

    case OPERAND_WORD:
        printf(" %c%d", prefix, ReadWord(code, addr));
        inc = sizeof(Word);
        break;

    case OPERAND_WORD2:
        printf(" %c%d", prefix, ReadWord(code, addr));
        printf(" %d", ReadWord(code, addr + sizeof(uint16_t)));
        inc = 2 * sizeof(uint16_t);
        break;

    case OPERAND_WORD3:
        printf(" %c%d", prefix, ReadWord(code, addr));
        printf(" %d", ReadWord(code, addr + sizeof(uint16_t)));
        printf(" %d", ReadWord(code, addr + 2 * sizeof(uint16_t)));
        inc = 3 * sizeof(uint16_t);
        break;

    case OPERAND_QUAD:
        printf(" %c%lld", prefix, ReadInt(code, addr));
        inc = sizeof(Int);
        break;
    }

    // add extra info
    switch (op) {
    case OP_LOADF:
        printf(" = %f", ReadFloat(code, addr));
        break;

    case OP_LOADS:
        printf(" = \"%s\"", GetConstString(code, ReadWord(code, addr)));
        break;

    case OP_CALL:
        // TODO function id could be retrived if we have OP_CALL_STATIC
        // to call functions that are defined statically
        break;
    }

    printf("\n");
    return addr + inc;
}

// XXX TEST
void PrintBytecode__(const Bytecode *code);
void PrintBytecode(const Bytecode *code)
{
    if (REGISTER_MACHINE) {
        PrintBytecode__(code);
        return;
    }

    // function info
    for (int i = 0; i < code->funcs_.len; i++) {
        const FuncInfo *info = &code->funcs_.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < Size(code)) {
        printf("[%6lld] ", addr);

        const int op = Read(code, addr++);
        const struct OpcodeInfo *info = LookupOpcodeInfo(op);
        addr = print_op(code, info->opcode, info->operand_size, addr);

        if (op == OP_EOC)
            break;
    }
}

// XXX TEST
void print_value(struct Value val, int type)
{
    switch (type) {

    case VAL_INT:
        printf("%lld", val.inum);
        break;

    case VAL_FLOAT:
        if (fmod(val.fpnum, 1.) == 0.0)
            printf("%g.0", val.fpnum);
        else
            printf("%g", val.fpnum);
        break;

    case VAL_STRING:
        printf("\"%s\"", val.str->data);
        break;

    default:
        UNREACHABLE;
        break;
    }
}

void PrintBytecode__(const Bytecode *code)
{
    if (code_constant_pool_get_int_count(&code->const_pool) > 0) {
        printf("* constant int:\n");
        int count = code_constant_pool_get_int_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct Value val = code_constant_pool_get_int(&code->const_pool, i);
            printf("[%6d] %lld\n", i, val.inum);
        }
    }

    if (code_constant_pool_get_float_count(&code->const_pool) > 0) {
        printf("* constant float:\n");
        int count = code_constant_pool_get_float_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct Value val = code_constant_pool_get_float(&code->const_pool, i);
            printf("[%6d] %g\n", i, val.fpnum);
        }
    }

    if (code_constant_pool_get_string_count(&code->const_pool) > 0) {
        printf("* constant string:\n");
        int count = code_constant_pool_get_string_count(&code->const_pool);

        for (int i = 0; i < count; i++) {
            struct Value val = code_constant_pool_get_string(&code->const_pool, i);
            printf("[%6d] \"%s\"\n", i, runtime_string_get_cstr(val.str));
        }
    }

    // function info
    for (int i = 0; i < code->funcs_.len; i++) {
        const FuncInfo *info = &code->funcs_.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < Size__(code)) {
        const uint32_t instcode = Read__(code, addr);
        struct Instruction inst = {0};
        int inc = 1;

        Decode__(instcode, &inst);
        int imm_size = 0;
        PrintInstruction__(code, addr, &inst, &imm_size);
        inc += imm_size;

        if (inst.op == OP_EOC)
            break;

        //addr++;
        addr += inc;

        // TODO come up with better way
        const struct OpcodeInfo__ *info = lookup_opcode_info__(inst.op);
        if (info->extend)
            addr += 2;
    }
}

static void print_operand__(const struct Bytecode *code,
        int addr, int operand, bool separator, int *imm_size)
{
    switch (operand) {

    case IMMEDIATE_INT32:
    case IMMEDIATE_INT64:
        {
            struct Value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("$%lld", val.inum);
        }
        break;

    case IMMEDIATE_FLOAT:
        {
            struct Value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("$%g", val.fpnum);
        }
        break;

    case IMMEDIATE_STRING:
        {
            struct Value val = ReadImmediateValue__(code, addr + 1, operand, imm_size);
            printf("\"%s\"", runtime_string_get_cstr(val.str));
        }
        break;

    default:
        if (is_smallint_register(operand)) {
            printf("$%d", register_to_smallint(operand));
        }
        else {
            printf("r%d", operand);
        }
        break;
    }

    if (separator)
        printf(", ");
}

static void print_operand16__(const struct Bytecode *code, int operand)
{
    printf("$%d", operand);
}

static Int print_op__(const Bytecode *code, Int addr, const struct Instruction *inst, int *imm_size)
{
    const struct OpcodeInfo__ *info = lookup_opcode_info__(inst->op);

    if (addr >= 0)
        printf("[%6lld] ", addr);

    // padding spaces
    if (info->operand != OPERAND_NONE)
        printf("%-12s", info->mnemonic);
    else
        printf("%s", info->mnemonic);

    // append operand
    switch (info->operand) {

    case OPERAND_A__:
        if (inst->op == OP_ALLOCATE__)
            print_operand16__(code, inst->A);
        else
            print_operand__(code, addr, inst->A, 0, NULL);
        break;

    case OPERAND_AB_:
        print_operand__(code, addr, inst->A, 1, NULL);
        print_operand__(code, addr, inst->B, 0, imm_size);
        break;

    case OPERAND_ABB:
        print_operand__(code, addr, inst->A, 1, NULL);
        print_operand16__(code, inst->BB);
        break;

    case OPERAND_ABC:
        print_operand__(code, addr, inst->A, 1, imm_size);
        print_operand__(code, addr, inst->B, 1, imm_size);
        print_operand__(code, addr, inst->C, 0, imm_size);
        break;
    }

    if (info->extend) {
        int64_t lo = Read__(code, addr + 1);
        int64_t hi = Read__(code, addr + 2);
        int64_t immediate = (hi << 32) | lo;
        printf(" $%lld", immediate);
    }

    printf("\n");
    return addr;
}
