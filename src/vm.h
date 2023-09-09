#ifndef VM_H
#define VM_H

#include <vector>
#include "bytecode.h"

struct Object {
    union {
        Int ival = 0;
        Float fval;
    };
};

struct Call {
    int func_index = 0;
    int argc = 0;
    Int return_ip = 0;
    Int return_bp = 0;
};

class VM {
public:
    VM() {}
    ~VM() {}

    void Run(const Bytecode &code);

    Int StackTopInt() const;
    void PrintStack() const;
    void EnablePrintStack(bool enable);

private:
    std::vector<Object> stack_ = {{0}};
    const Bytecode *code_ = nullptr;

    Int eoc_ = 0; // end of code
    Int ip_ = 0; // instruction pointer
    Int sp_ = 0; // stack pointer
    Int bp_ = 0; // base pointer

    std::vector<Call> callstack_ = {{0}};
    int call_sp_ = 0;

    bool print_stack_ = false;

    void run();

    // registers
    void set_ip(Int ip);
    void set_sp(Int sp);
    void set_bp(Int bp);

    // read byte code
    Int read_byte(Int index) const;
    Int fetch_byte();
    Int fetch_word();
    Int fetch_int();
    Float fetch_float();

    // stack
    void push(Object obj);
    Object pop();
    Object top() const;

    // stack helper
    void push_int( Int val );
    Int pop_int();
    Float pop_float();

    void push_call(Call call);
    Call pop_call();

    Object get_local(int id) const;
    Object get_global(int id) const;
    void set_local(int id, Object obj);
    void set_global(int id, Object obj);
    bool is_eoc() const;
};

#endif // _H
