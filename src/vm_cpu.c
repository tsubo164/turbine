#include "vm_cpu.h"
#include "runtime_array.h"
#include "runtime_string.h"
#include "runtime_struct.h"
#include "code_print.h"

#include <string.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>
#include <math.h>

static void set_ip(struct vm_cpu *vm, int64_t ip)
{
    vm->ip = ip;
}

static void set_sp(struct vm_cpu *vm, int64_t sp)
{
    if (sp >= vm->stack.len)
        runtime_valuevec_resize(&vm->stack, sp + 1);

    vm->sp = sp;
}

static void set_bp(struct vm_cpu *vm, int64_t bp)
{
    vm->bp = bp;
}

static struct runtime_value top(const struct vm_cpu *vm)
{
    return vm->stack.data[vm->sp];
}

static void push_call(struct vm_cpu *vm, const struct vm_call *call)
{
    vm_callstack_push(&vm->callstack, call);
}

static void pop_call(struct vm_cpu *vm, struct vm_call *call)
{
    vm_callstack_pop(&vm->callstack, call);
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

static int id_to_addr(const struct vm_cpu *vm, int id)
{
    return index_to_addr(vm->bp) + 1 + id;
}

static int addr_to_id(const struct vm_cpu *vm, int addr)
{
    return addr - (index_to_addr(vm->bp) + 1);
}

static struct runtime_value read_stack(const struct vm_cpu *vm, int64_t addr)
{
    int64_t index = addr_to_index(addr);
    return vm->stack.data[index];
}

static void write_stack(struct vm_cpu *vm, int64_t addr, struct runtime_value val)
{
    int64_t index = addr_to_index(addr);
    vm->stack.data[index] = val;
}

static struct runtime_value get_local(const struct vm_cpu *vm, int id)
{
    int64_t addr = id_to_addr(vm, id);
    return read_stack(vm, addr);
}

static void set_local(struct vm_cpu *vm, int id, struct runtime_value val)
{
    int64_t addr = id_to_addr(vm, id);
    write_stack(vm, addr, val);
}

static struct runtime_value get_global(const struct vm_cpu *vm, int addr)
{
    return read_stack(vm, addr);
}

static void set_global(struct vm_cpu *vm, int addr, struct runtime_value val)
{
    write_stack(vm, addr, val);
}

static struct runtime_value fetch_register_value(struct vm_cpu *vm, int id)
{
    if (code_is_immediate_value(id)) {
        struct runtime_value imm = {0};
        int imm_size = 0;

        imm = code_read_immediate_value(vm->code, vm->ip, id, &imm_size);
        vm->ip += imm_size;

        return imm;
    }
    else {
        return get_local(vm, id);
    }
}

int64_t vm_get_stack_top(const struct vm_cpu *vm)
{
    const struct runtime_value val = top(vm);
    return val.inum;
}

void vm_print_stack(const struct vm_cpu *vm)
{
    printf("    ------\n");
    for (int64_t i = vm->sp; i >= 0; i--) {
        if (i <= vm->sp && i > 0)
            printf("[%6lld] ", index_to_addr(i));
        else if (i == 0)
            printf("[%6s] ", "*");

        if (i == vm->sp)
            printf("SP->");
        else
            printf("    ");

        printf("|%4llu|", vm->stack.data[i].inum);

        if (i <= vm->sp && i > vm->bp)
        {
            int64_t addr = index_to_addr(i);
            int64_t id = addr_to_id(vm, addr);
            printf(" [%lld]", id);
        }

        if (i == vm->bp)
            printf("<-BP");

        printf("\n");
    }
    printf("--------------\n\n");
}

void vm_enable_print_stack(struct vm_cpu *vm, bool enable)
{
    vm->print_stack = enable;
}

void vm_print_gc_objects(const struct vm_cpu *vm)
{
    runtime_gc_print_objects(&vm->gc);
}

static void call_function(struct vm_cpu *vm, int return_reg, int func_id)
{
    int64_t func_addr = code_get_function_address(vm->code, func_id);

    struct vm_call call = {0};
    call.argc = code_get_function_arg_count(vm->code, func_id);
    call.return_ip = vm->ip;
    call.return_bp = vm->bp;
    call.return_sp = vm->sp;
    call.return_reg = return_reg;
    push_call(vm, &call);

    set_ip(vm, func_addr);
    /* TODO make reg_to_addr() */
    set_bp(vm, vm->bp + 1 + call.return_reg - 1);

    /* Register allocation (parameters + local variables) */
    int max_reg_count = code_get_function_register_count(vm->code, func_id);
    set_sp(vm, vm->bp + max_reg_count);
}

static bool is_eoc(const struct vm_cpu *vm)
{
    return vm->ip == vm->eoc;
}

static uint32_t fetch(struct vm_cpu *vm)
{
    return code_read(vm->code, vm->ip++);
}

static void run_cpu(struct vm_cpu *vm)
{
    bool halt = false;

    while (!is_eoc(vm) && !halt) {
        int64_t old_ip = vm->ip;
        int32_t instcode = fetch(vm);

        struct code_instruction inst = {0};
        code_decode_instruction(instcode, &inst);

        if (vm->print_stack) {
            int imm_size = 0;
            code_print_instruction(vm->code, old_ip, &inst, &imm_size);
            vm_print_stack(vm);
        }

        switch (inst.op) {

        case OP_ALLOCATE:
            {
                int64_t size = inst.A;
                set_sp(vm, vm->sp + size);
            }
            break;

        case OP_MOVE:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value srcval = fetch_register_value(vm, src);

                set_local(vm, dst, srcval);
            }
            break;

        case OP_LOAD:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value srcaddr = fetch_register_value(vm, src);
                struct runtime_value srcval = get_global(vm, srcaddr.inum);

                set_local(vm, dst, srcval);
            }
            break;

        case OP_STORE:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstaddr = fetch_register_value(vm, dst);
                struct runtime_value srcval = fetch_register_value(vm, src);

                set_global(vm, dstaddr.inum, srcval);
            }
            break;

        case OP_LOADARRAY:
            {
                int dst = inst.A;
                int src = inst.B;
                int idx = inst.C;
                struct runtime_value srcobj = fetch_register_value(vm, src);
                struct runtime_value idxval = fetch_register_value(vm, idx);
                struct runtime_value srcval = runtime_array_get(srcobj.array, idxval.inum);

                set_local(vm, dst, srcval);
            }
            break;

        case OP_STOREARRAY:
            {
                uint8_t dst = inst.A;
                uint8_t idx = inst.B;
                uint8_t src = inst.C;
                struct runtime_value dstobj = fetch_register_value(vm, dst);
                struct runtime_value idxval = fetch_register_value(vm, idx);
                struct runtime_value srcval = fetch_register_value(vm, src);

                runtime_array_set(dstobj.array, idxval.inum, srcval);
            }
            break;

        case OP_LOADSTRUCT:
            {
                uint8_t dst = inst.A;
                uint8_t src = inst.B;
                uint8_t field_idx = inst.C;
                struct runtime_value srcobj = fetch_register_value(vm, src);
                struct runtime_value srcval = runtime_struct_get(srcobj.strct, field_idx);

                set_local(vm, dst, srcval);
            }
            break;

        case OP_STORESTRUCT:
            {
                uint8_t dst = inst.A;
                uint8_t field_idx = inst.B;
                uint8_t src = inst.C;
                struct runtime_value dstobj = fetch_register_value(vm, dst);
                struct runtime_value srcval = fetch_register_value(vm, src);

                runtime_struct_set(dstobj.strct, field_idx, srcval);
            }
            break;

        case OP_LOADTYPEID:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                dstval.inum = src;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_LOADADDR:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value addrval;

                addrval.inum = id_to_addr(vm, src);
                set_local(vm, dst, addrval);
            }
            break;

        case OP_DEREF:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value addrval = fetch_register_value(vm, src);
                struct runtime_value srcval = get_global(vm, addrval.inum);

                set_local(vm, dst, srcval);
            }
            break;

        /* array/struct */
        case OP_NEWARRAY:
            {
                int dst = inst.A;
                int len = inst.B;
                struct runtime_value lenval = fetch_register_value(vm, len);

                struct runtime_array *obj = runtime_array_new(lenval.inum);
                runtime_gc_push_object(&vm->gc, (struct runtime_object*) obj);

                struct runtime_value srcobj = {.array = obj};
                set_local(vm, dst, srcobj);
            }
            break;

        case OP_NEWSTRUCT:
            {
                int dst = inst.A;
                int len = inst.B;

                struct runtime_struct *obj = runtime_struct_new(len);
                runtime_gc_push_object(&vm->gc, (struct runtime_object*) obj);

                struct runtime_value srcobj = {.strct = obj};
                set_local(vm, dst, srcobj);
            }
            break;

        /* function call */
        case OP_CALL:
            {
                int ret_reg = inst.A;
                int func_id = inst.BB;

                call_function(vm, ret_reg, func_id);
            }
            break;

        case OP_CALLPOINTER:
            {
                int ret_reg = inst.A;
                int src = inst.B;
                struct runtime_value idval = fetch_register_value(vm, src);
                int func_id = idval.inum;

                call_function(vm, ret_reg, func_id);
            }
            break;

        case OP_CALLBUILTIN:
            {
                int ret_reg = inst.A;
                int func_id = inst.BB;

                /* prologue */
                int old_bp = vm->bp;
                int old_sp = vm->sp;
                int max_reg_count = 0;

                set_bp(vm, vm->bp + 1 + ret_reg - 1);
                set_sp(vm, vm->bp + max_reg_count);

                /* call */
                runtime_native_function_t native_func;
                native_func = code_get_native_function_pointer(vm->code, func_id);
                assert(native_func);

                struct runtime_value *registers = &vm->stack.data[vm->bp + 1];
                int reg_count = code_get_function_arg_count(vm->code, func_id);
                int result = 0;
                struct runtime_value ret_val = {0};

                if (code_is_function_variadic(vm->code, func_id)) {
                    struct runtime_value arg_count = fetch_register_value(vm, 0);
                    /* 2 registers for each argument and 1 register for argument count */
                    reg_count = 2 * arg_count.inum + 1;
                }

                result = native_func(&vm->gc, registers, reg_count);
                ret_val = get_local(vm, 0);

                /* epilogue */
                set_bp(vm, old_bp);
                set_sp(vm, old_sp);

                set_local(vm, ret_reg, ret_val);

                if (result == RESULT_NORETURN) {
                    /* TODO consider making push_to_stack */
                    int64_t sp_addr = index_to_addr(vm->sp);
                    write_stack(vm, sp_addr, ret_val);
                    halt = true;
                }
            }
            break;

        case OP_RETURN:
            {
                int src = inst.A;
                struct runtime_value ret_val = fetch_register_value(vm, src);
                struct vm_call call = {0};

                pop_call(vm, &call);

                int ret_reg = call.return_reg;

                set_ip(vm, call.return_ip);
                set_bp(vm, call.return_bp);
                set_sp(vm, call.return_sp);
                set_local(vm, ret_reg, ret_val);
            }
            break;

        case OP_JUMP:
            {
                int addr = inst.BB;
                set_ip(vm, addr);
            }
            break;

        case OP_JUMPIFZERO:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value testval = fetch_register_value(vm, src);

                if (testval.inum == 0)
                    set_ip(vm, dst);
            }
            break;

        case OP_JUMPIFNOTZ:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value testval = fetch_register_value(vm, src);

                if (testval.inum != 0)
                    set_ip(vm, dst);
            }
            break;

        case OP_FORNUMBEGIN:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value start = fetch_register_value(vm, src + 1);
                struct runtime_value stop = fetch_register_value(vm, src + 2);

                if (start.inum < stop.inum)
                    set_local(vm, src, start);
                else
                    set_ip(vm, dst);
            }
            break;

        case OP_FORNUMEND:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value iter = fetch_register_value(vm, src);
                struct runtime_value stop = fetch_register_value(vm, src + 2);
                struct runtime_value step = fetch_register_value(vm, src + 3);

                iter.inum += step.inum;

                if (iter.inum < stop.inum) {
                    set_local(vm, src, iter);
                    set_ip(vm, dst);
                }
            }
            break;

        case OP_FORARRAYBEGIN:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value idx = {.inum = 0};
                struct runtime_value obj = fetch_register_value(vm, src + 2);

                if (idx.inum < runtime_array_len(obj.array)) {
                    struct runtime_value val = runtime_array_get(obj.array, idx.inum);
                    set_local(vm, src + 1, val);
                    set_local(vm, src, idx);
                }
                else {
                    set_ip(vm, dst);
                }
            }
            break;

        case OP_FORARRAYEND:
            {
                int src = inst.A;
                int dst = inst.BB;
                struct runtime_value idx = fetch_register_value(vm, src);
                struct runtime_value obj = fetch_register_value(vm, src + 2);

                idx.inum++;

                if (idx.inum < runtime_array_len(obj.array)) {
                    struct runtime_value val = runtime_array_get(obj.array, idx.inum);
                    set_local(vm, src + 1, val);
                    set_local(vm, src, idx);
                    set_ip(vm, dst);
                }
            }
            break;

#define DO_BINOP(num0, num1, op, num2, zerocheck) \
do { \
    int dst = inst.A; \
    int src1 = inst.B; \
    int src2 = inst.C; \
    struct runtime_value dstval; \
    struct runtime_value srcval1 = fetch_register_value(vm, src1); \
    struct runtime_value srcval2 = fetch_register_value(vm, src2); \
    if ((zerocheck) && (srcval2.num2 == 0)) {\
        /* runtime error */ \
    } \
    dstval.num0 = srcval1.num1 op srcval2.num2; \
    set_local(vm, dst, dstval); \
} while (0)

#define BINOPI(op)  DO_BINOP(inum, inum, op, inum, false)
#define BINOPIZ(op) DO_BINOP(inum, inum, op, inum, true)
#define BINOPF(op)  DO_BINOP(fpnum, fpnum, op, fpnum, false)
#define BINOPFZ(op) DO_BINOP(fpnum, fpnum, op, fpnum, true)
#define RELOPI(op) DO_BINOP(inum, inum, op, inum, false)
#define RELOPF(op) DO_BINOP(inum, fpnum, op, fpnum, false)
#define BITOP(op) DO_BINOP(inum, inum, op, inum, false)

        /* arithmetic */
        case OP_ADDINT:
            BINOPI(+);
            break;

        case OP_ADDFLOAT:
            BINOPF(+);
            break;

        case OP_SUBINT:
            BINOPI(-);
            break;

        case OP_SUBFLOAT:
            BINOPF(-);
            break;

        case OP_MULINT:
            BINOPI(*);
            break;

        case OP_MULFLOAT:
            BINOPF(*);
            break;

        case OP_DIVINT:
            BINOPIZ(/);
            break;

        case OP_DIVFLOAT:
            BINOPFZ(/);
            break;

        case OP_REMINT:
            BINOPIZ(%);
            break;

        case OP_REMFLOAT:
            {
                int dst = inst.A;
                int src1 = inst.B;
                int src2 = inst.C;

                struct runtime_value dstval;
                struct runtime_value srcval1 = fetch_register_value(vm, src1);
                struct runtime_value srcval2 = fetch_register_value(vm, src2);

                if (srcval2.fpnum == 0) {
                    /* TODO check zero division */
                }
                dstval.fpnum = fmod(srcval1.fpnum, srcval2.fpnum);
                set_local(vm, dst, dstval);
            }
            break;

            /* TODO move this */
        case OP_CATSTRING:
            {
                int dst = inst.A;
                int src1 = inst.B;
                int src2 = inst.C;

                struct runtime_value dstval;
                struct runtime_value srcval1 = fetch_register_value(vm, src1);
                struct runtime_value srcval2 = fetch_register_value(vm, src2);

                struct runtime_string *s = runtime_string_concat(srcval1.str, srcval2.str);
                runtime_gc_push_object(&vm->gc, (struct runtime_object*) s);

                dstval.str = s;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_EQINT:
            RELOPI(==);
            break;

        case OP_EQFLOAT:
            RELOPF(==);
            break;

        case OP_NEQINT:
            RELOPI(!=);
            break;

        case OP_NEQFLOAT:
            RELOPF(!=);
            break;

        case OP_EQSTRING:
            {
                int dst = inst.A;
                int src1 = inst.B;
                int src2 = inst.C;

                struct runtime_value dstval;
                struct runtime_value srcval1 = fetch_register_value(vm, src1);
                struct runtime_value srcval2 = fetch_register_value(vm, src2);

                dstval.inum = runtime_string_compare(srcval1.str, srcval2.str) == 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_NEQSTRING:
            {
                int dst = inst.A;
                int src1 = inst.B;
                int src2 = inst.C;

                struct runtime_value dstval;
                struct runtime_value srcval1 = fetch_register_value(vm, src1);
                struct runtime_value srcval2 = fetch_register_value(vm, src2);

                dstval.inum = runtime_string_compare(srcval1.str, srcval2.str) != 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_LTINT:
            RELOPI(<);
            break;

        case OP_LTFLOAT:
            RELOPF(<);
            break;

        case OP_LTEINT:
            RELOPI(<=);
            break;

        case OP_LTEFLOAT:
            RELOPF(<=);
            break;

        case OP_GTINT:
            RELOPI(>);
            break;

        case OP_GTFLOAT:
            RELOPF(>);
            break;

        case OP_GTEINT:
            RELOPI(>=);
            break;

        case OP_GTEFLOAT:
            RELOPF(>=);
            break;

        case OP_BITWISEAND:
            BITOP(&);
            break;

        case OP_BITWISEOR:
            BITOP(|);
            break;

        case OP_BITWISEXOR:
            BITOP(^);
            break;

        case OP_BITWISENOT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);
                dstval.inum = ~srcval.inum;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_SHL:
            BITOP(<<);
            break;

        case OP_SHR:
            BITOP(>>);
            break;

        case OP_INC:
            {
                int src = inst.A;
                struct runtime_value srcval = fetch_register_value(vm, src);
                srcval.inum++;
                set_local(vm, src, srcval);
            }
            break;

        case OP_DEC:
            {
                int src = inst.A;
                struct runtime_value srcval = fetch_register_value(vm, src);
                srcval.inum--;
                set_local(vm, src, srcval);
            }
            break;

        case OP_NEGINT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);
                dstval.inum = -1 * srcval.inum;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_NEGFLOAT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);
                dstval.fpnum = -1. * srcval.fpnum;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_SETIFZERO:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);
                dstval.inum = srcval.inum == 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_SETIFNOTZ:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);
                dstval.inum = srcval.inum != 0;
                set_local(vm, dst, dstval);
            }
            break;

        /* conversion */
        case OP_BOOLTOINT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.inum = srcval.inum != 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_BOOLTOFLOAT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.fpnum = srcval.inum != 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_INTTOBOOL:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.inum = srcval.inum != 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_INTTOFLOAT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.fpnum = srcval.inum;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_FLOATTOBOOL:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.inum = srcval.fpnum != 0;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_FLOATTOINT:
            {
                int dst = inst.A;
                int src = inst.B;
                struct runtime_value dstval;
                struct runtime_value srcval = fetch_register_value(vm, src);

                dstval.inum = srcval.fpnum;
                set_local(vm, dst, dstval);
            }
            break;

        case OP_HALT:
            halt = true;
            break;

        case OP_NOP:
            break;

        default:
            fprintf(stderr, "unexpected instruction: %d\n", inst.op);
            code_print_instruction(vm->code, old_ip, &inst, NULL);
            assert(!"internal error");
            break;
        }
    }
}

static struct runtime_value make_args_value(struct runtime_gc *gc, const struct vm_args *args)
{
    struct runtime_array *array;

    array = runtime_array_new(args->count);
    runtime_gc_push_object(gc, (struct runtime_object *) array);

    for (int i = 0; i < args->count; i++) {
        struct runtime_string *str = runtime_string_new(args->values[i]);
        struct runtime_value elem = {.str = str};

        runtime_gc_push_object(gc, (struct runtime_object *) str);
        runtime_array_set(array, i, elem);
    }

    struct runtime_value val = {.array = array};
    return val;
}

void vm_execute_bytecode(struct vm_cpu *vm, const struct code_bytecode *bytecode,
        const struct vm_args *args)
{
    vm->code = bytecode;
    vm->eoc = code_get_size(vm->code);

    vm->globals = &vm->globals__;

    /* empty data at the bottom of stacks */
    struct runtime_value empty = {0};
    runtime_valuevec_resize(&vm->stack, 256);
    runtime_valuevec_set(&vm->stack, 0, empty);
    vm->sp = 0;

    /* args */
    struct runtime_value argsval = make_args_value(&vm->gc, args);
    /* TODO consider separating global area or reading the first allocate instruction
     * to detect the args slot */
    set_local(vm, 0, argsval);
    vm->sp = 1;

    vm_callstack_init(&vm->callstack);

    run_cpu(vm);
}
