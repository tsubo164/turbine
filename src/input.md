# foo(x int) int
    return 19
    if x == 10
        - y int
        y = 23
        x = y
    return x + 3

//# add int
//    - x int
//    - y int
//      * test string
//      return x + y
# add (x int, y int) int
    return x + y

# main() int
    return foo(10) + add(20 + 3)
