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
        std::stringstream input("# main\n - id int\n id = 114 \n id + 11\n");
        Interpreter ip;

        ASSERTL(125, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 42 \n 19\n");
        Interpreter ip;

        ASSERTL(19, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 12 \n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 39 + 3 \n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("# main\n - id int\n id = 0 \n id + 114\n");
        Interpreter ip;

        ASSERTL(114, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 3129 + 1293 \n");
        Interpreter ip;

        ASSERTL(4422, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 3129 + 1293+1111\n");
        Interpreter ip;

        ASSERTL(5533, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 20+22\n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("# main\n - a int\n a = 12 \n a\n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("# main\n - a int\n a = 11\n");
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 12 == 11\n");
        Interpreter ip;

        ASSERTL(0, ip.Run(input));
    }
    {
        std::stringstream input("# main\n 42 == 42\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input("# main\n - a int\n a = 39\n a == 39\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main\n"
            "    - a int\n"
            "    a = 39\n"
            "    a == 39\n"
            );

        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }

    {
        std::stringstream input(
            "# seven\n"
            "    return 7\n"
            "# main\n"
            "    return seven()\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
