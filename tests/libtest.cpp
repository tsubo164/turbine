#include <iostream>
#include <sstream>
#include "test.h"
#include "../src/tokenizer.h"
#include "../src/bytecode.h"
#include "../src/codegen.h"
#include "../src/parser.h"
#include "../src/scope.h"
#include "../src/vm.h"
#include "../src/interpreter.h"

int main(int argc, char **argv)
{
    {
        std::stringstream input("  id = 114 \n  id + 11");
        Interpreter ip;

        ASSERTL(125, ip.Run(input));
    }
    {
        std::stringstream input(" 42 \n 19\n");
        Interpreter ip;

        ASSERTL(19, ip.Run(input));
    }
    {
        std::stringstream input(" 12 ");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("  39 + 3 ");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("  id = 0 \n  id + 114");
        Interpreter ip;

        ASSERTL(114, ip.Run(input));
    }
    {
        std::stringstream input("  3129 + 1293 ");
        Interpreter ip;

        ASSERTL(4422, ip.Run(input));
    }
    {
        std::stringstream input("  3129 + 1293+1111");
        Interpreter ip;

        ASSERTL(5533, ip.Run(input));
    }
    {
        std::stringstream input("20+22");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("a = 12 \n a");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("a = 11");
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
