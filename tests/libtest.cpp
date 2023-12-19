#include <iostream>
#include "test.h"
#include "../src/bytecode.h"
#include "../src/codegen.h"
#include "../src/parser.h"
#include "../src/lexer.h"
#include "../src/scope.h"
#include "../src/vm.h"
#include "../src/interpreter.h"

int main(int argc, char **argv)
{
    {
        std::string input("# main() int\n - id int\n id = 114 \n id + 11\n");
        Interpreter ip;

        ASSERTL(125, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 42 \n 19\n");
        Interpreter ip;

        ASSERTL(19, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 12 \n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 39 + 3 \n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        const std::string input("# main() int\n - id int\n id = 0 \n id + 114\n");
        Interpreter ip;

        ASSERTL(114, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 3129 + 1293 \n");
        Interpreter ip;

        ASSERTL(4422, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 3129 + 1293+1111\n");
        Interpreter ip;

        ASSERTL(5533, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 20+22\n");
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        const std::string input("# main() int\n - a int\n a = 12 \n a\n");
        Interpreter ip;

        ASSERTL(12, ip.Run(input));
    }
    {
        const std::string input("# main() int\n - a int\n a = 11\n");
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 12 == 11\n");
        Interpreter ip;

        ASSERTL(0, ip.Run(input));
    }
    {
        const std::string input("# main() int\n 42 == 42\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        const std::string input("# main() int  \n - a int\n a = 39\n a == 39\n");
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "    - a int\n"
            "    a = 39\n"
            "    a == 39\n"
            );

        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        const std::string input(
            "# seven() int\n"
            "    return 7\n"
            "# main() int\n"
            "    return seven()\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }
    {
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
        const std::string input(
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
    {
        const std::string input(
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 42\n"
            "        - b int\n"
            "        b = 13\n"
            "\n"
            "        if  b == 13\n"
            "            - c int\n"
            "            c = 9\n"
            "\n"
            "    //b = 4 // error\n"
            "    return 31\n"
            );
        Interpreter ip;

        ASSERTL(31, ip.Run(input));
    }
    {
        const std::string input(
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
        const std::string input(
            "- gcount int\n"
            "- gvar int\n"
            "\n"
            "# seven() int\n"
            "    gvar = 119\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "//# add int\n"
            "//    - x int\n"
            "//    - y int\n"
            "//      * test string\n"
            "//      return x + y\n"
            "\n"
            "# main() int\n"
            "    seven()\n"
            "    return gvar\n"
            "    return seven() + add(30, 5)\n"
            );
        Interpreter ip;

        ASSERTL(119, ip.Run(input));
    }
    {
        const std::string input(
            "# foo(x int) int\n"
            "    return 19\n"
            "    if x == 10\n"
            "        - y int\n"
            "        y = 23\n"
            "        x = y\n"
            "    return x + 3\n"
            "\n"
            "//# add int\n"
            "//    - x int\n"
            "//    - y int\n"
            "//      * test string\n"
            "//      return x + y\n"
            "# add (x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return foo(10) + add(20 + 3)\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        const std::string input(
            "## Point\n"
            "  - x int\n"
            "  - y int\n"
            "\n"
            "- pt Point\n"
            "\n"
            "# add(a int, b int) int\n"
            "  return a + b\n"
            "\n"
            "# main() int\n"
            "  - a int\n"
            "  pt.x = 2\n"
            "  pt.y = 3\n"
            "  a = pt.y\n"
            "  return pt.x + pt.y\n"
            );
        Interpreter ip;

        ASSERTL(5, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  - f float\n"
            "  f = 3.14\n"
            "  if f == 3.14\n"
            "    return 1\n"
            "  else\n"
            "    return 0\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  return 0xF + 0Xa\n"
            );
        Interpreter ip;

        ASSERTL(25, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  - f float\n"
            "  - g float\n"
            "  f = 3.14\n"
            "  g = 0.86\n"
            "  if f + g == 4.0\n"
            "    return 1\n"
            "  else\n"
            "    return 0\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  if 13 == 13\n"
            "    i = 42\n"
            "  else\n"
            "    i = 99\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  - s0 string\n"
            "  - s1 string\n"
            "  - str string\n"
            "  s0 = \"Hello, \"\n"
            "  s1 = \"World!\"\n"
            "  str = s0 + s1\n"
            "  if str == \"Hello, World!\"\n"
            "    return 42\n"
            "  else\n"
            "    return 0\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  if 42 != 42\n"
            "    i = 0\n"
            "  else\n"
            "    i = 11\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(11, ip.Run(input));
    }
    {
        // '-' operator and order of eval args
        const std::string input(
            "# sub(x int, y int) int\n"
            "    return x - y\n"
            "# main() int\n"
            "    return sub(12, 7)\n"
            );
        Interpreter ip;

        ASSERTL(5, ip.Run(input));
    }
    {
        // '*' operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 12\n"
            "    j = 3\n"
            "    return 46 - i * j\n"
            );
        Interpreter ip;

        ASSERTL(10, ip.Run(input));
    }
    {
        // '/' operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 12\n"
            "    j = 3\n"
            "    return 46 - i / j\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        // '%' operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 19\n"
            "    j = 7\n"
            "    return 46 - i % j\n"
            );
        Interpreter ip;

        ASSERTL(41, ip.Run(input));
    }
    {
        // '(' expr ')'
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 19\n"
            "    j = 17\n"
            "    return 21 * (i - j)\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        // "||" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 7\n"
            "    return i || j\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // "||" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 0\n"
            "    return i || j\n"
            );
        Interpreter ip;

        ASSERTL(0, ip.Run(input) != 0);
    }
    {
        // "&&" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 7\n"
            "    return i && j\n"
            );
        Interpreter ip;

        ASSERTL(0, ip.Run(input) != 0);
    }
    {
        // "&&" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 1\n"
            "    j = 7\n"
            "    return i && j\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // "+" unary operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 7\n"
            "    return +i\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }
    {
        // "-" unary operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return -i\n"
            );
        Interpreter ip;

        ASSERTL(-42, ip.Run(input));
    }
    {
        // "-+" unary operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return -+-+-+- -+-+ +-i\n"
            );
        Interpreter ip;

        ASSERTL(-42, ip.Run(input));
    }
    {
        // "!" unary operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return !(42 != i)\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // "!" unary operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return !i\n"
            );
        Interpreter ip;

        ASSERTL(0, ip.Run(input) != 0);
    }
    {
        // "<" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return i < 5\n"
            );
        Interpreter ip;

        ASSERTL(0, ip.Run(input) != 0);
    }
    {
        // "<=" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return i <= 42\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // ">" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return i > 5\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // ">=" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return i >= 42\n"
            );
        Interpreter ip;

        ASSERTL(1, ip.Run(input) != 0);
    }
    {
        // "++" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    i++\n"
            "    return i\n"
            );
        Interpreter ip;

        ASSERTL(43, ip.Run(input));
    }
    {
        // "--" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    i--\n"
            "    return i\n"
            );
        Interpreter ip;

        ASSERTL(41, ip.Run(input));
    }
    {
        // "&" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x3A9\n"
            "    return i & 0xFF\n"
            );
        Interpreter ip;

        ASSERTL(0xA9, ip.Run(input));
    }
    {
        // "|" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x3A9\n"
            "    return i | 0xFFF\n"
            );
        Interpreter ip;

        ASSERTL(0xFFF, ip.Run(input));
    }
    {
        // "~" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8\n"
            "    return ~i & 0xF\n"
            );
        Interpreter ip;

        ASSERTL(0x7, ip.Run(input));
    }
    {
        // "^" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x78\n"
            "    return i ^ 0xF0\n"
            );
        Interpreter ip;

        ASSERTL(0x88, ip.Run(input));
    }
    {
        // "<<" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8\n"
            "    return i << 3\n"
            );
        Interpreter ip;

        ASSERTL(0x40, ip.Run(input));
    }
    {
        // ">>" operator
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8F\n"
            "    return i >> 4\n"
            );
        Interpreter ip;

        ASSERTL(0x08, ip.Run(input));
    }
    {
        // "for" statment
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    j = 0\n"
            "    for i = 0; i < 10; i++\n"
            "        j = j + 2\n"
            "    return j\n"
            );
        Interpreter ip;

        ASSERTL(20, ip.Run(input));
    }
    {
        // "for" statment while style
        const std::string input(
            "# main() int\n"
            "    - i int\n"
            "    i = 0\n"
            "    for i < 10\n"
            "        i++\n"
            "    return i\n"
            );
        Interpreter ip;

        ASSERTL(10, ip.Run(input));
    }
    {
        // "for" statment infinite loop
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  i = 0\n"
            "  for\n"
            "    i++\n"
            "    if i == 8\n"
            "      break\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(8, ip.Run(input));
    }
    {
        // "break" statment
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  for i = 0; i < 10; i++\n"
            "    if i == 5\n"
            "      break\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(5, ip.Run(input));
    }
    {
        // "continue" statment
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  - j int\n"
            "  j = 0\n"
            "  for i = 0; i < 10; i++\n"
            "    if i % 2 == 0\n"
            "      continue\n"
            "    j++\n"
            "  return j\n"
            );
        Interpreter ip;

        ASSERTL(5, ip.Run(input));
    }
    {
        // "switch" statment
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  - j int\n"
            "  i = 2\n"
            "  j = 0\n"
            "  switch i\n"
            "  case 0\n"
            "    j = 0\n"
            "  case 1\n"
            "    j = 23\n"
            "  case 2\n"
            "    j = 34\n"
            "  case 3\n"
            "    j = 77\n"
            "  return  j\n"
            );
        Interpreter ip;

        ASSERTL(34, ip.Run(input));
    }
    {
        // "default" statment
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  - j int\n"
            "  i = 5\n"
            "  j = 0\n"
            "  switch i\n"
            "  case 0\n"
            "    j = 0\n"
            "  case 1\n"
            "    j = 23\n"
            "  case 2\n"
            "    j = 34\n"
            "  case 3\n"
            "    j = 77\n"
            "  default\n"
            "    j = 99\n"
            "  return  j\n"
            );
        Interpreter ip;

        ASSERTL(99, ip.Run(input));
    }
    {
        // local var init
        const std::string input(
            "# main() int\n"
            "  - i int = 42\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        // global var init
        const std::string input(
            "- g int = 39\n"
            "# main() int\n"
            "  return g\n"
            );
        Interpreter ip;

        ASSERTL(39, ip.Run(input));
    }
    {
        // local var type
        const std::string input(
            "# main() int\n"
            "  - i = 41\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(41, ip.Run(input));
    }
    {
        // local var type
        const std::string input(
            "# main() int\n"
            "  - i = 41\n"
            "  - f = 3.1415\n"
            "  - g float = 3.1\n"
            "  if f == g + 0.0415\n"
            "    i = 9\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(9, ip.Run(input));
    }
    {
        // "+=" operator
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  i += 4\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(46, ip.Run(input));
    }
    {
        // "-=" operator
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  i -= 4\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(38, ip.Run(input));
    }
    {
        // "*=" operator
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  i *= 4\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(168, ip.Run(input));
    }
    {
        // "/=" operator
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  i /= 4\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(10, ip.Run(input));
    }
    {
        // "%=" operator
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  i %= 4\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(2, ip.Run(input));
    }
    {
        // bool type
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  - b = true\n"
            "  if b\n"
            "    i = 19\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(19, ip.Run(input));
    }
    {
        // bool type
        const std::string input(
            "# main() int\n"
            "  - i = 42\n"
            "  - b = true\n"
            "  - c = false\n"
            "  if b != !c\n"
            "    i = 19\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        // nop statement
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  for i = 0; i < 7; i++\n"
            "    nop\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(7, ip.Run(input));
    }
    {
        // block comment
        const std::string input(
            "# main() int\n"
            "  - i = 42   // int\n"
            "  /*\n"
            "    this is a block comment\n"
            "    the first indent has to match.\n"
            "  /*\n"
            "      nested block comment.\n"
            "  */\n"
            "  this is another line in block comment.\n"
            "  */\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(42, ip.Run(input));
    }
    {
        // char literal
        const std::string input(
            "# main() int\n"
            "  - i = 'a'\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(97, ip.Run(input));
    }
    {
        // char literal
        const std::string input(
            "# main() int\n"
            "  - i = '\n'\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(10, ip.Run(input));
    }
    {
        // slash at the end of string literal
        const std::string input(
            "# main() int\n"
            "  - i int\n"
            "  - s = \"Hello\\\\\"\n"
            "  if s == \"Hello\\\\\"\n"
            "    i = 13\n"
            "  return i\n"
            );
        Interpreter ip;

        ASSERTL(13, ip.Run(input));
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
