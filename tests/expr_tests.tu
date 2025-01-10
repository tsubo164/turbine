[test]

# main() int

  ---
    // large number literals that don't fit into a register
    - a = 400 - 300
    test.AssertI(100, a)
    - b = 300 - 400
    test.AssertI(-100, b)

  print(test._test_count_, "tests done.")

  return 0
