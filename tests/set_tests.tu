[test]

# main(args []string) int

  ---
    - s set{int}
    test.AssertI(0, setlen(s))
    setadd(s, 2)
    setadd(s, -1)
    test.AssertI(2, setlen(s))


  ---
    - s = set{11, 22, 33}

  print(test._test_count_, "tests done.")

  return 0
