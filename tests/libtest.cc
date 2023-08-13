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
        Tokenizer toknizer;
        Token tok;

        toknizer.SetInput(strm);

        toknizer.Get(tok);
        ASSERTL(42, tok.ival);
    }
    {
        std::stringstream strm(" +  19  ");
        Tokenizer toknizer;
        Token tok;

        toknizer.SetInput(strm);

        toknizer.Get(tok);
        ASSERTL(0, tok.ival);
        ASSERTI(TOK_PLUS, tok.kind);

        toknizer.Get(tok);
        ASSERTL(19, tok.ival);
    }
    {
        std::stringstream strm(" foo  \n if");
        StringTable string_table;
        Tokenizer toknizer;
        Token tok;

        toknizer.SetInput(strm);
        toknizer.SetStringTable(string_table);

        toknizer.Get(tok);
        ASSERTI(TOK_IDENT, tok.kind);
        ASSERTS("foo", string_table.Lookup(tok.str_id));

        toknizer.Get(tok);
        ASSERTL(TOK_IF, tok.kind);
    }
    {
        std::stringstream strm("  12 ");
        Parser parser;

        Node *tree = parser.ParseStream(strm);

        ASSERTL(12, tree->ival);

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  39 + 3 ");
        Parser parser;

        Node *tree = parser.ParseStream(strm);

        ASSERTI(NOD_ADD, tree->kind);
        ASSERTL(0, tree->ival);
        ASSERTL(39, tree->lhs->ival);
        ASSERTL(3, tree->rhs->ival);

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  3129 + 1293 ");
        Parser parser;

        Node *tree = parser.ParseStream(strm);

        ASSERTL(4422, EvalTree(tree));

        DeleteTree(tree);
    }
    {
        std::stringstream strm("  3129 + 1293+1111");
        Parser parser;

        Node *tree = parser.ParseStream(strm);

        ASSERTL(5533, EvalTree(tree));

        DeleteTree(tree);
    }
    {
        std::stringstream strm("20+22");
        Parser parser;

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
        Parser parser;

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
