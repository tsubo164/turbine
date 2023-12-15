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

void Bytecode::LoadByte(Byte byte)
{
    bytes_.push_back(OP_LOADB);
    bytes_.push_back(byte);
}

void Bytecode::LoadInt(Int integer)
{
    bytes_.push_back(OP_LOADI);
    push_back<Int>(bytes_, integer);
}

void Bytecode::LoadFloat(Float fp)
{
    bytes_.push_back(OP_LOADF);
    push_back<Float>(bytes_, fp);
}

void Bytecode::LoadString(Word id)
{
    bytes_.push_back(OP_LOADS);
    push_back<Word>(bytes_, id);
}

void Bytecode::LoadLocal(Byte id)
{
    bytes_.push_back(OP_LOADLOCAL);
    bytes_.push_back(id);
}

void Bytecode::LoadGlobal(Word id)
{
    bytes_.push_back(OP_LOADGLOBAL);
    push_back<Word>(bytes_, id);
}

void Bytecode::StoreLocal(Byte id)
{
    bytes_.push_back(OP_STORELOCAL);
    bytes_.push_back(id);
}

void Bytecode::StoreGlobal(Word id)
{
    bytes_.push_back(OP_STOREGLOBAL);
    push_back<Word>(bytes_, id);
}

void Bytecode::IncLocal(Byte id)
{
    bytes_.push_back(OP_INCLOCAL);
    bytes_.push_back(id);
}

void Bytecode::IncGlobal(Word id)
{
    bytes_.push_back(OP_INCGLOBAL);
    push_back<Word>(bytes_, id);
}

void Bytecode::DecLocal(Byte id)
{
    bytes_.push_back(OP_DECLOCAL);
    bytes_.push_back(id);
}

void Bytecode::DecGlobal(Word id)
{
    bytes_.push_back(OP_DECGLOBAL);
    push_back<Word>(bytes_, id);
}

void Bytecode::AllocateLocal(Byte count)
{
    bytes_.push_back(OP_ALLOC);
    bytes_.push_back(count);
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
    const Int operand_addr = NextAddr();
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

Int Bytecode::Jump(Int addr)
{
    bytes_.push_back(OP_JMP);
    const Int operand_addr = NextAddr();
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

void Bytecode::AddString()
{
    bytes_.push_back(OP_ADDS);
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

void Bytecode::Exit()
{
    bytes_.push_back(OP_EXIT);
}

void Bytecode::End()
{
    bytes_.push_back(OP_EOC);
}

void Bytecode::BackPatch(Int operand_addr)
{
    const Int next_addr = NextAddr();
    write<Word>(bytes_, operand_addr, next_addr);
}

Int Bytecode::GetFunctionAddress(Word func_index) const
{
    if (func_index >= funcs_.size()) {
        std::cerr << "internal error: function index out of range: "
            << func_index << ", function count: " << funcs_.size() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return funcs_[func_index].addr;
}

Int Bytecode::GetFunctionArgCount(Word func_index) const
{
    if (func_index >= funcs_.size()) {
        std::cerr << "internal error: function index out of range: "
            << func_index << ", function count: " << funcs_.size() << std::endl;
        std::exit(EXIT_FAILURE);
    }

    return funcs_[func_index].argc;
}

void Bytecode::RegisterFunction(Word func_index, Byte argc)
{
    const Word next_index = funcs_.size();

    if (func_index != next_index) {
        std::cerr << "error: "
            << "function func_index " << func_index
            << " and next index " << next_index
            << " should match" << std::endl;
        std::exit(EXIT_FAILURE);
    }

    const Int next_addr = NextAddr();
    funcs_.emplace_back(func_index, argc, next_addr);
}

Int Bytecode::RegisterConstString(std::string_view str)
{
    const Word next_index = strings_.size();

    strings_.emplace_back(str.data(), str.length());

    return next_index;
}

const std::string &Bytecode::GetConstString(Word str_index) const
{
    if (str_index < 0 || str_index >= strings_.size()) {
        InternalError("index out of range: " + std::to_string(str_index),
                __FILE__, __LINE__);
    }

    return strings_[str_index];
}

Byte Bytecode::Read(Int addr) const
{
    if (addr < 0 || addr >= Size())
        InternalError("address out of range: " + std::to_string(Size()),
                __FILE__, __LINE__);

    return bytes_[addr];
}

Word Bytecode::ReadWord(Int addr) const
{
    if (addr < 0 || addr >= Size())
        InternalError("address out of range: " + std::to_string(Size()),
                __FILE__, __LINE__);

    return read<Word>(bytes_, addr);
}

Int Bytecode::ReadInt(Int addr) const
{
    if (addr < 0 || addr >= Size())
        InternalError("address out of range: " + std::to_string(Size()),
                __FILE__, __LINE__);

    return read<Int>(bytes_, addr);
}

Float Bytecode::ReadFloat(Int addr) const
{
    if (addr < 0 || addr >= Size())
        InternalError("address out of range: " + std::to_string(Size()),
                __FILE__, __LINE__);

    return read<Float>(bytes_, addr);
}

Int Bytecode::NextAddr() const
{
    return Size();
}

Int Bytecode::Size() const
{
    return bytes_.size();
}

void Bytecode::SwapBlockEnds(std::vector<Int> &ends)
{
    block_ends_.swap(ends);
}

void Bytecode::BackPatchEnds()
{
    for (auto addr: block_ends_)
        BackPatch(addr);
}

void Bytecode::PushBackPatchEnd(Int addr)
{
    block_ends_.push_back(addr);
}

enum OperandSize {
    OPERAND_NONE,
    OPERAND_BYTE,
    OPERAND_WORD,
    OPERAND_QUAD,
};

Int Bytecode::print_op(int op, int operand, Int address) const
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
        text += " $" + std::to_string(static_cast<int>(Read(addr)));
        inc = 1;
        break;

    case OPERAND_WORD:
        text += " $" + std::to_string(ReadWord(addr));
        inc = sizeof(Word);
        break;

    case OPERAND_QUAD:
        text += " $" + std::to_string(ReadInt(addr));
        inc = sizeof(Int);
        break;
    }

    // add extra info
    switch (op) {
    case OP_LOADF:
        text += " = " + std::to_string(ReadFloat(addr));
        break;

    case OP_LOADS:
        text += " = \"" + GetConstString(ReadWord(addr)) + "\"";
        break;

    case OP_CALL:
        text += " = @" + std::to_string(GetFunctionAddress(ReadWord(addr)));
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

void Bytecode::Print() const
{
    // function info
    for (const auto &func: funcs_)
        std::cout << "* function id: " << func.id << " @" << func.addr << std::endl;

    Int addr = 0;

    while (addr < Size()) {
        std::cout << "[" << std::setw(6) << addr << "] ";

        const int op = Read(addr++);

        switch (op) {

#define OP(opcode, operand_size) \
            case opcode: addr = print_op(op, operand_size, addr); break;
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
