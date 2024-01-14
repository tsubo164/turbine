#include "interpreter.h"
#include "compiler.h"
#include "builtin.h"
#include "token.h"
#include "ast.h"
#include <iostream>
#include <iomanip>

static void print_header(const char *title)
{
    std::cout << "## " << title << std::endl;
    std::cout << "---" << std::endl;
}

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
    delete prog_;
}

Int Interpreter::Run(const std::string &src)
{
    // Builtin functions
    DefineBuiltinFuncs(&scope_);

    // Tokenize
    const Token *tok = Tokenize(src.c_str());

    // Print token
    if (print_token_) {
        PrintToken(tok, !print_token_raw_);
        if (!print_tree_ && !print_symbols_ && !print_bytecode_) {
            return 0;
        }
    }

    // Compile source
    prog_ = Parse(src.c_str(), tok, &scope_);

    if (print_tree_) {
        print_header("tree");
        PrintProg(prog_, 0);
    }

    if (print_symbols_) {
        print_header("symbol");
        if (print_symbols_all_)
            scope_.Print();
        else
            prog_->scope->Print();
    }

    // Generate bytecode
    SetOptimize(enable_optimize_);
    GenerateCode(&code_, prog_);

    if (print_bytecode_) {
        print_header("bytecode");
        code_.Print();
    }

    // Run bytecode
    long ret = 0;
    if (!print_tree_ && !print_symbols_ && !print_bytecode_) {
        vm_.EnablePrintStack(print_stack_);
        vm_.Run(code_);
        ret = vm_.StackTopInt();
    }

    return ret;
}

void Interpreter::EnablePrintToken(bool enable, bool raw)
{
    print_token_ = enable;
    print_token_raw_ = raw;
}

void Interpreter::EnablePrintTree(bool enable)
{
    print_tree_ = enable;
}

void Interpreter::EnablePrintSymbols(bool enable, bool all)
{
    print_symbols_ = enable;
    print_symbols_all_ = all;
}

void Interpreter::EnablePrintBytecode(bool enable)
{
    print_bytecode_ = enable;
}

void Interpreter::EnablePrintStack(bool enable)
{
    print_stack_ = enable;
}

void Interpreter::EnableOptimize(bool enable)
{
    enable_optimize_ = enable;
}
