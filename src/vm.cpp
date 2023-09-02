#include "vm.h"

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

void VM::push(Object obj)
{
    if (sp_ == stack_.size() - 1) {
        stack_.push_back(obj);
        sp_++;
    }
    else {
        stack_[++sp_] = obj;
    }
}

Object VM::pop()
{
    return stack_[sp_--];
}

Object VM::top() const
{
    return stack_[sp_];
}

Int VM::pop_int()
{
    const Object obj = pop();
    return obj.ival;
}

void VM::push_int(Int val)
{
    Object obj;
    obj.ival = val;
    push(obj);
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

Object VM::get_local(int id) const
{
    return stack_[bp_ + 1 + id];
}

Object VM::get_arg(int id) const
{
    return stack_[bp_ - 2 - id];
}

void VM::set_local(int id, Object obj)
{
    stack_[bp_ + 1 + id] = obj;
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
                Object obj;
                obj.ival = fetch_byte();
                push(obj);
            }
            break;

        case OP_LOADI:
            {
                Object obj;
                obj.ival = fetch_int();
                push(obj);
            }
            break;

        case OP_LOADLOCAL:
            {
                const Int id = fetch_byte();
                const Object obj = get_local(id);
                push(obj);
            }
            break;

        case OP_LOADARG:
            {
                const Int id = fetch_byte();
                const Object obj = get_arg(id);
                push(obj);
            }
            break;

        case OP_STORELOCAL:
            {
                const Int id = fetch_byte();
                const Object obj = pop();
                set_local(id, obj);
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

        case OP_RET:
            {
                const Int argc = fetch_byte();
                const Object ret_obj = top();

                const Call call = pop_call();
                set_ip(call.return_ip);
                set_sp(bp_);
                set_bp(call.return_bp);
                push(ret_obj);
            }
            break;

        case OP_JEQ:
            {
                const Int addr = fetch_word();
                const Object cond = top();

                if (cond.ival == 0)
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

        case OP_EQ:
            {
                const Int val1 = pop_int();
                const Int val0 = pop_int();
                push_int(val0 == val1);
            }
            break;

        case OP_EXIT:
        case OP_EOC:
            brk = true;
            break;

        case OP_NOP:
        default:
            break;
        }
    }
}

Int VM::StackTopInt() const
{
    const Object obj = top();
    return obj.ival;
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

        printf( "|%4llu|", stack_[index].ival );

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
