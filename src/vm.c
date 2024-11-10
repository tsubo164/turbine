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

static void resize_stack(struct runtime_valuevec *v, int new_len)
{
    if (new_len >= v->cap) {
        v->cap = v->cap < 256 ? 256 : v->cap;
        while (v->cap < new_len)
            v->cap *= 2;
        // TODO Remove cast
        v->data = (struct runtime_value *) realloc(v->data, v->cap * sizeof(*v->data));
    }
    v->len = new_len;
}

static void push_value(struct runtime_valuevec *v, struct runtime_value val)
{
    if (v->len >= v->cap) {
        v->cap = new_cap(v->cap, 256);
        // TODO Remove cast
        v->data = (struct runtime_value *) realloc(v->data, v->cap * sizeof(*v->data));
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

static struct runtime_value top(const VM *vm)
{
    return vm->stack_.data[vm->sp_];
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

#define SPOFFSET 1
static int64_t index_to_addr(int64_t index)
{
    return index - SPOFFSET;
}

static int64_t addr_to_index(int64_t addr)
{
    return addr + SPOFFSET;
}

static int id_to_addr(const struct VM *vm, int id)
{
    return index_to_addr(vm->bp_) + 1 + id;
}

static int addr_to_id(const struct VM *vm, int addr)
{
    return addr - (index_to_addr(vm->bp_) + 1);
}

static struct runtime_value read_stack(const VM *vm, int64_t addr)
{
    int64_t index = addr_to_index(addr);
    return vm->stack_.data[index];
}

static void write_stack(VM *vm, int64_t addr, struct runtime_value val)
{
    int64_t index = addr_to_index(addr);
    vm->stack_.data[index] = val;
}

static struct runtime_value get_local(const VM *vm, int id)
{
    int64_t addr = id_to_addr(vm, id);
    return read_stack(vm, addr);
}

static void set_local(VM *vm, int id, struct runtime_value val)
{
    int64_t addr = id_to_addr(vm, id);
    write_stack(vm, addr, val);
}

static struct runtime_value get_global(const VM *vm, int addr)
{
    return read_stack(vm, addr);
}

static void set_global(VM *vm, int addr, struct runtime_value val)
{
    write_stack(vm, addr, val);
}

static struct runtime_value fetch_register_value(struct VM *vm, int id)
{
    if (IsImmediateValue__(id)) {
        struct runtime_value imm = {0};
        int imm_size = 0;

        imm = ReadImmediateValue__(vm->code_, vm->ip_, id, &imm_size);
        vm->ip_ += imm_size;

        return imm;
    }
    else {
        return get_local(vm, id);
    }
}

// XXX TEST
static void run__(VM *vm);
void Run(VM *vm, const Bytecode *code)
{
    vm->code_ = code;
    //vm->eoc_ = Size(vm->code_);
    vm->eoc = Size__(vm->code_);

    // empty data at the bottom of stacks
    struct runtime_value val = {0};
    push_value(&vm->stack_, val);
    vm->sp_ = 0;

    Call call = {0};
    push_callinfo(&vm->callstack_, &call);
    vm->call_sp_ = 0;

    run__(vm);
}

Int StackTopInt(const VM *vm)
{
    const struct runtime_value val = top(vm);
    return val.inum;
}

void PrintStack(const VM *vm)
{
    printf("    ------\n");
    for (Int i = vm->sp_; i >= 0; i--) {
        if (i <= vm->sp_ && i > 0)
            printf("[%6lld] ", index_to_addr(i));
        else if (i == 0)
            printf("[%6s] ", "*");

        if (i == vm->sp_)
            printf("SP->");
        else
            printf("    ");

        printf("|%4llu|", vm->stack_.data[i].inum);

        if (i <= vm->sp_ && i > vm->bp_)
        {
            int64_t addr = index_to_addr(i);
            int64_t id = addr_to_id(vm, addr);
            printf(" [%lld]", id);
        }

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

// XXX TEST
static bool is_eoc__(const VM *vm)
{
    return vm->ip_ == vm->eoc;
}

static uint32_t fetch__(VM *vm)
{
    return Read__(vm->code_, vm->ip_++);
}

static void run__(VM *vm)
{
    bool brk = false;

    while (!is_eoc__(vm) && !brk) {
        const Int old_ip = vm->ip_;
        const uint32_t instcode = fetch__(vm);

        struct code_instruction inst = {0};
        code_decode_instruction(instcode, &inst);

        if (vm->print_stack_) {
            int imm_size = 0;
            PrintInstruction__(vm->code_, old_ip, &inst, &imm_size);
            PrintStack(vm);
        }

        switch (inst.op) {

        /*
        case OP_LOADB:
            {
                struct runtime_value val;
                val.inum = fetch_byte(vm);
                push(vm, val);
            }
            break;

        case OP_LOADI:
            {
                struct runtime_value val;
                val.inum = fetch_int(vm);
                push(vm, val);
            }
            break;

        case OP_LOADF:
            {
                struct runtime_value val;
                val.fpnum = fetch_float(vm);
                push(vm, val);
            }
            break;

        case OP_LOADS:
            {
                const Word id = fetch_str(vm);
                const char *s = GetConstString(vm->code_, id);
                struct runtime_value val;
                val.str = NewString(&vm->gc_, s);
                push(vm, val);
            }
            break;

        case OP_LOADLOCAL:
            {
                const Int id = fetch_byte(vm);
                const struct runtime_value val = get_local(vm, id);
                push(vm, val);
            }
            break;

        case OP_LOADGLOBAL:
            {
                const Int id = fetch_word(vm);
                const struct runtime_value val = get_global(vm, id);
                push(vm, val);
            }
            break;

        case OP_STORELOCAL:
            {
                const Int id = fetch_byte(vm);
                const struct runtime_value val = pop(vm);
                set_local(vm, id, val);
            }
            break;

        case OP_STOREGLOBAL:
            {
                const Int id = fetch_word(vm);
                const struct runtime_value val = pop(vm);
                set_global(vm, id, val);
            }
            break;

        case OP_LOAD:
            {
                const struct runtime_value addr = pop(vm);
                const struct runtime_value val = vm->stack_.data[addr.inum];
                push(vm, val);
            }
            break;

        case OP_DECGLOBAL:
            {
                const Int id = fetch_word(vm);
                struct runtime_value val = get_global(vm, id);
                val.inum--;
                set_global(vm, id, val);
            }
            break;
            */

        case OP_ALLOCATE:
            {
                const Int size = inst.A;
                set_sp(vm, vm->sp_ + size);
            }
            break;

            /*
        case OP_CLEAR_LOCAL:
            {
                const uint64_t base = fetch_word(vm);
                const uint64_t count = fetch_word(vm);
                const struct runtime_value zero = {0};

                for (int i = 0; i < count; i++)
                    set_local(vm, base + i, zero);
            }
            break;

        case OP_CLEAR_GLOBAL:
            {
                const uint64_t base = fetch_word(vm);
                const uint64_t count = fetch_word(vm);
                const struct runtime_value zero = {0};

                for (int i = 0; i < count; i++)
                    set_global(vm, base + i, zero);
            }
            break;
            */

        case OP_MOVE:
            {
                const uint8_t dst = inst.A;
                const uint8_t src = inst.B;
                const struct runtime_value val = fetch_register_value(vm, src);

                set_local(vm, dst, val);
            }
            break;

        case OP_LOADINT:
            {
                uint8_t dst = inst.A;
                int64_t lo = fetch__(vm);
                int64_t hi = fetch__(vm);
                struct runtime_value val;

                val.inum = (hi << 32) | lo;
                set_local(vm, dst, val);
            }
            break;

        case OP_LOADFLOAT:
            {
                uint8_t dst = inst.A;
                int64_t lo = fetch__(vm);
                int64_t hi = fetch__(vm);
                int64_t inum = (hi << 32) | lo;
                struct runtime_value val;

                val.fpnum = *((Float *)&inum);
                set_local(vm, dst, val);
            }
            break;

        case OP_LOAD:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value srcaddr = fetch_register_value(vm, src);
                struct runtime_value val = get_global(vm, srcaddr.inum);

                set_local(vm, dst, val);
            }
            break;

        case OP_STORE:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value val0 = fetch_register_value(vm, dst);
                struct runtime_value val1 = fetch_register_value(vm, src);

                set_global(vm, val0.inum, val1);
            }
            break;

        case OP_LOADARRAY:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;
                struct runtime_value src = fetch_register_value(vm, reg1);
                struct runtime_value idx = fetch_register_value(vm, reg2);

                struct runtime_value val = ArrayGet(src.array, idx.inum);
                set_local(vm, reg0, val);
            }
            break;

        case OP_STOREARRAY:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;
                struct runtime_value dst = fetch_register_value(vm, reg0);
                struct runtime_value idx = fetch_register_value(vm, reg1);
                struct runtime_value src = fetch_register_value(vm, reg2);

                ArraySet(dst.array, idx.inum, src);
            }
            break;

        case OP_LOADSTRUCT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t field_idx = inst.C;
                struct runtime_value src = fetch_register_value(vm, reg1);
                struct runtime_value val = runtime_struct_get(src.strct, field_idx);

                set_local(vm, reg0, val);
            }
            break;

        case OP_STORESTRUCT:
            {
                uint8_t reg0 = inst.A;
                uint8_t field_idx = inst.B;
                uint8_t reg2 = inst.C;
                struct runtime_value dst = fetch_register_value(vm, reg0);
                struct runtime_value src = fetch_register_value(vm, reg2);

                runtime_struct_set(dst.strct, field_idx, src);
            }
            break;
            /*
        case OP_COPY_GLOBAL:
            {
                const uint64_t src = fetch_word(vm);
                const uint64_t dst = fetch_word(vm);
                const uint64_t count = fetch_word(vm);

                for (int i = 0; i < count; i++) {
                    const struct runtime_value val = get_global(vm, src + i);
                    set_global(vm, dst + i, val);
                }
            }
            break;
            */

        case OP_LOADADDR:
            {
                int reg0 = inst.A;
                int reg1 = inst.B;
                struct runtime_value addr;

                addr.inum = id_to_addr(vm, reg1);
                set_local(vm, reg0, addr);
            }
            break;

        case OP_DEREF:
            {
                int reg0 = inst.A;
                int reg1 = inst.B;
                struct runtime_value addr = fetch_register_value(vm, reg1);
                struct runtime_value val = get_global(vm, addr.inum);

                set_local(vm, reg0, val);
            }
            break;

            /*
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
            */

        case OP_LOADTYPENIL:
            {
                int dst = inst.A;
                struct runtime_value val;
                val.inum = TID_NIL;
                set_local(vm, dst, val);
            }
            break;

        case OP_LOADTYPEBOOL:
            {
                int dst = inst.A;
                struct runtime_value val;
                val.inum = TID_BOL;
                set_local(vm, dst, val);
            }
            break;

        case OP_LOADTYPEINT:
            {
                int dst = inst.A;
                struct runtime_value val;
                val.inum = TID_INT;
                set_local(vm, dst, val);
            }
            break;

        case OP_LOADTYPEFLOAT:
            {
                int dst = inst.A;
                struct runtime_value val;
                val.inum = TID_FLT;
                set_local(vm, dst, val);
            }
            break;

        case OP_LOADTYPESTRING:
            {
                int dst = inst.A;
                struct runtime_value val;
                val.inum = TID_STR;
                set_local(vm, dst, val);
            }
            break;

        // array/struct
        case OP_NEWARRAY:
            {
                int dst = inst.A;
                int reg1 = inst.B;
                struct runtime_value len = fetch_register_value(vm, reg1);
                struct runtime_value val;

                val.array = ArrayNew(&vm->gc_, len.inum);
                set_local(vm, dst, val);
            }
            break;

        case OP_NEWSTRUCT:
            {
                int dst = inst.A;
                int len = inst.B;

                struct runtime_struct *s = runtime_struct_new(len);
                runtime_append_gc_object(&vm->gc_, (struct Obj*) s);

                struct runtime_value val = {.strct = s};
                set_local(vm, dst, val);
            }
            break;

        // function call
        case OP_CALL:
            {
                uint16_t func_index = inst.BB;
                int64_t func_addr = GetFunctionAddress(vm->code_, func_index);

                Call call = {0};
                call.argc = GetFunctionArgCount(vm->code_, func_index);
                call.return_ip = vm->ip_;
                call.return_bp = vm->bp_;
                call.return_sp = vm->sp_;
                call.return_reg = inst.A;
                push_call(vm, &call);

                set_ip(vm, func_addr);
                // TODO make reg_to_addr()
                set_bp(vm, vm->bp_ + 1 + call.return_reg - 1);

                // Register allocation (parameters + local variables)
                int max_reg_count = GetMaxRegisterCount__(vm->code_, func_index);
                set_sp(vm, vm->bp_ + max_reg_count);
            }
            break;

        case OP_CALLPOINTER:
            {
                int ret = inst.A;
                int src = inst.B;
                struct runtime_value src_val = fetch_register_value(vm, src);
                int func_index = src_val.inum;
                int64_t func_addr = GetFunctionAddress(vm->code_, func_index);

                Call call = {0};
                call.argc = GetFunctionArgCount(vm->code_, func_index);
                call.return_ip = vm->ip_;
                call.return_bp = vm->bp_;
                call.return_sp = vm->sp_;
                call.return_reg = ret;
                push_call(vm, &call);

                set_ip(vm, func_addr);
                // TODO make reg_to_addr()
                set_bp(vm, vm->bp_ + 1 + call.return_reg - 1);

                // Register allocation (parameters + local variables)
                int max_reg_count = GetMaxRegisterCount__(vm->code_, func_index);
                set_sp(vm, vm->bp_ + max_reg_count);
            }
            break;

        case OP_CALLBUILTIN:
            {
                int ret_reg = inst.A;
                int func_index = inst.BB;

                if (func_index == 0) {
                    // builtin "print" function
                    // FIXME hard coded variadic

                    int old_bp = vm->bp_;
                    int old_sp = vm->sp_;

                    set_bp(vm, vm->bp_ + 1 + ret_reg - 1);
                    int max_reg_count = 0;
                    set_sp(vm, vm->bp_ + max_reg_count);

                    struct runtime_value arg_count = fetch_register_value(vm, 0);
                    int argc = arg_count.inum;
                    int arg_reg = 1;

                    for (int i = 0; i < argc; i ++) {

                        struct runtime_value val = fetch_register_value(vm, arg_reg++);
                        struct runtime_value type = fetch_register_value(vm, arg_reg++);

                        switch (type.inum) {

                        case TID_NIL:
                            continue;

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
                        bool skip_separator = false;
                        if (i < argc - 1) {
                            struct runtime_value next_type = fetch_register_value(vm, arg_reg + 1);
                            if (next_type.inum == TID_NIL)
                                skip_separator = true;
                        }

                        if (skip_separator)
                            continue;

                        int separator = (i == argc - 1) ? '\n' : ' ';
                        printf("%c", separator);
                    }

                    set_bp(vm, old_bp);
                    set_sp(vm, old_sp);

                    // ret val
                    struct runtime_value ret_val = {0};
                    set_local(vm, ret_reg, ret_val);
                }
                else if (func_index == 1) {
                    int src = inst.A;
                    struct runtime_value ret_code = fetch_register_value(vm, src);
                    // TODO push?
                    //set_global(vm, vm->sp_, ret_code);
                    vm->stack_.data[vm->sp_] = ret_code;
                    brk = true;
                }
            }
            break;

        case OP_RETURN:
            {
                uint8_t reg_id = inst.A;
                struct runtime_value ret_val = fetch_register_value(vm, reg_id);
                struct Call call = pop_call(vm);
                uint8_t ret_reg = call.return_reg;

                set_ip(vm, call.return_ip);
                set_bp(vm, call.return_bp);
                set_sp(vm, call.return_sp);
                set_local(vm, ret_reg, ret_val);
            }
            break;

        case OP_JUMP:
            {
                uint16_t addr = inst.BB;
                set_ip(vm, addr);
            }
            break;

        case OP_JUMPIFZERO:
            {
                uint8_t reg0 = inst.A;
                uint16_t addr = inst.BB;
                struct runtime_value cond = fetch_register_value(vm, reg0);

                if (cond.inum == 0)
                    set_ip(vm, addr);
            }
            break;

        case OP_JUMPIFNOTZ:
            {
                uint8_t reg0 = inst.A;
                uint16_t addr = inst.BB;
                struct runtime_value cond = fetch_register_value(vm, reg0);

                if (cond.inum != 0)
                    set_ip(vm, addr);
            }
            break;

#define BINOP(op,field,zerocheck) \
do { \
    uint8_t reg0 = inst.A; \
    uint8_t reg1 = inst.B; \
    uint8_t reg2 = inst.C; \
    struct runtime_value val1 = fetch_register_value(vm, reg1); \
    struct runtime_value val2 = fetch_register_value(vm, reg2); \
    struct runtime_value val0; \
    val0.field = val1.field op val2.field; \
    set_local(vm, reg0, val0); \
} while (0)
        // arithmetic
        case OP_ADDINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.inum = val1.inum + val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_ADDFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.fpnum = val1.fpnum + val2.fpnum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_SUBINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.inum = val1.inum - val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_SUBFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.fpnum = val1.fpnum - val2.fpnum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_MULINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.inum = val1.inum * val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_MULFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.fpnum = val1.fpnum * val2.fpnum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_DIVINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                // TODO check zero division
                val0.inum = val1.inum / val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_DIVFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                // TODO check zero division
                val0.fpnum = val1.fpnum / val2.fpnum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_REMINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                // TODO check zero division
                val0.inum = val1.inum % val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_REMFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                // TODO check zero division
                val0.fpnum = fmod(val1.fpnum, val2.fpnum);
                set_local(vm, reg0, val0);
            }
            break;

            // TODO move this
        case OP_CATSTRING:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                struct StringObj *s = runtime_string_concat(val1.str, val2.str);
                runtime_append_gc_object(&vm->gc_, (struct Obj*) s);

                val0.str = s;
                set_local(vm, reg0, val0);
            }
            break;

        case OP_EQINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                uint8_t reg2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, reg1);
                struct runtime_value val2 = fetch_register_value(vm, reg2);
                struct runtime_value val0;

                val0.inum = val1.inum == val2.inum;
                set_local(vm, reg0, val0);
            }
            break;

#define DO_BINOP__(r0, r1, op, r2, zerocheck) \
do { \
    uint8_t reg0 = inst.A; \
    uint8_t reg1 = inst.B; \
    uint8_t reg2 = inst.C; \
    struct runtime_value val1 = fetch_register_value(vm, reg1); \
    struct runtime_value val2 = fetch_register_value(vm, reg2); \
    struct runtime_value val0; \
    if ((zerocheck)) {\
        /* runtime error */ \
    } \
    val0.r0 = val1.r1 op val2.r2; \
    set_local(vm, reg0, val0); \
} while (0)
        case OP_EQFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                // The result of op is int (bool)
                val0.inum = val1.fpnum == val2.fpnum;
                set_local(vm, dst, val0);
            }
            //DO_BINOP__(inum, fpnum, ==, fpnum, 0);
            break;

        case OP_EQSTRING:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                // The result of op is int (bool)
                val0.inum = runtime_string_compare(val1.str, val2.str) == 0;
                set_local(vm, dst, val0);
            }
            break;

        case OP_NEQINT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                // The result of op is int (bool)
                val0.inum = val1.inum != val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_NEQFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                // The result of op is int (bool)
                val0.inum = val1.fpnum != val2.fpnum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_NEQSTRING:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                // The result of op is int (bool)
                val0.inum = runtime_string_compare(val1.str, val2.str) != 0;
                set_local(vm, dst, val0);
            }
            break;

        case OP_LTINT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum < val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_LTFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.fpnum < val2.fpnum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_LTEINT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum <= val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_LTEFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.fpnum <= val2.fpnum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_GTINT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum > val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_GTFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.fpnum > val2.fpnum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_GTEINT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum >= val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_GTEFLOAT:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.fpnum >= val2.fpnum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_BITWISEAND:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum & val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_BITWISEOR:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum | val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_BITWISEXOR:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum ^ val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_BITWISENOT:
            {
                uint8_t dst = inst.A;
                uint8_t src = inst.B;
                struct runtime_value val = fetch_register_value(vm, src);
                val.inum = ~val.inum;
                set_local(vm, dst, val);
            }
            break;

        case OP_SHL:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum << val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_SHR:
            {
                uint8_t dst = inst.A;
                uint8_t src1 = inst.B;
                uint8_t src2 = inst.C;

                struct runtime_value val1 = fetch_register_value(vm, src1);
                struct runtime_value val2 = fetch_register_value(vm, src2);
                struct runtime_value val0;

                val0.inum = val1.inum >> val2.inum;
                set_local(vm, dst, val0);
            }
            break;

        case OP_INC:
            {
                uint8_t src = inst.A;
                struct runtime_value val0 = fetch_register_value(vm, src);
                val0.inum++;
                set_local(vm, src, val0);
            }
            break;

        case OP_DEC:
            {
                uint8_t src = inst.A;
                struct runtime_value val0 = fetch_register_value(vm, src);
                val0.inum--;
                set_local(vm, src, val0);
            }
            break;

        case OP_NEGINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                struct runtime_value val1 = fetch_register_value(vm, reg1);
                val1.inum *= -1;
                set_local(vm, reg0, val1);
            }
            break;

        case OP_NEGFLOAT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                struct runtime_value val1 = fetch_register_value(vm, reg1);
                val1.fpnum *= -1.;
                set_local(vm, reg0, val1);
            }
            break;

        case OP_SETIFZERO:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                struct runtime_value val1 = fetch_register_value(vm, reg1);
                val1.inum = val1.inum == 0;
                set_local(vm, reg0, val1);
            }
            break;

        case OP_SETIFNOTZ:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                struct runtime_value val1 = fetch_register_value(vm, reg1);
                val1.inum = val1.inum == 0;
                set_local(vm, reg0, val1);
            }
            break;

            /*
        case OP_POP:
            {
                pop(vm);
            }
            break;

        case OP_DUP:
            {
                const struct runtime_value v = top(vm);
                push(vm, v);
            }
            break;
            */

        case OP_BOOLTOINT:
            {
                uint8_t reg0 = inst.A;
                uint8_t reg1 = inst.B;
                struct runtime_value val0;
                struct runtime_value val1 = fetch_register_value(vm, reg1);

                val0.inum = val1.inum != 0;
                set_local(vm, reg0, val0);
            }
            break;

            /*
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

        case OP_PUSH_CHECK_NUM:
            {
                struct runtime_value val;
                val.inum = fetch_int(vm);
                push(vm, val);
            }
            break;

        case OP_POP_CHECK_NUM:
            {
                int64_t check_num = fetch_int(vm);
                struct runtime_value val = pop(vm);
                if (val.inum != check_num) {
                    fprintf(stderr, "ERROR: checknum %lld\n", check_num);
                    exit(EXIT_FAILURE);
                }
            }
            break;
        */

        case OP_EXIT:
        case OP_EOC:
            brk = true;
            break;

        case OP_NOP:
            break;

        default:
            {
                fprintf(stderr, "** Unimplemented instruction: %d\n", inst.op);
                PrintInstruction__(vm->code_, old_ip, &inst, NULL);
                UNREACHABLE;
            }
            break;
        }
    }
}
