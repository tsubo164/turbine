#include "bytecode.h"
#include "error.h"
#include <iostream>
#include <iomanip>
#include <cstring>

const char *OpcodeString(Byte op)
{
#define O(op) case op: return #op;
    switch (op) {
    O(OP_NOP);

    O(OP_LOADB);
    O(OP_LOADI);
    O(OP_LOADF);
    O(OP_LOADS);
    O(OP_LOADLOCAL);
    O(OP_LOADGLOBAL);
    O(OP_STORELOCAL);
    O(OP_STOREGLOBAL);

    O(OP_ALLOC);
    O(OP_CALL);
    O(OP_CALL_BUILTIN);
    O(OP_RET);
    O(OP_JMP);
    O(OP_JEQ);

    O(OP_ADD);
    O(OP_ADDF);
    O(OP_ADDS);
    O(OP_EQ);
    O(OP_EQF);
    O(OP_EQS);

    O(OP_EXIT);
    O(OP_EOC);
    default:
        std::cerr << "Opcode: " << static_cast<int>(op)
            << " not in OpcodeString()" << std::endl;
        std::exit(EXIT_FAILURE);
        return nullptr;
    }
#undef O
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
    const Int operand_addr = Size();
    push_back<Word>(bytes_, addr);

    return operand_addr;
}

Int Bytecode::Jump(Int addr)
{
    bytes_.push_back(OP_JMP);
    const Int operand_addr = Size();
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
    const Int current_addr = Size();
    write<Word>(bytes_, operand_addr, current_addr);
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

    const Int next_addr = Size();
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

Int Bytecode::Size() const
{
    return bytes_.size();
}

static void print_op(int op)
{
    std::cout << OpcodeString(op) << std::endl;
}

static void print_op_immediate(int op, Int immediate, bool end_line = true)
{
    std::cout << OpcodeString(op) << " $" << immediate;

    if (end_line)
        std::cout << std::endl;
}

static void print_op_immediate_f(int op, Float immediate, bool end_line = true)
{
    std::cout << OpcodeString(op) << " $" << immediate;

    if (end_line)
        std::cout << std::endl;
}

static void print_op_immediate_s(int op, Word immediate, const std::string s,
        bool end_line = true)
{
    std::cout << OpcodeString(op) <<
        " $" << immediate <<
        " = @\"" << s << "\"";

    if (end_line)
        std::cout << std::endl;
}

static void print_op_address(int op, Int address)
{
    std::cout << OpcodeString(op) << " @" << address << std::endl;
}

void Bytecode::Print() const
{
    // function info
    for (const auto &func: funcs_)
        std::cout << "* function id: " << func.id << " @" << func.addr << std::endl;

    bool brk = false;
    int addr = 0;

    while (addr < Size() && !brk) {
        std::cout << "[" << std::setw(6) << addr << "] ";

        const int op = Read(addr++);

        switch (op) {
        case OP_NOP:
            std::cout << OpcodeString(op) << std::endl;
            break;

        case OP_LOADB:
            print_op_immediate(op, Read(addr++));
            break;

        case OP_LOADI:
            print_op_immediate(op, ReadInt(addr));
            addr += 8;
            break;

        case OP_LOADF:
            print_op_immediate_f(op, ReadFloat(addr));
            addr += 8;
            break;

        case OP_LOADS:
            {
                const int index = ReadWord(addr);
                if (index < 0 || index >= strings_.size())
                    InternalError("index out of range: " +
                            std::to_string(index),
                            __FILE__, __LINE__);

                print_op_immediate_s(op, ReadWord(addr), strings_[index]);
                addr += 2;
            }
            break;

        case OP_LOADLOCAL:
            print_op_address(op, Read(addr++));
            break;

        case OP_LOADGLOBAL:
            print_op_address(op, ReadWord(addr));
            addr += 2;
            break;

        case OP_STORELOCAL:
            print_op_address(op, Read(addr++));
            break;

        case OP_STOREGLOBAL:
            print_op_address(op, ReadWord(addr));
            addr += 2;
            break;

        case OP_ALLOC:
            print_op_immediate(op, Read(addr++));
            break;

        case OP_CALL:
            print_op_immediate(op, ReadWord(addr), false);
            std::cout << " = @" << GetFunctionAddress(ReadWord(addr)) << std::endl;
            addr += 2;
            break;

        case OP_CALL_BUILTIN:
            print_op_immediate(op, Read(addr++));
            break;

        case OP_RET:
            print_op(op);
            break;

        case OP_JMP:
            print_op_immediate(op, ReadWord(addr));
            addr += 2;
            break;

        case OP_JEQ:
            print_op_immediate(op, ReadWord(addr));
            addr += 2;
            break;

        case OP_ADD:
            print_op(op);
            break;

        case OP_ADDF:
            print_op(op);
            break;

        case OP_ADDS:
            print_op(op);
            break;

        case OP_EQ:
            print_op(op);
            break;

        case OP_EQF:
            print_op(op);
            break;

        case OP_EQS:
            print_op(op);
            break;

        case OP_EXIT:
            print_op(op);
            break;

        case OP_EOC:
            print_op(op);
            brk = true;
            break;

        default:
            std::cerr << "Opcode: " << " not in Bytecode::Print()" << std::endl;
            std::exit(EXIT_FAILURE);
            break;
        }
    }
}
