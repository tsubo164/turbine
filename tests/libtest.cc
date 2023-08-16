#include <iostream>
#include <sstream>
#include "test.h"
#include "../src/tokenizer.h"
#include "../src/bytecode.h"
#include "../src/codegen.h"
#include "../src/parser.h"
#include "../src/vm.h"
#include "../src/mds.h"

int run_text(const char *code)
{
    std::stringstream input("  3129 + 1293 ");
    MDS mds;

    return mds.Run(input);
}

int main(int argc, char **argv)
{
    {
        std::stringstream input(" 42 ");
        MDS mds;

        const int ret = mds.Run(input);

        ASSERTL(42, ret);
    }
    {
        std::stringstream input(" 12 ");
        MDS mds;

        const int ret = mds.Run(input);

        ASSERTL(12, ret);
    }
    {
        std::stringstream input("  39 + 3 ");
        MDS mds;

        const int ret = mds.Run(input);

        ASSERTL(42, ret);
    }
    {
        std::stringstream input("  id + 114 ");
        MDS mds;

        const int ret = mds.Run(input);

        ASSERTL(114, ret);
    }
    {
        std::stringstream strm("  3129 + 1293 ");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(4422, tree->Eval());

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  3129 + 1293+1111");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(5533, tree->Eval());

        DeleteTree(tree);
    }
    {
        std::stringstream strm("a = 12");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(12, tree->Eval());

        DeleteTree(tree);
    }
    {
        std::stringstream strm("20+22");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);
        Bytecode code;
        GenerateCode(tree, code);

        ASSERTI(6, code.Size());

        ASSERTI(OP_LOADB, code.Read(0));
        ASSERTI(20, code.Read(1));
        ASSERTI(OP_LOADB, code.Read(2));
        ASSERTI(22, code.Read(3));
        ASSERTI(OP_ADD, code.Read(4));
        ASSERTI(OP_EOC, code.Read(5));

        DeleteTree(tree);
    }
    {
        std::stringstream strm("20+22");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);
        Bytecode code;
        GenerateCode(tree, code);

        VM vm;
        vm.Run(code);

        ASSERTL(42, vm.StackTopInt());

        DeleteTree(tree);
    }
    {
        std::stringstream strm(" a =   11");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);
        Bytecode code;

        code.AllocateLocal(1); // XXX TMP
        GenerateCode(tree, code);
        code.Print();

        VM vm;
        vm.EnablePrintStack(true);
        vm.Run(code);

        ASSERTL(11, vm.StackTopInt());

        DeleteTree(tree);
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
