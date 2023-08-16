#include <iostream>
#include <sstream>
#include "test.h"
#include "../src/tokenizer.h"
#include "../src/bytecode.h"
#include "../src/codegen.h"
#include "../src/parser.h"
#include "../src/vm.h"

int main(int argc, char **argv)
{
    {
        std::stringstream strm("  42 ");
        StringTable string_table;
        Tokenizer toknizer(string_table);
        Token tok;

        toknizer.SetInput(strm);

        toknizer.Get(tok);
        ASSERTL(42, tok.ival);
    }
    {
        std::stringstream strm(" +  19  =");
        StringTable string_table;
        Tokenizer toknizer(string_table);
        Token tok;

        toknizer.SetInput(strm);

        toknizer.Get(tok);
        ASSERTL(0, tok.ival);
        ASSERTK(TK::Plus, tok.kind);

        toknizer.Get(tok);
        ASSERTL(19, tok.ival);

        toknizer.Get(tok);
        ASSERTK(TK::Equal, tok.kind);
    }
    {
        std::stringstream strm(" foo  \n if");
        StringTable string_table;
        Tokenizer toknizer(string_table);
        Token tok;

        toknizer.SetInput(strm);

        toknizer.Get(tok);
        ASSERTK(TK::Ident, tok.kind);
        ASSERTS("foo", string_table.Lookup(tok.str_id));

        toknizer.Get(tok);
        ASSERTK(TK::If, tok.kind);
    }
    {
        std::stringstream strm("  12 ");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(12, tree->Eval());

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  39 + 3 ");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(42, tree->Eval());

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  id + 114 ");
        StringTable string_table;
        Parser parser(string_table);

        Node *tree = parser.ParseStream(strm);

        ASSERTL(114, tree->Eval());

        DeleteTree(tree);
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
        tree->Print();

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

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
