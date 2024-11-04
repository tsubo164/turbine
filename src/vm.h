#ifndef VM_H
#define VM_H

#include "bytecode.h"
#include "gc.h"
// TODO remove this
#include "objarray.h"
#include "runtime_struct.h"

typedef struct Call {
    int func_index;
    int argc;
    Int return_ip;
    Int return_bp;
    Int return_sp;
    int return_reg;
} Call;

typedef struct CallVec {
    Call *data;
    int cap;
    int len;
} CallVec;

typedef struct VM {
    struct runtime_valuevec stack_;
    const Bytecode *code_;

    // XXX TEST
    Int eoc; // end of code
    Int eoc_; // end of code
    Int ip_; // instruction pointer
    Int sp_; // stack pointer
    Int bp_; // base pointer

    CallVec callstack_;
    int call_sp_;

    bool print_stack_;
    GC gc_;
} VM;

void Run(VM *vm, const Bytecode *code);
Int StackTopInt(const VM *vm);

void PrintStack(const VM *vm);
void EnablePrintStack(VM *vm, bool enable);
void PrintObjs(const VM *vm);

#endif // _H
