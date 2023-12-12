//## Point
//  - x int
//  - y int
//
//- pt Point

//# main() int
//  pt.x = 2
//  pt.y = 3
//  a = pt.y
//  return pt.x + pt.y

//# add int
//    - x int
//    - y int
//        - test string
//        if x < y
//            print("No!")
//        return x + y

//# printnode void
//  + node :Node
//  + depth int
//  ---
//  - d int
//
//# exit void
//  ---
//  - d int

- test_count int

# AssertI(expected int, actual int, line int) int
    test_count++

    if expected != actual
        print("error:")
        exit(1)
    return

# sub(x int, y int) int
    return x - y

# main() int
    - i int
    - j int
    i = 14
    j = 3
    return 46 - i % j
    return sub(12, 7)
    return AssertI(1, 1, 7)

//- glbl int
//
//# main() int
//  - i int
//  if 42 != 42
//    i = 0
//  else
//    i = 11
//  i++
//  glbl = 99
//  glbl++
//  return i
