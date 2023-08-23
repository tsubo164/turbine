#include "interpreter.h"

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
    delete tree_;
}

Int Interpreter::Run(std::istream &stream)
{
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
