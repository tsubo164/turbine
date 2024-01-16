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

void Bytecode::LoadAddress(Word id)
{
    bytes_.push_back(OP_LOADA);
    push_back<Word>(bytes_, id);
}

void Bytecode::Dereference()
{
    bytes_.push_back(OP_DEREF);
}

void Bytecode::Index()
{
    bytes_.push_back(OP_INDEX);
}

void Bytecode::LoadTypeNil()
{
    bytes_.push_back(OP_LOADTYPEN);
}

void Bytecode::LoadTypeBool()
{
    bytes_.push_back(OP_LOADTYPEB);
}

void Bytecode::LoadTypeInt()
{
    bytes_.push_back(OP_LOADTYPEI);
}

void Bytecode::LoadTypeFloat()
{
    bytes_.push_back(OP_LOADTYPEF);
}

void Bytecode::LoadTypeString()
{
    bytes_.push_back(OP_LOADTYPES);
}

void Bytecode::CallFunction(Word func_index, bool builtin)
{
    if (builtin) {
        bytes_.push_back(OP_CALL_BUILTIN);
        push_back<Byte>(bytes_, func_index);
    }
    else {
        bytes_.push_back(OP_CALL);
        push_back<Word>(bytes_, func_index);
    }
}

Int Bytecode::JumpIfZero(Int addr)
{
    bytes_.push_back(OP_JEQ);
    const Int operand_addr = NextAddr(this);
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

Int Bytecode::Jump(Int addr)
{
    bytes_.push_back(OP_JMP);
    const Int operand_addr = NextAddr(this);
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

void Bytecode::Return()
{
    bytes_.push_back(OP_RET);
}

void Bytecode::AddInt()
{
    bytes_.push_back(OP_ADD);
}

void Bytecode::AddFloat()
{
    bytes_.push_back(OP_ADDF);
}

void Bytecode::ConcatString()
{
    bytes_.push_back(OP_CATS);
}

void Bytecode::SubInt()
{
    bytes_.push_back(OP_SUB);
}

void Bytecode::SubFloat()
{
    bytes_.push_back(OP_SUBF);
}

void Bytecode::MulInt()
{
    bytes_.push_back(OP_MUL);
}

void Bytecode::MulFloat()
{
    bytes_.push_back(OP_MULF);
}

void Bytecode::DivInt()
{
    bytes_.push_back(OP_DIV);
}

void Bytecode::DivFloat()
{
    bytes_.push_back(OP_DIVF);
}

void Bytecode::RemInt()
{
    bytes_.push_back(OP_REM);
}

void Bytecode::RemFloat()
{
    bytes_.push_back(OP_REMF);
}

void Bytecode::EqualInt()
{
    bytes_.push_back(OP_EQ);
}

void Bytecode::EqualFloat()
{
    bytes_.push_back(OP_EQF);
}

void Bytecode::EqualString()
{
    bytes_.push_back(OP_EQS);
}

void Bytecode::NotEqualInt()
{
    bytes_.push_back(OP_NEQ);
}

void Bytecode::NotEqualFloat()
{
    bytes_.push_back(OP_NEQF);
}

void Bytecode::NotEqualString()
{
    bytes_.push_back(OP_NEQS);
}

void Bytecode::LessInt()
{
    bytes_.push_back(OP_LT);
}

void Bytecode::LessFloat()
{
    bytes_.push_back(OP_LTF);
}

void Bytecode::LessEqualInt()
{
    bytes_.push_back(OP_LTE);
}

void Bytecode::LessEqualFloat()
{
    bytes_.push_back(OP_LTEF);
}

void Bytecode::GreaterInt()
{
    bytes_.push_back(OP_GT);
}

void Bytecode::GreaterFloat()
{
    bytes_.push_back(OP_GTF);
}

void Bytecode::GreaterEqualInt()
{
    bytes_.push_back(OP_GTE);
}

void Bytecode::GreaterEqualFloat()
{
    bytes_.push_back(OP_GTEF);
}

void Bytecode::And()
{
    bytes_.push_back(OP_AND);
}

void Bytecode::Or()
{
    bytes_.push_back(OP_OR);
}

void Bytecode::Xor()
{
    bytes_.push_back(OP_XOR);
}

void Bytecode::Not()
{
    bytes_.push_back(OP_NOT);
}

void Bytecode::ShiftLeft()
{
    bytes_.push_back(OP_SHL);
}

void Bytecode::ShiftRight()
{
    bytes_.push_back(OP_SHR);
}

void Bytecode::NegateInt()
{
    bytes_.push_back(OP_NEG);
}

void Bytecode::NegateFloat()
{
    bytes_.push_back(OP_NEGF);
}

void Bytecode::SetIfZero()
{
    bytes_.push_back(OP_SETZ);
}

void Bytecode::SetIfNotZero()
{
    bytes_.push_back(OP_SETNZ);
}

void Bytecode::Pop()
{
    bytes_.push_back(OP_POP);
}

void Bytecode::DuplicateTop()
{
    bytes_.push_back(OP_DUP);
}

void Bytecode::BoolToInt()
{
    bytes_.push_back(OP_BTOI);
}

void Bytecode::BoolToFloat()
{
    bytes_.push_back(OP_BTOF);
}

void Bytecode::IntToBool()
{
    bytes_.push_back(OP_ITOB);
}

void Bytecode::IntToFloat()
{
    bytes_.push_back(OP_ITOF);
}

void Bytecode::FloatToBool()
{
    bytes_.push_back(OP_FTOB);
}

void Bytecode::FloatToInt()
{
    bytes_.push_back(OP_FTOI);
}

void Bytecode::Exit()
{
    bytes_.push_back(OP_EXIT);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);
}

Int GetFunctionAddress(const Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.size()) {
        std::cerr << "internal error: function index out of range: "
            << func_index << ", function count: " << code->funcs_.size() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return code->funcs_[func_index].addr;
}

Int GetFunctionArgCount(const Bytecode *code, Word func_index)
{
    if (func_index >= code->funcs_.size()) {
        std::cerr << "internal error: function index out of range: "
            << func_index << ", function count: " << code->funcs_.size() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return code->funcs_[func_index].argc;
}

void RegisterFunction(Bytecode *code, Word func_index, Byte argc)
{
    const Word next_index = code->funcs_.size();

    if (func_index != next_index) {
        std::cerr << "error: "
            << "function func_index " << func_index
            << " and next index " << next_index
            << " should match" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const Int next_addr = NextAddr(code);
    code->funcs_.emplace_back(func_index, argc, next_addr);
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
    code->ors_.push(-1);
}

void BeginFor(Bytecode *code)
{
    code->breaks_.push(-1);
    code->continues_.push(-1);
}

void BeginSwitch(Bytecode *code)
{
    code->casecloses_.push(-1);
}

void PushOrClose(Bytecode *code, Int addr)
{
    code->ors_.push(addr);
}

void PushBreak(Bytecode *code, Int addr)
{
    code->breaks_.push(addr);
}

void PushContinue(Bytecode *code, Int addr)
{
    code->continues_.push(addr);
}

void PushCaseClose(Bytecode *code, Int addr)
{
    code->casecloses_.push(addr);
}

void BackPatch(Bytecode *code, Int operand_addr)
{
    const Int next_addr = NextAddr(code);
    write<Word>(code->bytes_, operand_addr, next_addr);
}

void BackPatchOrCloses(Bytecode *code)
{
    while (!code->ors_.empty()) {
        const Int addr = code->ors_.top();
        code->ors_.pop();
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchBreaks(Bytecode *code)
{
    while (!code->breaks_.empty()) {
        const Int addr = code->breaks_.top();
        code->breaks_.pop();
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchContinues(Bytecode *code)
{
    while (!code->continues_.empty()) {
        const Int addr = code->continues_.top();
        code->continues_.pop();
        if (addr == -1)
            break;
        BackPatch(code, addr);
    }
}

void BackPatchCaseCloses(Bytecode *code)
{
    while (!code->casecloses_.empty()) {
        const Int addr = code->casecloses_.top();
        code->casecloses_.pop();
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
    for (const auto &func: code->funcs_)
        std::cout << "* function id: " << func.id << " @" << func.addr << std::endl;

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
