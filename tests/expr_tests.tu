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

  ---
    test.AssertI(3000, 2000 + 1000)
    test.AssertI(1000, 2000 - 1000)
    test.AssertI(2000000, 2000 * 1000)
    test.AssertI(2, 2000 / 1000)
    test.AssertI(6, 2000 % 997)
    test.AssertI(-42, -42)

  ---
    test.AssertB(false, false && false)
    test.AssertB(false, true && false)
    test.AssertB(true, true || false)
    test.AssertB(false, false || false)
    test.AssertB(false, !true)
    test.AssertB(true, !false)

  print(test._test_count_, "tests done.")

  return 0
