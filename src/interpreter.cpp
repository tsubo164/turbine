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

    if (print_tree_) {
        tree_->Print();
        return 0;
    }

    if (scope_.GetVariableCount() > 0)
        code_.AllocateLocal(scope_.GetVariableCount());

    // Bytecode
    GenerateCode(tree_, code_);

    // Run
    //vm_.EnablePrintStack(true);
    vm_.Run(code_);

    return vm_.StackTopInt();
}

void Interpreter::EnablePrintTree(bool enable)
{
    print_tree_ = true;
}
