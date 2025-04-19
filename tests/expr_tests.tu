> test

# return_large_int() int
  return 4422

# main() int

  ---
    // large number literals that don't fit into a register
    - a = 400 - 300
    test::AssertI(100, a)
    - b = 300 - 400
    test::AssertI(-100, b)

  ---
    test::AssertI(4422, return_large_int())

  ---
    test::AssertI(3000, 2000 + 1000)
    test::AssertI(1000, 2000 - 1000)
    test::AssertI(2000000, 2000 * 1000)
    test::AssertI(2, 2000 / 1000)
    test::AssertI(6, 2000 % 997)
    test::AssertI(-42, -42)

  ---
    test::AssertB(false, 2000 == 1000)
    test::AssertB(true, 2000 != 1000)
    test::AssertB(true, 2000 >= 1000)
    test::AssertB(false, 2000 <= 1000)
    test::AssertB(true, 2000 > 1000)
    test::AssertB(false, 2000 < 1000)

  ---
    test::AssertI(4, 1 << 2)
    test::AssertI(1024, 1 << 10)
    test::AssertI(32, 1024 >> 5)
    test::AssertI(1024, 1024 >> 0)
    test::AssertI(0, 4 & 2)
    test::AssertI(6, 4 | 2)
    test::AssertI(1, 1 ^ 0)
    test::AssertI(0, 1 ^ 1)
    test::AssertI(0, 0 ^ 0)
    test::AssertI(1, 0 ^ 1)

  ---
    test::AssertB(false, false && false)
    test::AssertB(false, true && false)
    test::AssertB(true, true || false)
    test::AssertB(false, false || false)
    test::AssertB(false, !true)
    test::AssertB(true, !false)

  ---
    test::AssertF(3000.0, 2000.0 + 1000.0)
    test::AssertF(1000.0, 2000.0 - 1000.0)
    test::AssertF(2000000.0, 2000.0 * 1000.0)
    test::AssertF(2.0, 2000.0 / 1000.0)
    test::AssertF(6.0, 2000.0 % 997.0)
    test::AssertF(-42.0, -42.0)

  ---
    test::AssertB(false, 2000.0 == 1000.0)
    test::AssertB(true, 2000.0 != 1000.0)
    test::AssertB(true, 2000.0 >= 1000.0)
    test::AssertB(false, 2000.0 <= 1000.0)
    test::AssertB(true, 2000.0 > 1000.0)
    test::AssertB(false, 2000.0 < 1000.0)

  print(test::_test_count_, "tests done.")

  return 0
