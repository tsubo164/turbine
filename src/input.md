# inc(x int) int
    - x int
    return x + 1

# main() int
    - a int
    a = 42 // a(1)
    if a == 42
        - b int // b(2)
        b = 13

        if  b == 13
            - c int
            - a int // a(3)
            c = 9
            a = 11  // a(3)
            b = a   // b(2)
        a = b  // a(1)
    //b = 4 // error
    return a // a(1)
