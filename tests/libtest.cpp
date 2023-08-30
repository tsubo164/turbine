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
        std::stringstream input("# main() int\n - id int\n id = 114 \n id + 11\n");
        Interpreter ip;

        ASSERTL(125, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 42 \n 19\n");
        Interpreter ip;

        ASSERTL(19, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 12 \n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 39 + 3 \n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n - id int\n id = 0 \n id + 114\n");
        Interpreter ip;

        ASSERTL(114, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 3129 + 1293 \n");
        Interpreter ip;

        ASSERTL(4422, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 3129 + 1293+1111\n");
        Interpreter ip;

        ASSERTL(5533, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 20+22\n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n - a int\n a = 12 \n a\n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n - a int\n a = 11\n");
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 12 == 11\n");
        Interpreter ip;

        ASSERTL(0, ip.Run(input));
    }
    {
        std::stringstream input("# main() int\n 42 == 42\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input("# main() int  \n - a int\n a = 39\n a == 39\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main() int\n"
            "    - a int\n"
            "    a = 39\n"
            "    a == 39\n"
            );

        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input(
            "# seven() int\n"
            "    return 7\n"
            "# main() int\n"
            "    return seven()\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }
    {
        std::stringstream input(
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return seven() + 35\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input(
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return seven() + add(30, 5)\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 12\n"
            "        return 11\n"
            "    return 22\n"
            );
        Interpreter ip;

        ASSERTL(22, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 42\n"
            "        return 11\n"
            "    return 22\n"
            );
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 42\n"
            "        return 1\n"
            "    else\n"
            "        return 0\n"
            "    return 33\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input(
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 41\n"
            "        return 1\n"
            "    else\n"
            "        return 0\n"
            "    return 33\n"
            );
        Interpreter ip;

        ASSERTL(0, ip.Run(input));
    }
    {
        std::stringstream input(
            "// if statement\n"
            "// line comment at beginning of line\n"
            "\n"
            "# main() int\n"
            "    - a int\n"
            "  // comment with incorrect indetation\n"
            "    a = 42 // comment after vaid statement\n"
            "    if a == 42\n"
            "        return 1\n"
            "    else\n"
            "        return 0\n"
            "    // comment with the same indetation\n"
            "    return 33\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        std::stringstream input(
            "// if statement\n"
            "// line comment at beginning of line\n"
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# main() int\n"
            "    - a int\n"
            "  // comment with incorrect indetation\n"
            "    a = 42 // comment after vaid statement\n"
            "    if a == 42\n"
            "        return seven()\n"
            "    else\n"
            "        return 0\n"
            "    // comment with the same indetation\n"
            "    return 33\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }
    {
        std::stringstream input(
            "# upper(s string) string\n"
            "    return s\n"
            "\n"
            "# main() int\n"
            "    - s string\n"
            "    return 33\n"
            );
        Interpreter ip;

        ASSERTL(33, ip.Run(input));
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
