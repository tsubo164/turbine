#ifndef INTERPRETER_H
#define INTERPRETER_H

#include <istream>
#include "bytecode.h"
#include "codegen.h"
#include "parser.h"
#include "lexer.h"
#include "scope.h"
#include "vm.h"

class Interpreter {
public:
    Interpreter();
    ~Interpreter();

    Int Run(const std::string &src);

    void EnablePrintToken(bool enable, bool raw);
    void EnablePrintTree(bool enable);
    void EnablePrintSymbols(bool enable, bool all);
    void EnablePrintBytecode(bool enable);
    void EnablePrintStack(bool enable);

private:
    Scope scope_;
    Parser parser_;

    Prog *prog_ = nullptr;
    Bytecode code_;
    VM vm_;

    bool print_token_ = false;
    bool print_token_raw_ = false;
    bool print_tree_ = false;
    bool print_symbols_ = false;
    bool print_symbols_all_ = false;
    bool print_bytecode_ = false;
    bool print_stack_ = false;
    void print_token(const std::string &src) const;
};

#endif // _H
