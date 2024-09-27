#include "bytecode.h"
#include "error.h"
#include "mem.h"
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>

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
    OPERAND_A_B,
    OPERAND_AB_,
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
    // XXX TEST register machine
    { OP_NOP__,        "NOP",          OPERAND____ },
    { OP_LOADBYTE__,   "LOADBYTE",     OPERAND_A_B },
    { OP_CALL__,       "CALL",         OPERAND_AB_ },
    { OP_RETURN__,     "RETURN",       OPERAND_A__ },
    { OP_EXIT__,       "EXIT",         OPERAND____ },
    { OP_EOC__,        "EOC",          OPERAND____ },
};

// XXX TEST register machine
static_assert(sizeof(opcode_table)/sizeof(opcode_table[0])==OP_COUNT__, "MISSING_OPCODE_ENTRY");

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

void push(AddrStack *s, Int addr)
{
    if (!s->data)
        s->data = NALLOC(256, Int);
    s->data[++s->sp] = addr;
}

Int pop(AddrStack *s)
{
    if (!s->data)
        return 0;
    return s->data[s->sp--];
}

Int top(const AddrStack *s)
{
    if (!s->data)
        return 0;
    return s->data[s->sp];
}

bool empty(const AddrStack *s)
{
    return s->sp == 0;
}

static int new_cap(int cur_cap, int min_cap)
{
    return cur_cap < min_cap ? min_cap : cur_cap * 2;
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

static void push_inst(struct Int32Vec *v, uint32_t data)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 128);
        v->data = realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = data;
}

static void push_inst____(struct Bytecode *code, uint8_t op)
{
    const uint32_t inst = (op << 24);
    push_inst(&code->insts, inst);
}

static void push_op_a(struct Bytecode *code, uint8_t op, uint8_t a)
{
    const uint32_t inst = (op << 24) | (a << 16);
    push_inst(&code->insts, inst);
}

static void push_inst_ab(struct Bytecode *code, uint8_t op, uint16_t word)
{
    const uint32_t inst = (op << 24) | (word << 8);
    push_inst(&code->insts, inst);
}

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

// XXX TEST register
int PoolInt__(Bytecode *code, Int val)
{
    if (code->const_count == 127) {
        return -1;
    }
    code->consts[code->const_count].inum = val;

    int reg = code->const_count++;
    return reg + 128;
}

void CallFunction__(Bytecode *code, Word func_index, bool builtin)
{
    if (builtin) {
        /*
        push_byte(&code->bytes_, OP_CALL_BUILTIN);
        push_byte(&code->bytes_, func_index);
        */
    }
    else {
        /*
        push_byte(&code->bytes_, OP_CALL__);
        push_word(&code->bytes_, func_index);
        push_byte(&code->bytes_, 0);
        */
        push_inst_ab(code, OP_CALL__, func_index);
    }
}

void Return__(Bytecode *code, Byte id)
{
    push_op_a(code, OP_RETURN__, id);
}

void Exit__(Bytecode *code)
{
    push_inst____(code, OP_EXIT__);
}

void End__(Bytecode *code)
{
    push_inst____(code, OP_EOC__);
}

Int Size__(const Bytecode *code)
{
    return code->insts.len;
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

uint32_t Read__(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size__(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size__(code));

    return code->insts.data[addr];
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
    push(&code->ors_, -1);
}

void BeginFor(Bytecode *code)
{
    push(&code->breaks_, -1);
    push(&code->continues_, -1);
}

void BeginSwitch(Bytecode *code)
{
    push(&code->casecloses_, -1);
}

void PushOrClose(Bytecode *code, Int addr)
{
    push(&code->ors_, addr);
}

void PushBreak(Bytecode *code, Int addr)
{
    push(&code->breaks_, addr);
}

void PushContinue(Bytecode *code, Int addr)
{
    push(&code->continues_, addr);
}

void PushCaseClose(Bytecode *code, Int addr)
{
    push(&code->casecloses_, addr);
}

void BackPatch(Bytecode *code, Int operand_addr)
{
    const Int next_addr = NextAddr(code);
    *((Word*) &code->bytes_.data[operand_addr]) = next_addr;
}

void BackPatchOrCloses(Bytecode *code)
{
    while (!empty(&code->ors_)) {
        const Int addr = top(&code->ors_);
        pop(&code->ors_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchBreaks(Bytecode *code)
{
    while (!empty(&code->breaks_)) {
        const Int addr = top(&code->breaks_);
        pop(&code->breaks_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchContinues(Bytecode *code)
{
    while (!empty(&code->continues_)) {
        const Int addr = top(&code->continues_);
        pop(&code->continues_);
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchCaseCloses(Bytecode *code)
{
    while (!empty(&code->casecloses_)) {
        const Int addr = top(&code->casecloses_);
        pop(&code->casecloses_);
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
static Int print_op__(const Bytecode *code, int op, int operand, Int address, uint32_t inst);

void PrintBytecode__(const Bytecode *code)
{
    if (code->const_count) {
        printf("* constant pool:\n");
        for (int i = 0; i < code->const_count; i++) {
            printf("  [%3d] %lld\n", i, code->consts[i].inum);
        }
    }

    // function info
    for (int i = 0; i < code->funcs_.len; i++) {
        const FuncInfo *info = &code->funcs_.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < Size__(code)) {
        printf("[%6lld] ", addr);

        const uint32_t inst = Read__(code, addr++);
        const int op = inst >> 24;
        const struct OpcodeInfo *info = LookupOpcodeInfo(op);
        //addr = print_op__(code, info->opcode, info->operand_size, addr);
        print_op__(code, info->opcode, info->operand_size, addr, inst);

        if (op == OP_EOC)
            break;
    }
}

static Int print_op__(const Bytecode *code, int op, int operand, Int address, uint32_t inst)
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

    case OPERAND_A__:
        {
            int a = (inst >> 16) & 0xFF;
            if (a >= 128)
                printf(" C%d", a - 128);
            else
                printf(" R%d", a);
        }
        break;

    case OPERAND_AB_:
        {
            int ab = (inst >> 8) & 0xFFFF;
            printf(" $%d", ab);
        }
        break;

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
        /*
        */
    }

    // add extra info
    /*
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
    */

    printf("\n");
    return addr + inc;
}
