#include <stdio.h>
#include "test.h"
#include "../src/interpreter.h"

int main(int argc, char **argv)
{
    struct interpreter_option opt = {0};
    struct interpreter_args args = {0};
    args.filename = "libtest.c";

    {
        const char *input = "# main() int\n - id int\n id = 114 \n return id + 11\n";

        ASSERTL(125, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n 42 \n return 19\n";

        ASSERTL(19, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return 12 \n";

        ASSERTL(12, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return 39 + 3 \n";

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n - id int\n id = 0 \n return id + 114\n";

        ASSERTL(114, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return 3129 + 1293 \n";

        ASSERTL(4422, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return 3129 + 1293+1111\n";

        ASSERTL(5533, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return 20+22\n";

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n - a int\n a = 12 \n return a\n";

        ASSERTL(12, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n - a int\n a = 11\n return a\n";

        ASSERTL(11, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return int(12 == 11)\n";

        ASSERTL(0, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int\n return int(42 == 42)\n";

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = "# main() int  \n - a int\n a = 39\n return int(a == 39)\n";

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "    - a int\n"
            "    a = 39\n"
            "    return int(a == 39)\n"
            ;


        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# seven() int\n"
            "    return 7\n"
            "# main() int\n"
            "    return seven()\n"
            ;

        ASSERTL(7, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return seven() + 35\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return seven() + add(30, 5)\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 12\n"
            "        return 11\n"
            "    return 22\n"
            ;

        ASSERTL(22, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 42\n"
            "        return 11\n"
            "    return 22\n"
            ;

        ASSERTL(11, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 42\n"
            "        return 1\n"
            "    else\n"
            "        return 0\n"
            "    return 33\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "    - a int\n"
            "    a = 42\n"
            "    if a == 41\n"
            "        return 1\n"
            "    else\n"
            "        return 0\n"
            "    return 33\n"
            ;

        ASSERTL(0, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
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
            ;

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
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
            ;

        ASSERTL(7, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# upper(s string) string\n"
            "    return s\n"
            "\n"
            "# main() int\n"
            "    - s string\n"
            "    return 33\n"
            ;

        ASSERTL(33, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
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
            ;

        ASSERTL(31, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# seven() int\n"
            "    return 7\n"
            "\n"
            "# add(x int, y int) int\n"
            "    return x + y\n"
            "\n"
            "# main() int\n"
            "    return seven() + add(30, 5)\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "- _count_ int\n"
            "- _var_ int\n"
            "\n"
            "# seven() int\n"
            "    _var_ = 119\n"
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
            "    return _var_\n"
            "    return seven() + add(30, 5)\n"
            ;

        ASSERTL(119, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# foo(x int) int\n"
            "    return 19\n"
            "    - xx int\n"
            "    if x == 10\n"
            "        - y int\n"
            "        y = 23\n"
            "        xx = y\n"
            "    return xx + 3\n"
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
            "    return foo(10) + add(20, 3)\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "## Point struct\n"
            "  - x int\n"
            "  - y int\n"
            "\n"
            "- _pt_ Point\n"
            "\n"
            "# add(a int, b int) int\n"
            "  return a + b\n"
            "\n"
            "# main() int\n"
            "  - a int\n"
            "  _pt_.x = 2\n"
            "  _pt_.y = 3\n"
            "  a = _pt_.y\n"
            "  return _pt_.x + _pt_.y\n"
            ;

        ASSERTL(5, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "  - f float\n"
            "  f = 3.14\n"
            "  if f == 3.14\n"
            "    return 1\n"
            "  else\n"
            "    return 0\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "  return 0xF + 0Xa\n"
            ;

        ASSERTL(25, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "  - f float\n"
            "  - g float\n"
            "  f = 3.14\n"
            "  g = 0.86\n"
            "  if f + g == 4.0\n"
            "    return 1\n"
            "  else\n"
            "    return 0\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  if 13 == 13\n"
            "    i = 42\n"
            "  else\n"
            "    i = 99\n"
            "  return i\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
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
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  if 42 != 42\n"
            "    i = 0\n"
            "  else\n"
            "    i = 11\n"
            "  return i\n"
            ;

        ASSERTL(11, interpret_source(input, &args, &opt));
    }
    {
        /* '-' operator and order of eval args */
        const char *input = 
            "# sub(x int, y int) int\n"
            "    return x - y\n"
            "# main() int\n"
            "    return sub(12, 7)\n"
            ;

        ASSERTL(5, interpret_source(input, &args, &opt));
    }
    {
        /* '*' operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 12\n"
            "    j = 3\n"
            "    return 46 - i * j\n"
            ;

        ASSERTL(10, interpret_source(input, &args, &opt));
    }
    {
        /* '/' operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 12\n"
            "    j = 3\n"
            "    return 46 - i / j\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        /* '%' operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 19\n"
            "    j = 7\n"
            "    return 46 - i % j\n"
            ;

        ASSERTL(41, interpret_source(input, &args, &opt));
    }
    {
        /* '(' expr ')' */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 19\n"
            "    j = 17\n"
            "    return 21 * (i - j)\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        /* "||" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 7\n"
            "    return i || j\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "||" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 0\n"
            "    return i || j\n"
            ;

        ASSERTL(0, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "&&" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 0\n"
            "    j = 7\n"
            "    return i && j\n"
            ;

        ASSERTL(0, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "&&" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    i = 1\n"
            "    j = 7\n"
            "    return i && j\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "+" unary operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 7\n"
            "    return +i\n"
            ;

        ASSERTL(7, interpret_source(input, &args, &opt));
    }
    {
        /* "-" unary operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return -i\n"
            ;

        ASSERTL(-42, interpret_source(input, &args, &opt));
    }
    {
        /* "-+" unary operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return -+-+-+- -+-+ +-i\n"
            ;

        ASSERTL(-42, interpret_source(input, &args, &opt));
    }
    {
        /* "!" unary operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return int(!(42 != i))\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "!" unary operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return !i\n"
            ;

        ASSERTL(0, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "<" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return int(i < 5)\n"
            ;

        ASSERTL(0, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "<=" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return int(i <= 42)\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* ">" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return int(i > 5)\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* ">=" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    return int(i >= 42)\n"
            ;

        ASSERTL(1, interpret_source(input, &args, &opt) != 0);
    }
    {
        /* "+= 1" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    i += 1\n"
            "    return i\n"
            ;

        ASSERTL(43, interpret_source(input, &args, &opt));
    }
    {
        /* "-= 1" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 42\n"
            "    i -= 1\n"
            "    return i\n"
            ;

        ASSERTL(41, interpret_source(input, &args, &opt));
    }
    {
        /* "&" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x3A9\n"
            "    return i & 0xFF\n"
            ;

        ASSERTL(0xA9, interpret_source(input, &args, &opt));
    }
    {
        /* "|" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x3A9\n"
            "    return i | 0xFFF\n"
            ;

        ASSERTL(0xFFF, interpret_source(input, &args, &opt));
    }
    {
        /* "~" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8\n"
            "    return ~i & 0xF\n"
            ;

        ASSERTL(0x7, interpret_source(input, &args, &opt));
    }
    {
        /* "^" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x78\n"
            "    return i ^ 0xF0\n"
            ;

        ASSERTL(0x88, interpret_source(input, &args, &opt));
    }
    {
        /* "<<" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8\n"
            "    return i << 3\n"
            ;

        ASSERTL(0x40, interpret_source(input, &args, &opt));
    }
    {
        /* ">>" operator */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0x8F\n"
            "    return i >> 4\n"
            ;

        ASSERTL(0x08, interpret_source(input, &args, &opt));
    }
    {
        /* "for" statment */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    - j int\n"
            "    j = 0\n"
            "    for i in 0..10\n"
            "        j = j + 2\n"
            "    return j\n"
            ;

        ASSERTL(20, interpret_source(input, &args, &opt));
    }
    {
        /* "while" statment */
        const char *input = 
            "# main() int\n"
            "    - i int\n"
            "    i = 0\n"
            "    while i < 10\n"
            "        i += 1\n"
            "    return i\n"
            ;

        ASSERTL(10, interpret_source(input, &args, &opt));
    }
    {
        /* "while" statment infinite loop */
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  i = 0\n"
            "  while true\n"
            "    i += 1\n"
            "    if i == 8\n"
            "      break\n"
            "  return i\n"
            ;

        ASSERTL(8, interpret_source(input, &args, &opt));
    }
    {
        /* "break" statment */
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  for j in 0..10\n"
            "    if j == 5\n"
            "      break\n"
            "    i += 1\n"
            "  return i\n"
            ;

        ASSERTL(5, interpret_source(input, &args, &opt));
    }
    {
        /* "continue" statment */
        const char *input = 
            "# main() int\n"
            "  - j int\n"
            "  j = 0\n"
            "  for i in 0..10\n"
            "    if i % 2 == 0\n"
            "      continue\n"
            "    j += 1\n"
            "  return j\n"
            ;

        ASSERTL(5, interpret_source(input, &args, &opt));
    }
    {
        /* "switch" statment */
        const char *input = 
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
            "  return j\n"
            ;

        ASSERTL(34, interpret_source(input, &args, &opt));
    }
    {
        /* "default" statment */
        const char *input = 
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
            "  return j\n"
            ;

        ASSERTL(99, interpret_source(input, &args, &opt));
    }
    {
        /* local var init */
        const char *input = 
            "# main() int\n"
            "  - i int = 42\n"
            "  return i\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        /* global var init */
        const char *input = 
            "- _g_ int = 39\n"
            "# main() int\n"
            "  return _g_\n"
            ;

        ASSERTL(39, interpret_source(input, &args, &opt));
    }
    {
        /* local var type */
        const char *input = 
            "# main() int\n"
            "  - i = 41\n"
            "  return i\n"
            ;

        ASSERTL(41, interpret_source(input, &args, &opt));
    }
    {
        /* local var type */
        const char *input = 
            "# main() int\n"
            "  - i = 41\n"
            "  - f = 3.1415\n"
            "  - g float = 3.1\n"
            "  if f == g + 0.0415\n"
            "    i = 9\n"
            "  return i\n"
            ;

        ASSERTL(9, interpret_source(input, &args, &opt));
    }
    {
        /* "+=" operator */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  i += 4\n"
            "  return i\n"
            ;

        ASSERTL(46, interpret_source(input, &args, &opt));
    }
    {
        /* "-=" operator */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  i -= 4\n"
            "  return i\n"
            ;

        ASSERTL(38, interpret_source(input, &args, &opt));
    }
    {
        /* "*=" operator */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  i *= 4\n"
            "  return i\n"
            ;

        ASSERTL(168, interpret_source(input, &args, &opt));
    }
    {
        /* "/=" operator */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  i /= 4\n"
            "  return i\n"
            ;

        ASSERTL(10, interpret_source(input, &args, &opt));
    }
    {
        /* "%=" operator */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  i %= 4\n"
            "  return i\n"
            ;

        ASSERTL(2, interpret_source(input, &args, &opt));
    }
    {
        /* bool type */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  - b = true\n"
            "  if b\n"
            "    i = 19\n"
            "  return i\n"
            ;

        ASSERTL(19, interpret_source(input, &args, &opt));
    }
    {
        /* bool type */
        const char *input = 
            "# main() int\n"
            "  - i = 42\n"
            "  - b = true\n"
            "  - c = false\n"
            "  if b != !c\n"
            "    i = 19\n"
            "  return i\n"
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        /* nop statement */
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  for j in 0..7\n"
            "    nop\n"
            "    i += 1\n"
            "  return i\n"
            ;

        ASSERTL(7, interpret_source(input, &args, &opt));
    }
    {
        /* block comment */
        const char *input = 
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
            ;

        ASSERTL(42, interpret_source(input, &args, &opt));
    }
    {
        /* char literal */
        const char *input = 
            "# main() int\n"
            "  - i = 'a'\n"
            "  return i\n"
            ;

        ASSERTL(97, interpret_source(input, &args, &opt));
    }
    {
        /* char literal */
        const char *input = 
            "# main() int\n"
            "  - i = '\n'\n"
            "  return i\n"
            ;

        ASSERTL(10, interpret_source(input, &args, &opt));
    }
    {
        /* slash at the end of string literal */
        const char *input = 
            "# main() int\n"
            "  - i int\n"
            "  - s = \"Hello\\\\\"\n"
            "  if s == \"Hello\\\\\"\n"
            "    i = 13\n"
            "  return i\n"
            ;

        ASSERTL(13, interpret_source(input, &args, &opt));
    }
    {
        /* nil return type */
        const char *input = 
            "# foo()\n"
            "  return\n"
            "# main() int\n"
            "  - i = 11\n"
            "  return i\n"
            ;

        ASSERTL(11, interpret_source(input, &args, &opt));
    }
    {
        /* scope statement */
        const char *input = 
            "# main() int\n"
            "  - i = 17\n"
            "  ---\n"
            "    - i int\n"
            "    i = 9\n"
            "  return i\n"
            ;

        ASSERTL(17, interpret_source(input, &args, &opt));
    }

    if (GetTestCount() <= 1)
        printf("%d test done.\n", GetTestCount());
    else if (GetTestCount() > 1)
        printf("%d tests done.\n", GetTestCount());

    return 0;
}
