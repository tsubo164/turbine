[test]

# main(args []string) int

  ---
    - m {}int

    m["foo"] = 42
    test.AssertI(42, m["foo"])

  print(test._test_count_, "tests done.")

  return 0
