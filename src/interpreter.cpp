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

    if (scope_.GetVariableCount() > 0)
        code_.AllocateLocal(scope_.GetVariableCount());

    // Bytecode
    GenerateCode(tree_, code_);

    // Run
    //vm_.EnablePrintStack(true);
    vm_.Run(code_);

    //tree_->Print();

    return vm_.StackTopInt();
}
