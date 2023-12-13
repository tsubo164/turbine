#include "vm.h"
#include <iostream>
#include <cmath>

void VM::set_ip(Int ip)
{
    ip_ = ip;
}

void VM::set_sp(Int sp)
{
    if (sp >= stack_.size())
        stack_.resize( sp + 1);

    sp_ = sp;
}

void VM::set_bp(Int bp)
{
    bp_ = bp;
}

Int VM::read_byte(Int index) const
{
    return code_->Read(index);
}

Int VM::fetch_byte()
{
    return read_byte(ip_++);
}

Int VM::fetch_word()
{
    constexpr int SIZE = sizeof(Word);
    Byte buf[SIZE] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = static_cast<Byte>(fetch_byte());

    Word ret = 0;
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

Int VM::fetch_int()
{
    constexpr int SIZE = sizeof(Int);
    Byte buf[SIZE] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = static_cast<Byte>(fetch_byte());

    Int ret = 0;
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

Float VM::fetch_float()
{
    constexpr int SIZE = sizeof(Float);
    Byte buf[SIZE] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = static_cast<Byte>(fetch_byte());

    Float ret = 0;
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

Word VM::fetch_str()
{
    constexpr int SIZE = sizeof(Word);
    Byte buf[SIZE] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = static_cast<Byte>(fetch_byte());

    Word ret = 0;
    std::memcpy(&ret, buf, SIZE);

    return ret;
}

void VM::push(Value val)
{
    if (sp_ == stack_.size() - 1) {
        stack_.push_back(val);
        sp_++;
    }
    else {
        stack_[++sp_] = val;
    }
}

Value VM::pop()
{
    return stack_[sp_--];
}

Value VM::top() const
{
    return stack_[sp_];
}

Int VM::pop_int()
{
    const Value val = pop();
    return val.inum;
}

Float VM::pop_float()
{
    const Value val = pop();
    return val.fpnum;
}

void VM::push_int(Int inum)
{
    Value val;
    val.inum = inum;
    push(val);
}

void VM::push_float(Float fpnum)
{
    Value val;
    val.fpnum = fpnum;
    push(val);
}

void VM::push_call(Call call)
{
    if (call_sp_ == callstack_.size() - 1) {
        callstack_.push_back(call);
        call_sp_++;
    }
    else {
        callstack_[++call_sp_] = call;
    }
}

Call VM::pop_call()
{
    return callstack_[call_sp_--];
}

Value VM::get_local(int id) const
{
    return stack_[bp_ + 1 + id];
}

Value VM::get_global(int id) const
{
    return stack_[1 + id];
}

void VM::set_local(int id, Value val)
{
    stack_[bp_ + 1 + id] = val;
}

void VM::set_global(int id, Value val)
{
    stack_[1 + id] = val;
}

bool VM::is_eoc() const
{
    return ip_ == eoc_;
}

void VM::Run(const Bytecode &code)
{
    code_ = &code;
    eoc_ = code.Size();
    run();
}

void VM::run()
{
    bool brk = false;

    while (!is_eoc() && !brk) {
        const Int old_ip = ip_;
        const Int op = fetch_byte();

        if (print_stack_) {
            printf("[%6lld] %s\n", old_ip, OpcodeString(op));
            PrintStack();
        }

        switch (op) {

        case OP_LOADB:
            {
                Value val;
                val.inum = fetch_byte();
                push(val);
            }
            break;

        case OP_LOADI:
            {
                Value val;
                val.inum = fetch_int();
                push(val);
            }
            break;

        case OP_LOADF:
            {
                Value val;
                val.fpnum = fetch_float();
                push(val);
            }
            break;

        case OP_LOADS:
            {
                const Word id = fetch_str();
                const std::string &s = code_->GetConstString(id);
                Value val;
                val.str = new std::string(s);
                push(val);
            }
            break;

        case OP_LOADLOCAL:
            {
                const Int id = fetch_byte();
                const Value val = get_local(id);
                push(val);
            }
            break;

        case OP_LOADGLOBAL:
            {
                const Int id = fetch_word();
                const Value val = get_global(id);
                push(val);
            }
            break;

        case OP_STORELOCAL:
            {
                const Int id = fetch_byte();
                const Value val = pop();
                set_local(id, val);
            }
            break;

        case OP_STOREGLOBAL:
            {
                const Int id = fetch_word();
                const Value val = pop();
                set_global(id, val);
            }
            break;

        case OP_INCLOCAL:
            {
                const Int id = fetch_byte();
                Value val = get_local(id);
                val.inum++;
                set_local(id, val);
            }
            break;

        case OP_INCGLOBAL:
            {
                const Int id = fetch_word();
                Value val = get_global(id);
                val.inum++;
                set_global(id, val);
            }
            break;

        case OP_ALLOC:
            {
                const Int size  = fetch_byte();
                set_sp(sp_ + size);
            }
            break;

        case OP_CALL:
            {
                const Word func_index = fetch_word();
                const Int func_addr = code_->GetFunctionAddress(func_index);

                Call call;
                call.argc = code_->GetFunctionArgCount(func_index);
                call.return_ip = ip_;
                call.return_bp = bp_;
                push_call(call);

                set_ip(func_addr);
                set_bp(sp_ - call.argc);
            }
            break;

        case OP_CALL_BUILTIN:
            {
                const Byte func_index = fetch_byte();

                if (func_index == 0) {
                    // builtin "print" function
                    const Value val = pop();
                    std::cout << *val.str << std::endl;
                }
                else if (func_index == 1) {
                    // builtin "exit" function
                    brk = true;
                }
            }
            break;

        case OP_RET:
            {
                const Value ret_obj = top();
                const Call call = pop_call();

                set_ip(call.return_ip);
                set_sp(bp_);
                set_bp(call.return_bp);
                push(ret_obj);
            }
            break;

        case OP_JMP:
            {
                const Int addr = fetch_word();
                set_ip(addr);
            }
            break;

        case OP_JEQ:
            {
                const Int addr = fetch_word();
                const Value cond = pop();

                if (cond.inum == 0)
                    set_ip(addr);
            }
            break;

        case OP_ADD:
            {
                const Int val1 = pop_int();
                const Int val0 = pop_int();
                push_int(val0 + val1);
            }
            break;

        case OP_ADDF:
            {
                const Float val1 = pop_float();
                const Float val0 = pop_float();
                push_float(val0 + val1);
            }
            break;

        case OP_ADDS:
            {
                const Value val1 = pop();
                const Value val0 = pop();
                Value val;
                val.str = new std::string(*val0.str + *val1.str);
                push(val);
            }
            break;

        case OP_SUB:
            {
                const Int val1 = pop_int();
                const Int val0 = pop_int();
                push_int(val0 - val1);
            }
            break;

        case OP_SUBF:
            {
                const Float val1 = pop_float();
                const Float val0 = pop_float();
                push_float(val0 - val1);
            }
            break;

        case OP_MUL:
            {
                const Int r = pop_int();
                const Int l = pop_int();
                push_int(l * r);
            }
            break;

        case OP_MULF:
            {
                const Float val1 = pop_float();
                const Float val0 = pop_float();
                push_float(val0 * val1);
            }
            break;

        case OP_DIV:
            {
                const Int r = pop_int();
                const Int l = pop_int();
                // TODO check zero div
                push_int(l / r);
            }
            break;

        case OP_DIVF:
            {
                const Float r = pop_float();
                const Float l = pop_float();
                // TODO check zero div
                push_float(l / r);
            }
            break;

        case OP_REM:
            {
                const Int r = pop_int();
                const Int l = pop_int();
                // TODO check zero div
                push_int(l % r);
            }
            break;

        case OP_REMF:
            {
                const Float r = pop_float();
                const Float l = pop_float();
                // TODO check zero div
                push_float(std::fmod(l, r));
            }
            break;

        case OP_EQ:
            {
                const Int val1 = pop_int();
                const Int val0 = pop_int();
                push_int(val0 == val1);
            }
            break;

        case OP_EQF:
            {
                const Float val1 = pop_float();
                const Float val0 = pop_float();
                // TODO do fpnum comp
                push_int(val0 == val1);
            }
            break;

        case OP_EQS:
            {
                const Value val1 = pop();
                const Value val0 = pop();
                Value val;
                val.inum = *val0.str == *val1.str;
                push(val);
            }
            break;

        case OP_NEQ:
            {
                const Int val1 = pop_int();
                const Int val0 = pop_int();
                push_int(val0 != val1);
            }
            break;

        case OP_NEG:
            {
                const Int r = pop_int();
                push_int( -1 * r);
            }
            break;

        case OP_NEGF:
            {
                const Float r = pop_float();
                push_float( -1 * r);
            }
            break;

        case OP_EXIT:
        case OP_EOC:
            brk = true;
            break;

        case OP_NOP:
            break;

        default:
            std::cerr << "Opcode: '" << OpcodeString(op) <<
                "' not in VM::run()" << std::endl;
            std::exit(EXIT_FAILURE);
            break;
        }
    }
}

Int VM::StackTopInt() const
{
    const Value val = top();
    return val.inum;
}

void VM::PrintStack() const
{
    printf( "    ------\n" );
    for ( Int i = sp_; i >= 0; i-- )
    {
        unsigned int index = static_cast< unsigned int >( i );
        if ( index == sp_ )
        {
            printf( "SP->" );
        }
        else
        {
            printf( "    " );
        }

        printf( "|%4llu|", stack_[index].inum );

        if ( index == bp_ )
        {
            printf( "<-BP" );
        }
        printf( "\n" );
    }
    printf( "--------------\n" );
}

void VM::EnablePrintStack(bool enable)
{
    print_stack_ = enable;
}
