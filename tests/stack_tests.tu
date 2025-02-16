[test]

# main(args []string) int

  ---
    - s stack{int}
    test.AssertI(0, stacklen(s))
    test.AssertB(true, stackempty(s))

    stackpush(s, 42)
    test.AssertI(42, stacktop(s))
    test.AssertI(1, stacklen(s))

    stackpush(s, -239)
    test.AssertI(-239, stacktop(s))
    test.AssertI(2, stacklen(s))
    test.AssertB(false, stackempty(s))

    test.AssertI(-239, stackpop(s))
    test.AssertI(1, stacklen(s))

    test.AssertI(42, stackpop(s))
    test.AssertI(0, stacklen(s))
    test.AssertB(true, stackempty(s))

  print(test._test_count_, "tests done.")

  return 0
