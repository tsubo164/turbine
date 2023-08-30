#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <istream>
#include "tokenizer.h"
#include "bytecode.h"
#include "codegen.h"
#include "parser.h"
#include "scope.h"
#include "vm.h"

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    Int Run(std::istream &stream);

    void EnablePrintToken(bool enable, bool raw);
    void EnablePrintTree(bool enable);
    void EnablePrintSymbols(bool enable);
    void EnablePrintBytecode(bool enable);
    void EnablePrintStack(bool enable);

private:
    StringTable string_table_;
    Scope scope_;
    Parser parser_ = {string_table_, scope_};

    Node *tree_ = nullptr;
    Bytecode code_;
    VM vm_;

    bool print_token_ = false;
    bool print_token_raw_ = false;
    bool print_tree_ = false;
    bool print_symbols_ = false;
    bool print_bytecode_ = false;
    bool print_stack_ = false;
    int print_token(std::istream &stream) const;
};

#endif // _H
