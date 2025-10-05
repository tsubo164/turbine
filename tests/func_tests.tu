> test

# foo(a int, &ok bool) int
  ok = true
  return 2 * a

# bar(&a int)
  a = 42


# main(args vec{string}) int

  ---
    // output parameter
    - ok bool
    test.AssertB(false, ok)
    - a = foo(12, &ok)
    test.AssertB(true, ok)
    test.AssertI(24, a)

    - b int
    test.AssertI(0, b)
    bar(&b)
    test.AssertI(42, b)

  print(test._test_count_, "tests done.")

  return 0
