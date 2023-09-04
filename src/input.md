# foo(x int) int
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

# main() int
    return foo(10)
