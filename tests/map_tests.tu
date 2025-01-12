[test]

# main(args []string) int

  ---
    - m {}int

    m["foo"] = 42
    test.AssertI(42, m["foo"])
    m["bar"] = -1212
    test.AssertI(-1212, m["bar"])

  ---
    // map literal
    - m = { "Go":923, "Python":4261, "Lua":1453, "Turbine":777 }
    test.AssertI(923,  m["Go"])
    test.AssertI(4261, m["Python"])
    test.AssertI(1453, m["Lua"])
    test.AssertI(777,  m["Turbine"])

  print(test._test_count_, "tests done.")

  return 0
