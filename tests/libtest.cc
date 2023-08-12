#include <iostream>
#include <sstream>
#include "../src/tokenizer.h"

int main(int argc, char **argv)
{
    std::stringstream strm("  42 ");
    Tokenizer toknizer;
    Token tok;

    toknizer.SetInput(strm);
    toknizer.Get(tok);

    if (tok.ival != 42) {
        printf("\033[0;31mNG\033[0;39m\n");
        return -1;
    }

    return 0;
}
