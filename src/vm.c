#include "vm.h"
#include "error.h"
#include "objarray.h"

#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <math.h>

// for variadic function
enum TypeID {
    TID_NIL = 0,
    TID_BOL,
    TID_INT,
    TID_FLT,
    TID_STR,
};

static int new_cap(int cur_cap, int min_cap)
{
    return cur_cap < min_cap ? min_cap : cur_cap * 2;
}

static void resize_stack(ValueVec *v, int new_len)
{
    if (new_len >= v->cap) {
        v->cap = v->cap < 256 ? 256 : v->cap;
        while (v->cap < new_len)
            v->cap *= 2;
        // TODO Remove cast
        v->data = (Value *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->len = new_len;
}

static void push_value(ValueVec *v, Value val)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 256);
        // TODO Remove cast
        v->data = (Value *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = val;
}

static void push_callinfo(CallVec *v, const Call *call)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 32);
        // TODO Remove cast
        v->data = (Call *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->data[v->len++] = *call;
}

static void set_ip(VM *vm, Int ip)
{
    vm->ip_ = ip;
}

static void set_sp(VM *vm, Int sp)
{
    if (sp >= vm->stack_.len)
        resize_stack(&vm->stack_, sp + 1);

    vm->sp_ = sp;
}

static void set_bp(VM *vm, Int bp)
{
    vm->bp_ = bp;
}

static Int read_byte(const VM *vm, Int index)
{
    return Read(vm->code_, index);
}

static Int fetch_byte(VM *vm)
{
    return read_byte(vm, vm->ip_++);
}

static Int fetch_word(VM *vm)
{
    const int SIZE = sizeof(Word);
    Byte buf[sizeof(Word)] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = (Byte)fetch_byte(vm);

    Word ret = 0;
    memcpy(&ret, buf, SIZE);

    return ret;
}

static Int fetch_int(VM *vm)
{
    const int SIZE = sizeof(Int);
    Byte buf[sizeof(Int)] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = (Byte)fetch_byte(vm);

    Int ret = 0;
    memcpy(&ret, buf, SIZE);

    return ret;
}

static Float fetch_float(VM *vm)
{
    const int SIZE = sizeof(Float);
    Byte buf[sizeof(Float)] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = (Byte)fetch_byte(vm);

    Float ret = 0;
    memcpy(&ret, buf, SIZE);

    return ret;
}

static Word fetch_str(VM *vm)
{
    const int SIZE = sizeof(Word);
    Byte buf[sizeof(Word)] = {0};

    for ( int i = 0; i < SIZE; i++ )
        buf[i] = (Byte)fetch_byte(vm);

    Word ret = 0;
    memcpy(&ret, buf, SIZE);

    return ret;
}

static void push(VM *vm, Value val)
{
    if (vm->sp_ == vm->stack_.len - 1) {
        push_value(&vm->stack_, val);
        vm->sp_++;
    }
    else {
        vm->stack_.data[++vm->sp_] = val;
    }
}

static Value pop(VM *vm)
{
    return vm->stack_.data[vm->sp_--];
}

static Value top(const VM *vm)
{
    return vm->stack_.data[vm->sp_];
}

static Int pop_int(VM *vm)
{
    const Value val = pop(vm);
    return val.inum;
}

static Float pop_float(VM *vm)
{
    const Value val = pop(vm);
    return val.fpnum;
}

static void push_int(VM *vm, Int inum)
{
    Value val;
    val.inum = inum;
    push(vm, val);
}

static void push_float(VM *vm, Float fpnum)
{
    Value val;
    val.fpnum = fpnum;
    push(vm, val);
}

static void push_call(VM *vm, const Call *call)
{
    if (vm->call_sp_ == vm->callstack_.len - 1) {
        push_callinfo(&vm->callstack_, call);
        vm->call_sp_++;
    }
    else {
        vm->callstack_.data[++vm->call_sp_] = *call;
    }
}

static Call pop_call(VM *vm)
{
    return vm->callstack_.data[vm->call_sp_--];
}

static Value get_local(const VM *vm, int id)
{
    return vm->stack_.data[vm->bp_ + 1 + id];
}

static Value get_global(const VM *vm, int id)
{
    return vm->stack_.data[1 + id];
}

static void set_local(VM *vm, int id, Value val)
{
    vm->stack_.data[vm->bp_ + 1 + id] = val;
}

static void set_global(VM *vm, int id, Value val)
{
    vm->stack_.data[1 + id] = val;
}

static bool is_eoc(const VM *vm)
{
    return vm->ip_ == vm->eoc_;
}

static void run(VM *vm)
{
    bool brk = false;

    while (!is_eoc(vm) && !brk) {
        const Int old_ip = vm->ip_;
        const Int op = fetch_byte(vm);

        if (vm->print_stack_) {
            printf("[%6lld] %s\n", old_ip, OpcodeString(op));
            PrintStack(vm);
        }

        switch (op) {

        case OP_LOADB:
            {
                Value val;
                val.inum = fetch_byte(vm);
                push(vm, val);
            }
            break;

        case OP_LOADI:
            {
                Value val;
                val.inum = fetch_int(vm);
                push(vm, val);
            }
            break;

        case OP_LOADF:
            {
                Value val;
                val.fpnum = fetch_float(vm);
                push(vm, val);
            }
            break;

        case OP_LOADS:
            {
                const Word id = fetch_str(vm);
                const char *s = GetConstString(vm->code_, id);
                Value val;
                val.str = NewString(&vm->gc_, s);
                push(vm, val);
            }
            break;

        case OP_LOADLOCAL:
            {
                const Int id = fetch_byte(vm);
                const Value val = get_local(vm, id);
                push(vm, val);
            }
            break;

        case OP_LOADGLOBAL:
            {
                const Int id = fetch_word(vm);
                const Value val = get_global(vm, id);
                push(vm, val);
            }
            break;

        case OP_STORELOCAL:
            {
                const Int id = fetch_byte(vm);
                const Value val = pop(vm);
                set_local(vm, id, val);
            }
            break;

        case OP_STOREGLOBAL:
            {
                const Int id = fetch_word(vm);
                const Value val = pop(vm);
                set_global(vm, id, val);
            }
            break;

        case OP_LOAD:
            {
                const Value addr = pop(vm);
                const Value val = vm->stack_.data[addr.inum];
                push(vm, val);
            }
            break;

        case OP_STORE:
            {
                const Value addr = pop(vm);
                const Value val = pop(vm);
                vm->stack_.data[addr.inum] = val;
            }
            break;

        case OP_INCLOCAL:
            {
                const Int id = fetch_byte(vm);
                Value val = get_local(vm, id);
                val.inum++;
                set_local(vm, id, val);
            }
            break;

        case OP_INCGLOBAL:
            {
                const Int id = fetch_word(vm);
                Value val = get_global(vm, id);
                val.inum++;
                set_global(vm, id, val);
            }
            break;

        case OP_DECLOCAL:
            {
                const Int id = fetch_byte(vm);
                Value val = get_local(vm, id);
                val.inum--;
                set_local(vm, id, val);
            }
            break;

        case OP_DECGLOBAL:
            {
                const Int id = fetch_word(vm);
                Value val = get_global(vm, id);
                val.inum--;
                set_global(vm, id, val);
            }
            break;

        case OP_ALLOC:
            {
                const Int size = fetch_byte(vm);
                set_sp(vm, vm->sp_ + size);
            }
            break;

        case OP_CLEAR_LOCAL:
            {
                const uint64_t base = fetch_word(vm);
                const uint64_t count = fetch_word(vm);
                const struct Value zero = {0};

                for (int i = 0; i < count; i++)
                    set_local(vm, base + i, zero);
            }
            break;

        case OP_CLEAR_GLOBAL:
            {
                const uint64_t base = fetch_word(vm);
                const uint64_t count = fetch_word(vm);
                const struct Value zero = {0};

                for (int i = 0; i < count; i++)
                    set_global(vm, base + i, zero);
            }
            break;

        case OP_COPY_LOCAL:
            {
                const uint64_t src = fetch_word(vm);
                const uint64_t dst = fetch_word(vm);
                const uint64_t count = fetch_word(vm);

                for (int i = 0; i < count; i++) {
                    const struct Value val = get_local(vm, src + i);
                    set_local(vm, dst + i, val);
                }
            }
            break;

        case OP_COPY_GLOBAL:
            {
                const uint64_t src = fetch_word(vm);
                const uint64_t dst = fetch_word(vm);
                const uint64_t count = fetch_word(vm);

                for (int i = 0; i < count; i++) {
                    const struct Value val = get_global(vm, src + i);
                    set_global(vm, dst + i, val);
                }
            }
            break;

        case OP_LOADA:
            {
                const Int id = fetch_word(vm);
                push_int(vm, vm->bp_ + 1 + id);
            }
            break;

        case OP_DEREF:
            {
                const Int addr  = pop_int(vm);
                const Value val = vm->stack_.data[addr];
                push(vm, val);
            }
            break;

        case OP_INDEX:
            {
                const long index = pop_int(vm);
                const long base = pop_int(vm);
                const long len = vm->stack_.data[base].inum;

                if (index >= len) {
                    fprintf(stderr,
                            "panic: runtime error: index out of range[%ld] with length %ld\n",
                            index, len);
                    exit(1);
                }

                // index from next to base
                const long indexed = base + index + 1;
                push_int(vm, indexed);
            }
            break;

        case OP_LOADTYPEN:
            push_int(vm, TID_NIL);
            break;

        case OP_LOADTYPEB:
            push_int(vm, TID_BOL);
            break;

        case OP_LOADTYPEI:
            push_int(vm, TID_INT);
            break;

        case OP_LOADTYPEF:
            push_int(vm, TID_FLT);
            break;

        case OP_LOADTYPES:
            push_int(vm, TID_STR);
            break;

        case OP_CALL:
            {
                const uint16_t func_index = fetch_word(vm);
                const int64_t func_addr = GetFunctionAddress(vm->code_, func_index);

                Call call = {0};
                call.argc = GetFunctionArgCount(vm->code_, func_index);
                call.return_ip = vm->ip_;
                call.return_bp = vm->bp_;
                push_call(vm, &call);

                set_ip(vm, func_addr);
                set_bp(vm, vm->sp_ - call.argc);
            }
            break;

        case OP_CALL_POINTER:
            {
                const Value val = pop(vm);
                const uint16_t func_index = val.inum;
                const int64_t func_addr = GetFunctionAddress(vm->code_, func_index);

                // TODO make function for common part
                Call call = {0};
                call.argc = GetFunctionArgCount(vm->code_, func_index);
                call.return_ip = vm->ip_;
                call.return_bp = vm->bp_;
                push_call(vm, &call);

                set_ip(vm, func_addr);
                set_bp(vm, vm->sp_ - call.argc);
            }
            break;

        case OP_CALL_BUILTIN:
            {
                const Byte func_index = fetch_byte(vm);

                if (func_index == 0) {
                    // builtin "print" function
                    // FIXME hard coded variadic
                    const int argc = pop_int(vm);
                    Value dummy = {0};
                    ValueVec args = {0};
                    push_value(&args, dummy);
                    int sp = 0;

                    // pop args
                    for (int i = 0; i < argc; i++) {
                        const Value type = pop(vm);
                        const Value val = pop(vm);
                        push_value(&args, val);
                        sp++;
                        push_value(&args, type);
                        sp++;
                    }

                    while (sp > 0) {
                        const Value type = args.data[sp--];
                        const Value val = args.data[sp--];

                        const Int id = type.inum;

                        switch (id) {
                        case TID_NIL:
                            break;

                        case TID_BOL:
                            if (val.inum == 0)
                                printf("false");
                            else
                                printf("true");
                            break;

                        case TID_INT:
                            printf("%lld", val.inum);
                            break;

                        case TID_FLT:
                            printf("%g", val.fpnum);
                            break;

                        case TID_STR:
                            printf("%s", val.str->data);
                            break;
                        }

                        // peek next arg
                        if (sp > 0) {
                            const Value next_type = args.data[sp];
                            if (next_type.inum == TID_NIL) {
                                // remove nil and skip separator
                                sp--;
                                sp--;
                                continue;
                            }
                        }

                        if (sp == 0)
                            printf("\n");
                        else
                            printf(" ");
                    }
                    // ret val
                    Value ret = {0};
                    push(vm, ret);
                }
                else if (func_index == 1) {
                    // builtin "exit" function
                    Value ret_code = pop(vm);
                    // ret val
                    push(vm, ret_code);
                    brk = true;
                }
            }
            break;

        case OP_RET:
            {
                const Value ret_obj = top(vm);
                const Call call = pop_call(vm);

                set_ip(vm, call.return_ip);
                set_sp(vm, vm->bp_);
                set_bp(vm, call.return_bp);
                push(vm, ret_obj);
            }
            break;

        case OP_JMP:
            {
                const Int addr = fetch_word(vm);
                set_ip(vm, addr);
            }
            break;

        case OP_JEQ:
            {
                const Int addr = fetch_word(vm);
                const Value cond = pop(vm);

                if (cond.inum == 0)
                    set_ip(vm, addr);
            }
            break;

        case OP_ADD:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 + val1);
            }
            break;

        case OP_ADDF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_float(vm, val0 + val1);
            }
            break;

        case OP_CATS:
            {
                const Value val1 = pop(vm);
                const Value val0 = pop(vm);
                Value val;
                //FIXME
                char buf[1024] = {'\0'};
                snprintf(buf, 1024, "%s%s", val0.str->data, val1.str->data);
                val.str = NewString(&vm->gc_, buf);
                //val.str = NewString(&vm->gc_, val0.str->str + val1.str->str);
                push(vm, val);
            }
            break;

        case OP_SUB:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 - val1);
            }
            break;

        case OP_SUBF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_float(vm, val0 - val1);
            }
            break;

        case OP_MUL:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l * r);
            }
            break;

        case OP_MULF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_float(vm, val0 * val1);
            }
            break;

        case OP_DIV:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                // TODO check zero div
                push_int(vm, l / r);
            }
            break;

        case OP_DIVF:
            {
                const Float r = pop_float(vm);
                const Float l = pop_float(vm);
                // TODO check zero div
                push_float(vm, l / r);
            }
            break;

        case OP_REM:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                // TODO check zero div
                push_int(vm, l % r);
            }
            break;

        case OP_REMF:
            {
                const Float r = pop_float(vm);
                const Float l = pop_float(vm);
                // TODO check zero div
                push_float(vm, fmod(l, r));
            }
            break;

        case OP_EQ:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 == val1);
            }
            break;

        case OP_EQF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                // TODO do fpnum comp
                push_int(vm, val0 == val1);
            }
            break;

        case OP_EQS:
            {
                const Value val1 = pop(vm);
                const Value val0 = pop(vm);
                Value val;
                val.inum = !strcmp(val0.str->data, val1.str->data);
                push(vm, val);
            }
            break;

        case OP_NEQ:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 != val1);
            }
            break;

        case OP_NEQF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_int(vm, val0 != val1);
            }
            break;

        case OP_NEQS:
            {
                const Value val1 = pop(vm);
                const Value val0 = pop(vm);
                Value val;
                val.inum = strcmp(val0.str->data, val1.str->data);
                push(vm, val);
            }
            break;

        case OP_LT:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 < val1);
            }
            break;

        case OP_LTF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_int(vm, val0 < val1);
            }
            break;

        case OP_LTE:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 <= val1);
            }
            break;

        case OP_LTEF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_int(vm, val0 <= val1);
            }
            break;

        case OP_GT:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 > val1);
            }
            break;

        case OP_GTF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_int(vm, val0 > val1);
            }
            break;

        case OP_GTE:
            {
                const Int val1 = pop_int(vm);
                const Int val0 = pop_int(vm);
                push_int(vm, val0 >= val1);
            }
            break;

        case OP_GTEF:
            {
                const Float val1 = pop_float(vm);
                const Float val0 = pop_float(vm);
                push_int(vm, val0 >= val1);
            }
            break;

        case OP_AND:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l & r);
            }
            break;

        case OP_OR:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l | r);
            }
            break;

        case OP_XOR:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l ^ r);
            }
            break;

        case OP_NOT:
            {
                const Int i = pop_int(vm);
                push_int(vm, ~i);
            }
            break;

        case OP_SHL:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l << r);
            }
            break;

        case OP_SHR:
            {
                const Int r = pop_int(vm);
                const Int l = pop_int(vm);
                push_int(vm, l >> r);
            }
            break;

        case OP_NEG:
            {
                const Int i = pop_int(vm);
                push_int(vm, -1 * i);
            }
            break;

        case OP_NEGF:
            {
                const Float f = pop_float(vm);
                push_float(vm, -1 * f);
            }
            break;

        case OP_SETZ:
            {
                const Int i = pop_int(vm);
                push_int(vm, i == 0);
            }
            break;

        case OP_SETNZ:
            {
                const Int i = pop_int(vm);
                push_int(vm, i != 0);
            }
            break;

        case OP_POP:
            {
                pop(vm);
            }
            break;

        case OP_DUP:
            {
                const Value v = top(vm);
                push(vm, v);
            }
            break;

        case OP_BTOI:
            {
                const Int i = pop_int(vm);
                push_int(vm, i != 0);
            }
            break;

        case OP_BTOF:
            {
                const Int i = pop_int(vm);
                push_float(vm, i);
            }
            break;

        case OP_ITOB:
            {
                const Int i = pop_int(vm);
                push_int(vm, i != 0);
            }
            break;

        case OP_ITOF:
            {
                const Int i = pop_int(vm);
                push_float(vm, i);
            }
            break;

        case OP_FTOB:
            {
                const Float f = pop_float(vm);
                push_int(vm, f != 0.f);
            }
            break;

        case OP_FTOI:
            {
                const Float f = pop_float(vm);
                push_int(vm, f);
            }
            break;

        case OP_ARRAYLOCAL:
            {
                const Int len = pop_int(vm);
                Value val;
                val.array = NewArray(&vm->gc_, len);
                push(vm, val);
            }
            break;

        case OP_PUSH_CHECK_NUM:
            {
                Value val;
                val.inum = fetch_int(vm);
                push(vm, val);
            }
            break;

        case OP_POP_CHECK_NUM:
            {
                int64_t check_num = fetch_int(vm);
                Value val = pop(vm);
                if (val.inum != check_num) {
                    fprintf(stderr, "ERROR: checknum %lld\n", check_num);
                    exit(EXIT_FAILURE);
                }
            }
            break;

        case OP_EXIT:
        case OP_EOC:
            brk = true;
            break;

        case OP_NOP:
            break;

        default:
            UNREACHABLE;
            break;
        }
    }
}

void Run(VM *vm, const Bytecode *code)
{
    vm->code_ = code;
    vm->eoc_ = Size(vm->code_);

    // empty data at the bottom of stacks
    Value val = {0};
    push_value(&vm->stack_, val);
    vm->sp_ = 0;

    Call call = {0};
    push_callinfo(&vm->callstack_, &call);
    vm->call_sp_ = 0;

    run(vm);
}

Int StackTopInt(const VM *vm)
{
    const Value val = top(vm);
    return val.inum;
}

void PrintStack(const VM *vm)
{
    printf("    ------\n");
    for (Int i = vm->sp_; i >= 0; i--)
    {
        if (i == vm->sp_)
            printf("SP->");
        else
            printf("    ");

        printf("|%4llu|", vm->stack_.data[i].inum);

        if (i <= vm->sp_ && i > vm->bp_)
            printf(" [%lld]", i - vm->bp_ - 1);

        if (i == vm->bp_)
            printf("<-BP");
        printf("\n");
    }
    printf("--------------\n\n");
}

void EnablePrintStack(VM *vm, bool enable)
{
    vm->print_stack_ = enable;
}

void PrintObjs(const VM *vm)
{
    PrintObjects(&vm->gc_);
}
