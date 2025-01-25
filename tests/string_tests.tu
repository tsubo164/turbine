[test]

# main(args []string) int

  ---
    - a = 42
    - b = 3.14
    - fmt = format("<%d, %g>", a, b)
    test.AssertS("<42, 3.14>", fmt)

  ---
    - b = 2.75
    - fmt = format("%g%%", b)
    test.AssertS("2.75%", fmt)

  ---
    - d = 42
    - x = 0xae
    - X = 0XF9
    - o = 022//18
    - O = 027
    - f = 6.04
    - s = "Hello"
    - fmt = format("%d, 0x%x, 0X%X, 0%o, 0%o %g, %s", d, x, X, o, O, f, s)
    test.AssertS("42, 0xae, 0XF9, 022, 027 6.04, Hello", fmt)

  ---
    - a = 42
    - f = 3.14159
    - fmt string

    // align left and width
    fmt = format("%-8d, %010g", a, f)
    test.AssertS("42      , 0003.14159", fmt)

    // plus sign and precision
    fmt = format("%-+8d, %010.3g", a, f)
    test.AssertS("+42     , 0000003.14", fmt)

    // positive number space
    fmt = format("%- 8d, %010.3g", a, f)
    test.AssertS(" 42     , 0000003.14", fmt)

    // char
    fmt = format("%c", 97)
    test.AssertS("a", fmt)

    // alternate
    fmt = format("%#x, %#X", a, a)
    test.AssertS("0x2a, 0X2A", fmt)
    fmt = format("%#f, %#e, %#g", f, f, f)
    test.AssertS("3.141590, 3.141590e+00, 3.14159", fmt)

  ---
    // format number
    - x = 12345678
    - y = 3.0
    - fmt string
    fmt = format("[%-_15d] [%+10g]", x, y)
    test.AssertS("[12_345_678     ] [        +3]", fmt)
    fmt = format("[%,15d] [%-10g]", x, y)
    test.AssertS("[     12,345,678] [3         ]", fmt)
    fmt = format("[%+,15d] [%- 10g]", x, y)
    test.AssertS("[    +12,345,678] [ 3        ]", fmt)
    fmt = format("[%+,15d] [%+10g]", -x, y)
    test.AssertS("[    -12,345,678] [        +3]", fmt)
    fmt = format("[%- ,15d] [%+10.0g]", x, y)
    test.AssertS("[ 12,345,678    ] [      +3.0]", fmt)

    y = 12345678.98
    fmt = format("[%-_15d] [%,15.2f]", x, y)
    test.AssertS("[12_345_678     ] [  12,345,678.98]", fmt)
    fmt = format("[%-_15d] [%-,15.2f]", x, y)
    test.AssertS("[12_345_678     ] [12,345,678.98  ]", fmt)
    fmt = format("[%-_15d] [%-+,15.2f]", x, y)
    test.AssertS("[12_345_678     ] [+12,345,678.98 ]", fmt)

    - z = 42.0
    fmt = format("[%g] [%.0g]", z, z)
    test.AssertS("[42] [42.0]", fmt)

  ---
    // format string
    - s = "hello"
    - fmt string
    fmt = format("[%s]", s)
    test.AssertS("[hello]", fmt)
    fmt = format("[%-15s]", s)
    test.AssertS("[hello          ]", fmt)
    fmt = format("[%15s]", s)
    test.AssertS("[          hello]", fmt)
    fmt = format("[%15.3s]", s)
    test.AssertS("[            hel]", fmt)
    fmt = format("[%.4s]", s)
    test.AssertS("[hell]", fmt)
    fmt = format("[%-15.4s]", s)
    test.AssertS("[hell           ]", fmt)

  ---
    // strlen()
    test.AssertI(0, strlen(""))
    test.AssertI(5, strlen("Hello"))
    test.AssertI(6, strlen("Hello\n"))

  ---
    // cat string
    - s = ""
    test.AssertS("", s)
    s += "Hello"
    test.AssertS("Hello", s)
    s += ", World!"
    test.AssertS("Hello, World!", s)

  ---
    // constexpr
    test.AssertS("Hello, World!", "Hello," + " World!")
    test.AssertB(false, "Hello," == " World!")
    test.AssertB(true,  "foo" != "bar")
    test.AssertB(true,  "foo" >= "bar")
    test.AssertB(false, "foo" <= "bar")
    test.AssertB(true,  "foo" >  "bar")
    test.AssertB(false, "foo" <  "bar")
    test.AssertB(true,  "foo" >= "foo")
    test.AssertB(true,  "foo" <= "foo")

  print(test._test_count_, "tests done.")

  return 0
