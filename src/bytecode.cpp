#include "bytecode.h"
#include "error.h"
#include <iostream>
#include <iomanip>
#include <cstring>

const char *OpcodeString(Byte op)
{
    switch (op) {

#define OP(opcode, operand_size) case opcode: return #opcode;
    BYTECODE_LIST
#undef OP

    default:
        std::cerr << "Opcode: " << static_cast<int>(op)
            << " not in OpcodeString()" << std::endl;
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
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

void push_info(FuncInfoVec *v, Word id, Byte argc, Int addr)
{
    if (v->len >= v->cap) {
        int newcap = v->cap < 8 ? 8 : v->cap * 2;
        // TODO Remove cast
        v->data = (FuncInfo *) realloc(v->data, newcap * sizeof(*v->data));
    }
    FuncInfo *info = &v->data[v->len++];
    info->id = id;
    info->argc = argc;
    info->addr = addr;
}

void assert_range(const FuncInfoVec *v,  Word index)
{
    if (index >= v->len) {
        InternalError(__FILE__, __LINE__,
                "function index out of range: %d, function count: %d\n",
                index, v->len);
        exit(EXIT_FAILURE);
    }
}

template<typename T>
void push_back(std::vector<Byte> &bytes, T operand)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    std::memcpy(buf, &operand, SIZE);

    for (int i = 0; i < SIZE; i++)
        bytes.push_back(buf[i]);
}

template<typename T>
void write(std::vector<Byte> &bytes, Int addr, T operand)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    std::memcpy(buf, &operand, SIZE);

    for (int i = 0; i < SIZE; i++)
        bytes[addr + i] = buf[i];
}

template<typename T>
T read(const std::vector<Byte> &bytes, Int addr)
{
    constexpr int SIZE = sizeof(T);
    Byte buf[SIZE] = {0};

    for (int i = 0; i < SIZE; i++)
        buf[i] = bytes[addr + i];

    T ret {};
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

void LoadByte(Bytecode *code, Byte byte)
{
    code->bytes_.push_back(OP_LOADB);
    code->bytes_.push_back(byte);
}

void LoadInt(Bytecode *code, Int integer)
{
    constexpr Int bytemin = std::numeric_limits<Byte>::min();
    constexpr Int bytemax = std::numeric_limits<Byte>::max();

    if (integer >= bytemin && integer <= bytemax) {
        code->bytes_.push_back(OP_LOADB);
        push_back<Byte>(code->bytes_, integer);
    }
    else {
        code->bytes_.push_back(OP_LOADI);
        push_back<Int>(code->bytes_, integer);
    }
}

void LoadFloat(Bytecode *code, Float fp)
{
    code->bytes_.push_back(OP_LOADF);
    push_back<Float>(code->bytes_, fp);
}

void LoadString(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_LOADS);
    push_back<Word>(code->bytes_, id);
}

void LoadLocal(Bytecode *code, Byte id)
{
    code->bytes_.push_back(OP_LOADLOCAL);
    code->bytes_.push_back(id);
}

void LoadGlobal(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_LOADGLOBAL);
    push_back<Word>(code->bytes_, id);
}

void StoreLocal(Bytecode *code, Byte id)
{
    code->bytes_.push_back(OP_STORELOCAL);
    code->bytes_.push_back(id);
}

void StoreGlobal(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_STOREGLOBAL);
    push_back<Word>(code->bytes_, id);
}

void Load(Bytecode *code)
{
    code->bytes_.push_back(OP_LOAD);
}

void Store(Bytecode *code)
{
    code->bytes_.push_back(OP_STORE);
}

void IncLocal(Bytecode *code, Byte id)
{
    code->bytes_.push_back(OP_INCLOCAL);
    code->bytes_.push_back(id);
}

void IncGlobal(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_INCGLOBAL);
    push_back<Word>(code->bytes_, id);
}

void DecLocal(Bytecode *code, Byte id)
{
    code->bytes_.push_back(OP_DECLOCAL);
    code->bytes_.push_back(id);
}

void DecGlobal(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_DECGLOBAL);
    push_back<Word>(code->bytes_, id);
}

void Allocate(Bytecode *code, Byte count)
{
    if (count == 0)
        return;

    code->bytes_.push_back(OP_ALLOC);
    code->bytes_.push_back(count);
}

void LoadAddress(Bytecode *code, Word id)
{
    code->bytes_.push_back(OP_LOADA);
    push_back<Word>(code->bytes_, id);
}

void Dereference(Bytecode *code)
{
    code->bytes_.push_back(OP_DEREF);
}

void Index(Bytecode *code)
{
    code->bytes_.push_back(OP_INDEX);
}

void LoadTypeNil(Bytecode *code)
{
    code->bytes_.push_back(OP_LOADTYPEN);
}

void LoadTypeBool(Bytecode *code)
{
    code->bytes_.push_back(OP_LOADTYPEB);
}

void LoadTypeInt(Bytecode *code)
{
    code->bytes_.push_back(OP_LOADTYPEI);
}

void LoadTypeFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_LOADTYPEF);
}

void LoadTypeString(Bytecode *code)
{
    code->bytes_.push_back(OP_LOADTYPES);
}

void CallFunction(Bytecode *code, Word func_index, bool builtin)
{
    if (builtin) {
        code->bytes_.push_back(OP_CALL_BUILTIN);
        push_back<Byte>(code->bytes_, func_index);
    }
    else {
        code->bytes_.push_back(OP_CALL);
        push_back<Word>(code->bytes_, func_index);
    }
}

Int JumpIfZero(Bytecode *code, Int addr)
{
    code->bytes_.push_back(OP_JEQ);
    const Int operand_addr = NextAddr(code);
    push_back<Word>(code->bytes_, addr);

    return operand_addr;
}

Int Jump(Bytecode *code, Int addr)
{
    code->bytes_.push_back(OP_JMP);
    const Int operand_addr = NextAddr(code);
    push_back<Word>(code->bytes_, addr);

    return operand_addr;
}

void Return(Bytecode *code)
{
    code->bytes_.push_back(OP_RET);
}

void AddInt(Bytecode *code)
{
    code->bytes_.push_back(OP_ADD);
}

void AddFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_ADDF);
}

void ConcatString(Bytecode *code)
{
    code->bytes_.push_back(OP_CATS);
}

void SubInt(Bytecode *code)
{
    code->bytes_.push_back(OP_SUB);
}

void SubFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_SUBF);
}

void MulInt(Bytecode *code)
{
    code->bytes_.push_back(OP_MUL);
}

void MulFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_MULF);
}

void DivInt(Bytecode *code)
{
    code->bytes_.push_back(OP_DIV);
}

void DivFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_DIVF);
}

void RemInt(Bytecode *code)
{
    code->bytes_.push_back(OP_REM);
}

void RemFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_REMF);
}

void EqualInt(Bytecode *code)
{
    code->bytes_.push_back(OP_EQ);
}

void EqualFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_EQF);
}

void EqualString(Bytecode *code)
{
    code->bytes_.push_back(OP_EQS);
}

void NotEqualInt(Bytecode *code)
{
    code->bytes_.push_back(OP_NEQ);
}

void NotEqualFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_NEQF);
}

void NotEqualString(Bytecode *code)
{
    code->bytes_.push_back(OP_NEQS);
}

void LessInt(Bytecode *code)
{
    code->bytes_.push_back(OP_LT);
}

void LessFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_LTF);
}

void LessEqualInt(Bytecode *code)
{
    code->bytes_.push_back(OP_LTE);
}

void LessEqualFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_LTEF);
}

void GreaterInt(Bytecode *code)
{
    code->bytes_.push_back(OP_GT);
}

void GreaterFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_GTF);
}

void GreaterEqualInt(Bytecode *code)
{
    code->bytes_.push_back(OP_GTE);
}

void GreaterEqualFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_GTEF);
}

void And(Bytecode *code)
{
    code->bytes_.push_back(OP_AND);
}

void Or(Bytecode *code)
{
    code->bytes_.push_back(OP_OR);
}

void Xor(Bytecode *code)
{
    code->bytes_.push_back(OP_XOR);
}

void Not(Bytecode *code)
{
    code->bytes_.push_back(OP_NOT);
}

void ShiftLeft(Bytecode *code)
{
    code->bytes_.push_back(OP_SHL);
}

void ShiftRight(Bytecode *code)
{
    code->bytes_.push_back(OP_SHR);
}

void NegateInt(Bytecode *code)
{
    code->bytes_.push_back(OP_NEG);
}

void NegateFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_NEGF);
}

void SetIfZero(Bytecode *code)
{
    code->bytes_.push_back(OP_SETZ);
}

void SetIfNotZero(Bytecode *code)
{
    code->bytes_.push_back(OP_SETNZ);
}

void Pop(Bytecode *code)
{
    code->bytes_.push_back(OP_POP);
}

void DuplicateTop(Bytecode *code)
{
    code->bytes_.push_back(OP_DUP);
}

void BoolToInt(Bytecode *code)
{
    code->bytes_.push_back(OP_BTOI);
}

void BoolToFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_BTOF);
}

void IntToBool(Bytecode *code)
{
    code->bytes_.push_back(OP_ITOB);
}

void IntToFloat(Bytecode *code)
{
    code->bytes_.push_back(OP_ITOF);
}

void FloatToBool(Bytecode *code)
{
    code->bytes_.push_back(OP_FTOB);
}

void FloatToInt(Bytecode *code)
{
    code->bytes_.push_back(OP_FTOI);
}

void Exit(Bytecode *code)
{
    code->bytes_.push_back(OP_EXIT);
}

void End(Bytecode *code)
{
    code->bytes_.push_back(OP_EOC);
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

void RegisterFunction(Bytecode *code, Word func_index, Byte argc)
{
    const Word next_index = code->funcs_.len;

    if (func_index != next_index) {
        InternalError(__FILE__, __LINE__,
                "function func_index %d and next index %d should match\n",
                func_index, next_index);
        exit(EXIT_FAILURE);
    }

    const Int next_addr = NextAddr(code);
    push_info(&code->funcs_, func_index, argc, next_addr);
}

Int RegisterConstString(Bytecode *code, std::string_view str)
{
    const Word next_index = code->strings_.size();

    code->strings_.emplace_back(str.data(), str.length());

    return next_index;
}

const std::string &GetConstString(const Bytecode *code, Word str_index)
{
    if (str_index < 0 || str_index >= code->strings_.size()) {
        InternalError(__FILE__, __LINE__,
                "index out of range: %d", str_index);
    }

    return code->strings_[str_index];
}

Byte Read(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size(code));

    return code->bytes_[addr];
}

Word ReadWord(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size(code));

    return read<Word>(code->bytes_, addr);
}

Int ReadInt(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size(code));

    return read<Int>(code->bytes_, addr);
}

Float ReadFloat(const Bytecode *code, Int addr)
{
    if (addr < 0 || addr >= Size(code))
        InternalError(__FILE__, __LINE__,
                "address out of range: %d", Size(code));

    return read<Float>(code->bytes_, addr);
}

Int NextAddr(const Bytecode *code)
{
    return Size(code);
}

Int Size(const Bytecode *code)
{
    return code->bytes_.size();
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
    write<Word>(code->bytes_, operand_addr, next_addr);
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

enum OperandSize {
    OPERAND_NONE,
    OPERAND_BYTE,
    OPERAND_WORD,
    OPERAND_QUAD,
};

static Int print_op(const Bytecode *code, int op, int operand, Int address)
{
    const Int addr = address;
    Int inc = 0;
    std::string text = OpcodeString(op);

    // remove prefix "OP_"
    text = text.substr(text.find('_') + 1, std::string::npos);

    // padding spaces
    if (operand != OPERAND_NONE)
        text.append(12 - text.length(), ' ');

    // append operand
    switch (operand) {

    case OPERAND_BYTE:
        text += " $" + std::to_string(static_cast<int>(Read(code, addr)));
        inc = 1;
        break;

    case OPERAND_WORD:
        text += " $" + std::to_string(ReadWord(code, addr));
        inc = sizeof(Word);
        break;

    case OPERAND_QUAD:
        text += " $" + std::to_string(ReadInt(code, addr));
        inc = sizeof(Int);
        break;
    }

    // add extra info
    switch (op) {
    case OP_LOADF:
        text += " = " + std::to_string(ReadFloat(code, addr));
        break;

    case OP_LOADS:
        text += " = \"" + GetConstString(code, ReadWord(code, addr)) + "\"";
        break;

    case OP_CALL:
        // TODO function id could be retrived if we have OP_CALL_STATIC
        // to call functions that are defined statically
        //text += " = @" + std::to_string(GetFunctionAddress(ReadWord(addr)));
        break;

    case OP_LOADLOCAL: case OP_LOADGLOBAL:
    case OP_STORELOCAL: case OP_STOREGLOBAL:
        {
            const std::size_t found = text.find('$');
            text[found] = '@';
        }
        break;
    }

    // output
    std::cout << text << std::endl;
    return addr + inc;
}

void PrintBytecode(const Bytecode *code)
{
    // function info
    for (int i = 0; i < code->funcs_.len; i++) {
        const FuncInfo *info = &code->funcs_.data[i];
        printf("* function id: %d @%lld\n", info->id, info->addr);
    }

    Int addr = 0;

    while (addr < Size(code)) {
        std::cout << "[" << std::setw(6) << addr << "] ";

        const int op = Read(code, addr++);

        switch (op) {

#define OP(opcode, operand_size) \
            case opcode: addr = print_op(code, op, operand_size, addr); break;
        BYTECODE_LIST
#undef OP

        default:
            std::cerr << "Opcode: " << " not in Bytecode::Print()" << std::endl;
            std::exit(EXIT_FAILURE);
            break;
        }

        if (op == OP_EOC)
            break;
    }
}
