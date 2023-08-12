#include <iostream>
#include <sstream>
#include "test.h"
#include "../src/tokenizer.h"
#include "../src/parser.h"

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
        std::stringstream strm("  12 ");
        Parser parser;

        const Node *tree = parser.ParseStream(strm);

        ASSERTL(12, tree->ival);
    }
    {
        std::stringstream strm("  39 + 3 ");
        Parser parser;

        const Node *tree = parser.ParseStream(strm);

        ASSERTI(NOD_ADD, tree->kind);
        ASSERTL(0, tree->ival);
        ASSERTL(39, tree->lhs->ival);
        ASSERTL(3, tree->rhs->ival);
    }


    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
