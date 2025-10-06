> test

# foo(a int, &ok bool) int
  ok = true
  return 2 * a

# bar(&a int)
  a = 42

# baz(&a int)
  a = 2
  a = a + 3
  a += 8

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

  ---
    // use output parameter in function
    - a int
    test.AssertI(0, a)
    baz(&a)
    test.AssertI(13, a)

  print(test._test_count_, "tests done.")

  return 0
