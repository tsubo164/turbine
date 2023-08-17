#include "interpreter.h"

Interpreter::Interpreter()
{
}

Interpreter::~Interpreter()
{
    delete tree_;
}

Int Interpreter::Run(std::istream &input)
{
    // Parse
    tree_ = parser_.ParseStream(input);

    // Bytecode
    GenerateCode(tree_, code_);

    // Run
    //vm_.EnablePrintStack(true);
    vm_.Run(code_);

    tree_->Print();

    return vm_.StackTopInt();
}
