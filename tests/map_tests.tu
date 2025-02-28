[test]

# main(args vec{string}) int

  ---
    - m map{int}
    test.AssertI(0, maplen(m))

    m["foo"] = 42
    test.AssertI(42, m["foo"])
    m["bar"] = -1212
    test.AssertI(-1212, m["bar"])
    test.AssertI(2, maplen(m))

  ---
    // map literal
    - m = map{ "Go":923, "Python":4261, "Lua":1453, "Turbine":777 }
    test.AssertI(923,  m["Go"])
    test.AssertI(4261, m["Python"])
    test.AssertI(1453, m["Lua"])
    test.AssertI(777,  m["Turbine"])
    test.AssertI(4, maplen(m))

  ---
    - m = map{
      "foo":42,
      "bar":1212,
      "baz":284,
      "Go":923,
      "Nim":1736,
      "Zig":4812,
      "C/C++":361,
      "Bash":5792,
      "Rust":814,
      "Lua":1453,
      "Markdown":2678,
      "Toml":3921,
      "Yaml":837,
      "Java":5293,
      "Kotlin":1847,
      "Dart":615,
      "Lisp":7432,
      "Python":4261,
      "Ruby":519,
      "Perl":682,
      "PHP":-832,
      "JavaScript":7432,
      "Swift":3921,
      "Turbine":3574
    }
    test.AssertI(24, maplen(m))
    test.AssertI(3574, m["Turbine"])

  print(test._test_count_, "tests done.")

  return 0
