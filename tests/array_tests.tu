[test]

- _array_ []int = [0, 0, 0, 0]

- _a_ []int
- _b_ []int
- _c_ [][]int

# main(args []string) int

  // args
  ---
    test.AssertI(4, len(args))
    test.AssertS("foo", args[1])
    test.AssertS("bar", args[2])
    test.AssertS("baz", args[3])

  ---
    - a []int = [0, 0, 0, 0, 0, 0, 0, 0]
    - i = 9
    a[2] = 87
    test.AssertI(87, a[2])
    a[5] = 92
    test.AssertI(92, a[5])
    i = a[2] + a[5]
    test.AssertI(179, i)

  ---
    - a []int = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    - i = 7
    a[2] = 8
    test.AssertI(8, a[2])
    a[5] = 2
    test.AssertI(2, a[5])
    i = a[2] + a[5]
    test.AssertI(10, i)

  // global var
  ---
    _array_[3] = 99
    test.AssertI(99, _array_[3])
    test.AssertI(4, len(_array_))

  // resize
  ---
    - a = [99, 11, 22, 33 + 9]
    test.AssertI(64, a[2] + a[3])

    test.AssertI(4, len(a))
    resize(a, 8)
    test.AssertI(8, len(a))

  // resize in for
  ---
    - a = [11, 22, 33, 44]

    for i, x in resize(a, 8)
      if (i == 2)
        test.AssertI(33, x)
    test.AssertI(8, len(a))

  ---
    // array init with resize
    - x = resize([1.1], 4)
    test.AssertI(4, len(x))
    test.AssertF(1.1, x[0])
    x[1] = 3.9
    test.AssertF(3.9, x[1])

  ---
    // array and arithmetic operators
    - a = [2, 3, 4]
    a[1] <<= 1
    test.AssertI(6, a[1])

    // array and arithmetic operators with addition
    - b = [5, 6, 7]
    b[0] += 10
    test.AssertI(15, b[0])

    // array and arithmetic operators with subtraction
    - c = [10, 20, 30]
    c[2] -= 5
    test.AssertI(25, c[2])

    // array and arithmetic operators with multiplication
    - d = [1, 2, 3]
    d[1] *= 4
    test.AssertI(8, d[1])

    // array and arithmetic operators with division
    - e = [8, 16, 32]
    e[0] /= 4
    test.AssertI(2, e[0])

    // array and arithmetic operators with modulo
    - f = [10, 15, 20]
    f[1] %= 4
    test.AssertI(3, f[1])

    // array and bitwise OR
    - g = [1, 2, 4]
    g[2] |= 2
    test.AssertI(6, g[2])

    // array and bitwise AND
    - h = [7, 8, 15]
    h[2] &= 6
    test.AssertI(6, h[2])

    // array and bitwise XOR
    - i = [5, 10, 15]
    i[0] ^= 2
    test.AssertI(7, i[0])

    // array and bitwise NOT (unary)
    - j = [1, 2, 3]
    j[1] = ~j[1]
    test.AssertI(-3, j[1])

    // array and right shift
    - k = [8, 16, 32]
    k[0] >>= 2
    test.AssertI(2, k[0])

    // combined array and arithmetic logic
    - l = [1, 2, 3]
    l[0] = (l[1] * l[2]) + l[0]
    test.AssertI(7, l[0])

    // nested arrays and operations
    - m = [[1, 2], [3, 4], [5, 6]]
    m[2][1] += m[1][0] * m[0][1]
    test.AssertI(12, m[2][1])

    // updating array elements with a function
    - n = [10, 20, 30]
    n[1] = n[0] + n[2] - 5
    test.AssertI(35, n[1])

    // array index based on another array element
    - o = [0, 1, 2, 3]
    - p = [10, 20, 30, 40]
    o[2] = p[o[1]]
    test.AssertI(20, o[2])

    // array length-based operations
    - q = [10, 20, 30, 40]
    q[3] += len(q)
    test.AssertI(44, q[3])

  ---
    // array and arithmetic operators with addition
    - a = [5.43, 6.11, 7.95]
    a[0] += 10.22
    test.AssertF(15.65, a[0])

    // array and arithmetic operators with subtraction
    - b = [10.5, 20.8, 30.3]
    b[2] -= 5.7
    test.AssertF(24.6, b[2])

    // array and arithmetic operators with multiplication
    - c = [1.2, 2.3, 3.4]
    c[1] *= 4.0
    test.AssertF(9.2, c[1])

    // array and arithmetic operators with division
    - d = [8.8, 16.4, 32.1]
    d[0] /= 4.0
    test.AssertF(2.2, d[0])

    // array and arithmetic operators with modulo (using floating-point)
    - e = [10.5, 15.8, 20.2]
    e[1] %= 4.3
    //test.AssertF(2.9, e[1])
    test.AssertB(true, e[1] - 2.9 < 1e-14)

    // array and combined arithmetic operations
    - f = [1.1, 2.2, 3.3]
    f[0] = (f[1] * f[2]) + f[0]
    test.AssertF(8.36, f[0])

    // nested arrays and operations
    - g = [[1.1, 2.2], [3.3, 4.4], [5.5, 6.6]]
    g[2][1] += g[1][0] * g[0][1]
    test.AssertF(13.86, g[2][1])

    // updating array elements with a function
    - h = [10.7, 20.3, 30.6]
    h[1] = h[0] + h[2] - 5.5
    test.AssertF(35.8, h[1])

    // array index based on another array element (floats used for computation)
    - i = [0.0, 1.0, 2.0, 3.0]
    - j = [10.1, 20.2, 30.3, 40.4]
    i[2] = j[int(i[1])]
    test.AssertF(20.2, i[2])

    // array length-based operations
    - k = [10.1, 20.2, 30.3, 40.4]
    k[3] += float(len(k)) * 1.1
    test.AssertF(44.8, k[3])

    // floating-point precision tests
    - l = [0.1, 0.2, 0.3]
    l[2] += 0.4
    test.AssertF(0.7, l[2])

    // float and integer mixed operations
    - m = [1.5, 2.5, 3.5]
    - n = [2, 4, 6]
    m[1] *= float(n[0])
    test.AssertF(5.0, m[1])

    // float arithmetic with a negative value
    - o = [10.5, -5.2, 7.3]
    o[1] -= 2.3
    test.AssertF(-7.5, o[1])

    // float exponentiation
    //- p = [1.5, 2.5, 3.5]
    //p[0] **= 2
    //test.AssertF(2.25, p[0])

    // float division with result truncation (optional behavior check)
    - q = [10.5, 20.5, 30.5]
    q[2] = float(int(q[2] / 2.0))
    test.AssertF(15.0, q[2])
  
  ---
    // global variable array of ints with addition
    _a_ = [1, 2, 3]
    _a_[0] += 5
    test.AssertI(6, _a_[0])

    // global variable array of ints with subtraction
    _b_ = [10, 20, 30]
    _b_[2] -= 15
    test.AssertI(15, _b_[2])

    // global variable array of ints with multiplication
    _a_ = [2, 3, 4]  // reusing _a_
    _a_[1] *= 3
    test.AssertI(9, _a_[1])

    // global variable array of ints with division
    _b_ = [8, 16, 32]  // reusing _b_
    _b_[0] /= 2
    test.AssertI(4, _b_[0])

    // global variable array of ints with modulo
    _a_ = [10, 15, 20]  // reusing _a_
    _a_[1] %= 4
    test.AssertI(3, _a_[1])

    // global variable array with a new array literal
    _b_ = [100, 200, 300]  // reusing _b_
    _b_[0] -= 50
    test.AssertI(50, _b_[0])

    // global variable array combined arithmetic operations
    _a_ = [1, 2, 3]  // reusing _a_
    _a_[0] = (_a_[1] * _a_[2]) + _a_[0]
    test.AssertI(7, _a_[0])

    // nested global variable arrays (using subscript with _a_ and _b_)
    _c_ = [[1, 2], [3, 4], [5, 6]]
    _c_[2][1] += _c_[1][0] * _c_[0][1]
    test.AssertI(12, _c_[2][1])

    // updating global variable array elements with a function
    _b_ = [10, 20, 30]  // reusing _b_
    _b_[1] = _b_[0] + _b_[2] - 5
    test.AssertI(35, _b_[1])

    // global variable array index based on another global array element
    _a_ = [0, 1, 2, 3]  // reusing _a_
    _b_ = [10, 20, 30, 40]  // reusing _b_
    _a_[2] = _b_[_a_[1]]
    test.AssertI(20, _a_[2])

    // global variable array length-based operations
    _a_ = [10, 20, 30, 40]  // reusing _a_
    _a_[3] += len(_a_)
    test.AssertI(44, _a_[3])

    // global variable float mixed with int operation
    _b_ = [5, 10, 15]  // reusing _b_
    _b_[2] += int(2.5 * float(_b_[0]))
    test.AssertI(27, _b_[2])

    // global variable with new array literal
    _a_ = [50, 60, 70]  // reusing _a_
    _a_[0] = 100
    test.AssertI(100, _a_[0])

  print(test._test_count_, "tests done.")

  return 0
