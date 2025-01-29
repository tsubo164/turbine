[test]

# seven() int
  return 7

# main() int

  ---
    - a int
    - b int
    a = 42
    if a == 12
      b = 11
    else
      b = 22
    test.AssertI(22, b)

  ---
    - a int
    - b int
    a = 42
    if a == 42
      b = 11
    else
      b = 22
    test.AssertI(11, b)

  ---
    - a int
    - b int
    a = 42
    if a == 42
      b = 1
    else
      b = 0
    test.AssertI(1, b)

  ---
    - a int
    - b int
    a = 42
    if a == 41
      b = 1
    else
      b = 0
    test.AssertI(0, b)

  // if statement
  // line comment at beginning of line
  ---
    - a int
    - b int
   // comment with incorrect indetation
    a = 42 // comment after vaid statement
    if a == 42
      b = 1
    else
      b = 0
    // comment with the same indetation
    test.AssertI(1, b)

  ---
    // if statement
    // line comment at beginning of line
    - a int
    - b int
  // comment with incorrect indetation
    a = 42 // comment after vaid statement
    if a == 42
      b = seven()
    else
      b = 0
    // comment with the same indetation
    test.AssertI(7, b)

  ---
    - s string
    - a = 33
    test.AssertI(33, a)

  ---
    - a int
    a = 42
    if a == 42
        - b int
        b = 13
        if  b == 13
            - c int
            c = 9
    //b = 4 // error
    a = 31
    test.AssertI(31, a)

  ---
    - i int
    if 13 == 12
      i = 42
    elif 12 == 12
      i = 77
    else
      i = 99
    test.AssertI(77, i)

  print(test._test_count_, "tests done.")

  return 0
