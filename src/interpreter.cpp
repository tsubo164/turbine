#include "interpreter.h"
#include "tokenizer.h"
#include <iostream>

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
    delete tree_;
}

Int Interpreter::Run(std::istream &stream)
{
    // Print token
    if (print_token_) {
        return print_token(stream);
    }

    // Compile source
    tree_ = parser_.ParseStream(stream);

    if (print_tree_) {
        std::cout << "### tree" << std::endl;
        tree_->Print();
    }

    if (print_symbols_) {
        std::cout << "### symbols" << std::endl;
        scope_.Print();
    }

    // Generate bytecode
    code_.CallFunction(string_table_.Insert("main"));
    code_.Exit();

    GenerateCode(tree_, code_);

    if (print_bytecode_) {
        std::cout << "### bytecode" << std::endl;
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

void Interpreter::EnablePrintToken(bool enable)
{
    print_token_ = enable;
}

void Interpreter::EnablePrintTree(bool enable)
{
    print_tree_ = enable;
}

void Interpreter::EnablePrintSymbols(bool enable)
{
    print_symbols_ = enable;
}

void Interpreter::EnablePrintBytecode(bool enable)
{
    print_bytecode_ = enable;
}

void Interpreter::EnablePrintStack(bool enable)
{
    print_stack_ = enable;
}

int Interpreter::print_token(std::istream &stream) const
{
    StringTable string_table;
    Tokenizer tokenizer(string_table);

    tokenizer.SetInput(stream);
    Token token;
    Token *tok = &token;
    int indent = 0;
    bool bol = true;

    std::cout << "## token" << std::endl;

    for (;;) {
        tokenizer.Get(tok);

        if (tok->kind == TK::BlockBegin)
            indent++;
        else if (tok->kind == TK::BlockEnd)
            indent--;
        else
            std::cout << tok->kind;

        if (bol) {
            bol = false;
            for (int i = 0; i < indent; i++)
                std::cout << "....";
        }

        if (tok->kind == TK::NewLine) {
            std::cout << std::endl;
            bol = true;
        }
        else if (tok->kind != TK::BlockBegin && tok->kind != TK::BlockEnd) {
            std::cout << ' ';
        }

        if (tok->kind == TK::Eof)
            break;
    }
    std::cout << std::endl;

    return 0;
}
