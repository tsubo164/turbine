> test

- _vec_ vec{int} = vec{0, 0, 0, 0}

- _a_ vec{int}
- _b_ vec{int}
- _c_ vec{vec{int}}
- _d_ vec{int}

# main(args vec{string}) int

  // args
  ---
    test.AssertI(4, veclen(args))
    test.AssertS("foo", args[1])
    test.AssertS("bar", args[2])
    test.AssertS("baz", args[3])

  ---
    - a vec{int} = vec{0, 0, 0, 0, 0, 0, 0, 0}
    - i = 9
    a[2] = 87
    test.AssertI(87, a[2])
    a[5] = 92
    test.AssertI(92, a[5])
    i = a[2] + a[5]
    test.AssertI(179, i)

  ---
    - a vec{int} = vec{0, 1, 2, 3, 4, 5, 6, 7, 8, 9}
    - i = 7
    a[2] = 8
    test.AssertI(8, a[2])
    a[5] = 2
    test.AssertI(2, a[5])
    i = a[2] + a[5]
    test.AssertI(10, i)

  // global var
  ---
    _vec_[3] = 99
    test.AssertI(99, _vec_[3])
    test.AssertI(4, veclen(_vec_))

  // resize
  ---
    - a = vec{99, 11, 22, 33 + 9}
    test.AssertI(64, a[2] + a[3])

    test.AssertI(4, veclen(a))
    resize(a, 8)
    test.AssertI(8, veclen(a))

  // resize in for
  ---
    - a = vec{11, 22, 33, 44}

    for i, x in resize(a, 8)
      if (i == 2)
        test.AssertI(33, x)
    test.AssertI(8, veclen(a))

  ---
    // vector init with resize
    - x = resize(vec{1.1}, 4)
    test.AssertI(4, veclen(x))
    test.AssertF(1.1, x[0])
    x[1] = 3.9
    test.AssertF(3.9, x[1])

  ---
    // vector and arithmetic operators
    - a = vec{2, 3, 4}
    a[1] <<= 1
    test.AssertI(6, a[1])

    // vector and arithmetic operators with addition
    - b = vec{5, 6, 7}
    b[0] += 10
    test.AssertI(15, b[0])

    // vector and arithmetic operators with subtraction
    - c = vec{10, 20, 30}
    c[2] -= 5
    test.AssertI(25, c[2])

    // vector and arithmetic operators with multiplication
    - d = vec{1, 2, 3}
    d[1] *= 4
    test.AssertI(8, d[1])

    // vector and arithmetic operators with division
    - e = vec{8, 16, 32}
    e[0] /= 4
    test.AssertI(2, e[0])

    // vector and arithmetic operators with modulo
    - f = vec{10, 15, 20}
    f[1] %= 4
    test.AssertI(3, f[1])

    // vector and bitwise OR
    - g = vec{1, 2, 4}
    g[2] |= 2
    test.AssertI(6, g[2])

    // vector and bitwise AND
    - h = vec{7, 8, 15}
    h[2] &= 6
    test.AssertI(6, h[2])

    // vector and bitwise XOR
    - i = vec{5, 10, 15}
    i[0] ^= 2
    test.AssertI(7, i[0])

    // vector and bitwise NOT (unary)
    - j = vec{1, 2, 3}
    j[1] = ~j[1]
    test.AssertI(-3, j[1])

    // vector and right shift
    - k = vec{8, 16, 32}
    k[0] >>= 2
    test.AssertI(2, k[0])

    // combined vector and arithmetic logic
    - l = vec{1, 2, 3}
    l[0] = (l[1] * l[2]) + l[0]
    test.AssertI(7, l[0])

    // nested vectors and operations
    - m = vec{vec{1, 2}, vec{3, 4}, vec{5, 6}}
    m[2][1] += m[1][0] * m[0][1]
    test.AssertI(12, m[2][1])

    // updating vector elements with a function
    - n = vec{10, 20, 30}
    n[1] = n[0] + n[2] - 5
    test.AssertI(35, n[1])

    // vector index based on another vector element
    - o = vec{0, 1, 2, 3}
    - p = vec{10, 20, 30, 40}
    o[2] = p[o[1]]
    test.AssertI(20, o[2])

    // vector length-based operations
    - q = vec{10, 20, 30, 40}
    q[3] += veclen(q)
    test.AssertI(44, q[3])

  ---
    // vector and arithmetic operators with addition
    - a = vec{5.43, 6.11, 7.95}
    a[0] += 10.22
    test.AssertF(15.65, a[0])

    // vector and arithmetic operators with subtraction
    - b = vec{10.5, 20.8, 30.3}
    b[2] -= 5.7
    test.AssertF(24.6, b[2])

    // vector and arithmetic operators with multiplication
    - c = vec{1.2, 2.3, 3.4}
    c[1] *= 4.0
    test.AssertF(9.2, c[1])

    // vector and arithmetic operators with division
    - d = vec{8.8, 16.4, 32.1}
    d[0] /= 4.0
    test.AssertF(2.2, d[0])

    // vector and arithmetic operators with modulo (using floating-point)
    - e = vec{10.5, 15.8, 20.2}
    e[1] %= 4.3
    //test.AssertF(2.9, e[1])
    test.AssertB(true, e[1] - 2.9 < 1e-14)

    // vector and combined arithmetic operations
    - f = vec{1.1, 2.2, 3.3}
    f[0] = (f[1] * f[2]) + f[0]
    test.AssertF(8.36, f[0])

    // nested vectors and operations
    - g = vec{vec{1.1, 2.2}, vec{3.3, 4.4}, vec{5.5, 6.6}}
    g[2][1] += g[1][0] * g[0][1]
    test.AssertF(13.86, g[2][1])

    // updating vector elements with a function
    - h = vec{10.7, 20.3, 30.6}
    h[1] = h[0] + h[2] - 5.5
    test.AssertF(35.8, h[1])

    // vector index based on another vector element (floats used for computation)
    - i = vec{0.0, 1.0, 2.0, 3.0}
    - j = vec{10.1, 20.2, 30.3, 40.4}
    i[2] = j[int(i[1])]
    test.AssertF(20.2, i[2])

    // vector length-based operations
    - k = vec{10.1, 20.2, 30.3, 40.4}
    k[3] += float(veclen(k)) * 1.1
    test.AssertF(44.8, k[3])

    // floating-point precision tests
    - l = vec{0.1, 0.2, 0.3}
    l[2] += 0.4
    test.AssertF(0.7, l[2])

    // float and integer mixed operations
    - m = vec{1.5, 2.5, 3.5}
    - n = vec{2, 4, 6}
    m[1] *= float(n[0])
    test.AssertF(5.0, m[1])

    // float arithmetic with a negative value
    - o = vec{10.5, -5.2, 7.3}
    o[1] -= 2.3
    test.AssertF(-7.5, o[1])

    // float exponentiation
    //- p = vec{1.5, 2.5, 3.5]
    //p[0] **= 2
    //test.AssertF(2.25, p[0])

    // float division with result truncation (optional behavior check)
    - q = vec{10.5, 20.5, 30.5}
    q[2] = float(int(q[2] / 2.0))
    test.AssertF(15.0, q[2])
  
  ---
    // global variable vector of ints with addition
    _a_ = vec{1, 2, 3}
    _a_[0] += 5
    test.AssertI(6, _a_[0])

    // global variable vector of ints with subtraction
    _b_ = vec{10, 20, 30}
    _b_[2] -= 15
    test.AssertI(15, _b_[2])

    // global variable vector of ints with multiplication
    _a_ = vec{2, 3, 4}  // reusing _a_
    _a_[1] *= 3
    test.AssertI(9, _a_[1])

    // global variable vector of ints with division
    _b_ = vec{8, 16, 32}  // reusing _b_
    _b_[0] /= 2
    test.AssertI(4, _b_[0])

    // global variable vector of ints with modulo
    _a_ = vec{10, 15, 20}  // reusing _a_
    _a_[1] %= 4
    test.AssertI(3, _a_[1])

    // global variable vector with a new vector literal
    _b_ = vec{100, 200, 300}  // reusing _b_
    _b_[0] -= 50
    test.AssertI(50, _b_[0])

    // global variable vector combined arithmetic operations
    _a_ = vec{1, 2, 3}  // reusing _a_
    _a_[0] = (_a_[1] * _a_[2]) + _a_[0]
    test.AssertI(7, _a_[0])

    // nested global variable vectors (using subscript with _a_ and _b_)
    _c_ = vec{vec{1, 2}, vec{3, 4}, vec{5, 6}}
    _c_[2][1] += _c_[1][0] * _c_[0][1]
    test.AssertI(12, _c_[2][1])

    // updating global variable vector elements with a function
    _b_ = vec{10, 20, 30}  // reusing _b_
    _b_[1] = _b_[0] + _b_[2] - 5
    test.AssertI(35, _b_[1])

    // global variable vector index based on another global vector element
    _a_ = vec{0, 1, 2, 3}  // reusing _a_
    _b_ = vec{10, 20, 30, 40}  // reusing _b_
    _a_[2] = _b_[_a_[1]]
    test.AssertI(20, _a_[2])

    // global variable vector length-based operations
    _a_ = vec{10, 20, 30, 40}  // reusing _a_
    _a_[3] += veclen(_a_)
    test.AssertI(44, _a_[3])

    // global variable float mixed with int operation
    _b_ = vec{5, 10, 15}  // reusing _b_
    _b_[2] += int(2.5 * float(_b_[0]))
    test.AssertI(27, _b_[2])

    // global variable with new vector literal
    _a_ = vec{50, 60, 70}  // reusing _a_
    _a_[0] = 100
    test.AssertI(100, _a_[0])

  // vecpush() tests
  ---
    - a = vec{1, 2, 3}
    vecpush(a, 4)
    test.AssertI(4, veclen(a))
    test.AssertI(4, a[3])

  ---
    - b = vec{"apple", "banana"}
    vecpush(b, "cherry")
    test.AssertI(3, veclen(b))
    test.AssertS("cherry", b[2])

  ---
    - c = vec{10.5, 20.5}
    vecpush(c, 30.5)
    test.AssertI(3, veclen(c))
    test.AssertF(30.5, c[2])

  ---
    - d vec{int}
    vecpush(d, 42)
    test.AssertI(1, veclen(d))
    test.AssertI(42, d[0])

  ---
    - e = vec{vec{1, 2}, vec{3, 4}}
    vecpush(e, vec{5, 6})
    test.AssertI(3, veclen(e))
    test.AssertI(5, e[2][0])
    test.AssertI(6, e[2][1])

  ---
    // vecpush with global variable
    _d_ = vec{100, 200}
    vecpush(_d_, 300)
    test.AssertI(3, veclen(_d_))
    test.AssertI(300, _d_[2])

  print(test._test_count_, "tests done.")

  return 0
