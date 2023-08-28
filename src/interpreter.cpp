#include "interpreter.h"
#include "tokenizer.h"
#include <iostream>

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
        print_header("tree");
        tree_->Print();
    }

    if (print_symbols_) {
        print_header("symbol");
        scope_.Print();
    }

    // Generate bytecode
    Function *main_func = scope_.FindFunction(string_table_.Insert("main"));
    if (!main_func) {
        std::cerr << "'main' function not found" << std::endl;
        std::exit(EXIT_FAILURE);
    }
    code_.CallFunction(main_func->id);
    code_.Exit();

    GenerateCode(tree_, code_);

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

    print_header("token");

    for (;;) {
        tokenizer.Get(tok);

        if (tok->kind == TK::BlockBegin)
            indent++;
        else if (tok->kind == TK::BlockEnd)
            indent--;

        if (bol) {
            bol = false;
            for (int i = 0; i < indent; i++)
                std::cout << "....";
        }

        if (tok->kind == TK::NewLine) {
            std::cout << tok->kind << std::endl;
            bol = true;
        }
        else if (tok->kind != TK::BlockBegin && tok->kind != TK::BlockEnd) {
            std::cout << tok->kind << ' ';
        }

        if (tok->kind == TK::Eof)
            break;
    }
    std::cout << std::endl;

    return 0;
}
