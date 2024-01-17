#ifndef VM_H
#define VM_H

#include <vector>
#include "bytecode.h"
#include "gc.h"

typedef struct Value {
    union {
        Int inum = 0;
        Float fpnum;
        StringObj *str;
    };
} Value;

typedef struct Call {
    int func_index = 0;
    int argc = 0;
    Int return_ip = 0;
    Int return_bp = 0;
} Call;

typedef struct VM {
    std::vector<Value> stack_ = {{0}};
    const Bytecode *code_ = nullptr;

    Int eoc_ = 0; // end of code
    Int ip_ = 0; // instruction pointer
    Int sp_ = 0; // stack pointer
    Int bp_ = 0; // base pointer

    std::vector<Call> callstack_ = {{0}};
    int call_sp_ = 0;

    bool print_stack_ = false;
    GC gc_;
} VM;

void Run(VM *vm, const Bytecode *code);
Int StackTopInt(const VM *vm);

void PrintStack(const VM *vm);
void EnablePrintStack(VM *vm, bool enable);
void PrintObjs(const VM *vm);

#endif // _H
