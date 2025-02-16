[test]

# main(args []string) int

  ---
    - s stack{int}
    test.AssertI(0, stacklen(s))
    stackpush(s, 42)
    test.AssertI(1, stacklen(s))
    stackpush(s, -239)
    test.AssertI(2, stacklen(s))

  print(test._test_count_, "tests done.")

  return 0
