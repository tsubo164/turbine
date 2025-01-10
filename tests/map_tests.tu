[test]

# main(args []string) int

  ---
    - m {}int

    m["foo"] = 42
    test.AssertI(42, m["foo"])
    m["bar"] = -1212
    test.AssertI(-1212, m["bar"])

  print(test._test_count_, "tests done.")

  return 0
