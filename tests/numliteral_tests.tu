> test

# main() int

  ---
    - f = 5.3876e4
    test.AssertF(53876.0, f)
    f = 4e-11
    test.AssertF(0.00000000004, f)
    f = 1e+5
    test.AssertF(100000.0, f)
    f = 7.321E-3
    test.AssertF(0.007321, f)
    f = 3.2E+4
    test.AssertF(32000.0, f)
    f = 0.5e-6
    test.AssertF(0.0000005, f)
    f = 0.45
    test.AssertF(0.45, f)
    f = 6.e10
    test.AssertF(60000000000.0, f)

  print(test._test_count_, "tests done.")

  return 0
