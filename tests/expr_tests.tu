[test]

# return_large_int() int
  return 4422

# main() int

  ---
    // large number literals that don't fit into a register
    - a = 400 - 300
    test.AssertI(100, a)
    - b = 300 - 400
    test.AssertI(-100, b)

  ---
    test.AssertI(4422, return_large_int())

  print(test._test_count_, "tests done.")

  return 0
