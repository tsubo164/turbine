[test]

# main(args vec{string}) int

  ---
    - s set{int}
    test.AssertI(0, setlen(s))
    setadd(s, 2)
    setadd(s, -1)
    test.AssertI(2, setlen(s))
    test.AssertB(false, setcontains(s, 3))
    test.AssertB(true,  setcontains(s, 2))
    test.AssertB(true,  setcontains(s, -1))
    test.AssertB(false, setcontains(s, -1 - 4))
    test.AssertB(false, setadd(s, -1))
    test.AssertB(true,  setadd(s, 5))
    test.AssertI(3, setlen(s))

  ---
    - s set{int}
    setadd(s, 5)
    setadd(s, 7)
    setadd(s, 9)
    setadd(s, 1)
    setadd(s, 3)
    setadd(s, 4)
    setadd(s, 10)
    setadd(s, 20)
    setadd(s, 30)
    setadd(s, 40)
    setadd(s, 50)
    setadd(s, 60)
    test.AssertI(12,    setlen(s))
    test.AssertB(true,  setcontains(s, 4))
    test.AssertB(true,  setremove(s, 4))
    test.AssertB(false, setremove(s, 17))
    test.AssertI(11,    setlen(s))
    test.AssertB(false, setcontains(s, 4))

  ---
    - s = set{11, 22, 33}
    test.AssertI(3, setlen(s))
    test.AssertB(false, setcontains(s, 4))
    test.AssertB(true,  setcontains(s, 11))
    test.AssertB(true,  setcontains(s, 22))
    test.AssertB(true,  setcontains(s, 33))
    test.AssertB(false, setremove(s, 17))
    test.AssertB(true,  setremove(s, 22))
    test.AssertI(2, setlen(s))

  ---
    - s set{int}
    setadd(s, 5)
    setadd(s, 7)
    setadd(s, 9)
    setadd(s, 1)
    setadd(s, 3)
    setadd(s, 4)
    - sum = 0
    for val in s
      sum += val
    test.AssertI(29, sum)

  ---
    - s = set{5, 7, 9, 1, 3, 4, 10, 20, 30, 40, 50, 60}
    test.AssertI(12,    setlen(s))
    - sum = 0
    for i, val in s
      sum += i
    test.AssertI(66, sum)

  ---
    - s set{string} = set{
      "Alpha",
      "Beta",
      "Charlie",
      "Delta",
      "Echo",
      "Foxtrot",
      "Golf",
      "Hotel",
      "India",
      "Juliet",
      "Kilo",
      "Lima",
      "Mike",
      "November",
      "Oscar",
      "Papa",
      "Quebec",
      "Romeo",
      "Sierra",
      "Tango",
      "Uniform",
      "Victor",
      "Whiskey",
      "X-ray",
      "Yankee",
      "Zebra"
    }
    test.AssertI(26, setlen(s))

    setremove(s, "Alpha")
    setremove(s, "Beta")
    setremove(s, "Charlie")
    setremove(s, "Delta")
    setremove(s, "Echo")
    setremove(s, "Foxtrot")
    setremove(s, "Golf")
    setremove(s, "Hotel")
    setremove(s, "India")
    setremove(s, "Juliet")
    setremove(s, "Kilo")
    setremove(s, "Lima")
    setremove(s, "Mike")
    setremove(s, "November")
    setremove(s, "Oscar")
    setremove(s, "Papa")
    setremove(s, "Quebec")
    setremove(s, "Romeo")
    setremove(s, "Sierra")
    setremove(s, "Tango")
    setremove(s, "Uniform")
    setremove(s, "Victor")
    setremove(s, "Whiskey")
    setremove(s, "X-ray")
    setremove(s, "Yankee")
    setremove(s, "Zebra")
    test.AssertI(0, setlen(s))

  print(test._test_count_, "tests done.")

  return 0
