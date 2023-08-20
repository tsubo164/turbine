#include "vm.h"

VM::VM()
{
}

VM::~VM()
{
}

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
    return code_[index];
}

Int VM::fetch_byte()
{
    return read_byte(ip_++);
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

Object VM::get_local(int id) const
{
    return stack_[bp_ + 1 + id];
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
    code_ = code.Data();
    eoc_ = code.Size();
    run();
}

void VM::run()
{
    bool brk = false;

    while (!is_eoc() && !brk) {
        const Int op = fetch_byte();

        if (print_stack_) {
            printf("%s\n", OpcodeString(op));
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
