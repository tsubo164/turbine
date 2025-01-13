[test]

# main() int

  ---
    // "for" statment
    - j int
    j = 0
    for i in 0..10
        j = j + 2
    test.AssertI(20, j)

  ---
    // "while" statment
    - i int
    i = 0
    while i < 10
        i += 1
    test.AssertI(10, i)

  ---
    // "while" statment infinite loop
    - i int
    i = 0
    while true
      i += 1
      if i == 8
        break
    test.AssertI(8, i)

  ---
    // "break" statment
    - i int
    for j in 0..10
      if j == 5
        break
      i += 1
    test.AssertI(5, i)

  ---
    // "continue" statment
    - j int
    j = 0
    for i in 0..10
      if i % 2 == 0
        continue
      j += 1
    test.AssertI(5, j)

  // for zero times loop
  ---
    - a = 13
    for i in 0..0
      a *= 2
    test.AssertI(13, a)

  ---
    // step 2
    - sum = 0
    for i in 0..10, 2
      sum += i
    test.AssertI(20, sum)

  ---
    // for array value
    - array = [11, 23, 204, 2]
    - sum = 0
    for val in array
      sum += val
    test.AssertI(240, sum)

  ---
    // for array value and index
    - array = [11, 23, 204, 2]
    - sum = 0
    for i, val in array
      sum += i * val
    test.AssertI(437, sum)

  ---
    - sum = 0
    - i = 0
    while i < 10
      sum += i
      i += 1
    test.AssertI(45, sum)

  ---
    - sum = 0
    - i = 0
    while true
      sum += i
      i += 1
      if i >= 10
        break
    test.AssertI(45, sum)

  ---
    // for map value
    - m = { "Go":923, "Python":4261, "Lua":1453, "Turbine":777 }
    - sum = 0
    for val in m
      sum += val
    test.AssertI(7414, sum)

  print(test._test_count_, "tests done.")

  return 0
