[test]
[my_calc]

//-----------------------------------------------------------------------------
# seven() int
  return 7

# add(x int, y int) int
  return x + y

# sub(x int, y int) int
  return x - y

# upper(s string) string
  return s

# foo(x int) int
  return 19
  - xx int
  if x == 10
      - y int
      y = 23
      xx = y
  return xx + 3

// nil return type
# bar()
  return

## Point
  - x int
  - y int

# get_point_y(p Point) int
  return p.y

# set_point_x(p *Point, x int)
  p.x = x

# set_point_y(p *Point, y int)
  p.y = y

- _pt_ Point
- _center_ = Point { y = 117 }
- _P_ = Point { y = -238, x = 1178 }
- _Q_ = _P_

// global pointer to function
- _addfp_ = add

# twice(n *int)
  *n = *n * 2

- _array_ []int = [0, 0, 0, 0]

- _gcount_ int
- _gvar_ int
- _g_ int = 39
    
# set_gvar() int
  _gvar_ = 119

:: Color
  | symbol
  | ---
  | R // 0
  | G // 1
  | B // 2

# main() int

  ---
    - id int
    id = 114
    test.AssertI(125, id + 11)

  ---
    - i int = 19
    test.AssertI(19, i)

  ---
    test.AssertI(42, 39 + 3)

  ---
    - id int
    id = 0
    test.AssertI(114, id + 114)

  ---
    test.AssertI(4422, 3129 + 1293)

  ---
    test.AssertI(5533, 3129 + 1293+1111)

  ---
    test.AssertI(42, 20+22)

  ---
    - a int
    a = 12
    test.AssertI(12, a)

  ---
    - a int
    a = 11
    test.AssertI(11, a)

  ---
    test.AssertB(false, 12 == 11)

  ---
    test.AssertB(true, 42 == 42)

  ---
    - a int
    a = 39
    test.AssertB(true, a == 39)

  ---
    - a int
    a = 39
    test.AssertB(true, a == 39)

  ---
    test.AssertI(7, seven())

  ---
    test.AssertI(42, seven() + 35)

  ---
    test.AssertI(42, seven() + add(30, 5))

  ---
    - a int
    - b int
    a = 42
    if a == 12
      b = 11
    or
      b = 22
    test.AssertI(22, b)

  ---
    - a int
    - b int
    a = 42
    if a == 42
      b = 11
    or
      b = 22
    test.AssertI(11, b)

  ---
    - a int
    - b int
    a = 42
    if a == 42
      b = 1
    or
      b = 0
    test.AssertI(1, b)

  ---
    - a int
    - b int
    a = 42
    if a == 41
      b = 1
    or
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
    or
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
    or
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
    - a = seven() + add(30, 5)
    test.AssertI(42, a)

  ---
    test.AssertI(0, _gvar_)
    set_gvar()
    test.AssertI(119, _gvar_)

  ---
    test.AssertI(42, foo(10) + add(20, 3))

  ---
    - a int
    _pt_.x = 2
    _pt_.y = 3
    a = _pt_.y
    test.AssertI(5, _pt_.x + _pt_.y)

  ---
    - f float
    f = 3.14
    test.AssertF(3.14, f)

  ---
    test.AssertI(25, 0xF + 0Xa)

  ---
    - f float
    - g float
    f = 3.14
    g = 0.86
    test.AssertF(4.0, f + g)

  ---
    - i int
    if 13 == 13
      i = 42
    or
      i = 99
    test.AssertI(42, i)

  ---
    - s0 string
    - s1 string
    - str string
    - i = 0
    s0 = "Hello, "
    s1 = "World!"
    str = s0 + s1
    if str == "Hello, World!"
      i = 42
    or
      i = 0
    test.AssertI(42, i)

  ---
    - i int
    if 42 != 42
      i = 0
    or
      i = 11
    test.AssertI(11, i)

  ---
    // '-' operator and order of eval args
    - i = sub(12, 7)
    test.AssertI(5, i)

  ---
    // '*' operator
    - i int
    - j int
    i = 12
    j = 3
    test.AssertI(10, 46 - i * j)

  ---
    // '/' operator
    - i int
    - j int
    i = 12
    j = 3
    test.AssertI(42, 46 - i / j)

  ---
    // '%' operator
    - i int
    - j int
    i = 19
    j = 7
    test.AssertI(41, 46 - i % j)

  ---
    // '(' expr ')'
    - i int
    - j int
    i = 19
    j = 17
    test.AssertI(42, 21 * (i - j))

  ---
    // "||" operator
    - i int
    - j int
    i = 0
    j = 7
    test.AssertI(1, int((i || j) != 0))

  ---
    // "||" operator
    - i int
    - j int
    i = 0
    j = 0
    test.AssertI(0, int((i || j) != 0))

  ---
    // "&&" operator
    - i int
    - j int
    i = 0
    j = 7
    test.AssertI(0, int((i && j) != 0))

  ---
    // "&&" operator
    - i int
    - j int
    i = 1
    j = 7
    test.AssertI(1, int((i && j) != 0))

  ---
    // "+" unary operator
    - i int
    i = 7
    test.AssertI(7, +i)

  ---
    // "-" unary operator
    - i int
    i = 42
    test.AssertI(-42, -i)

  ---
    // "-+" unary operator
    - i int
    i = 42
    test.AssertI(-42, -+-+-+- -+-+ +-i)

  ---
    // "!" unary operator
    - i int
    i = 42
    test.AssertI(1, int(int(!(42 != i)) != 0))

  ---
    // "!" unary operator
    - i int
    i = 42
    test.AssertI(0, int((!i) != 0))

  ---
    // "<" operator
    - i int
    i = 42
    test.AssertI(0, int(int(i < 5) != 0))

  ---
    // "<=" operator
    - i int
    i = 42
    test.AssertI(1, int(int(i <= 42) != 0))

  ---
    // ">" operator
    - i int
    i = 42
    test.AssertI(1, int(int(i > 5) != 0))

  ---
    // ">=" operator
    - i int
    i = 42
    test.AssertI(1, int(int(i >= 42) != 0))

  ---
    // " += 1" operator
    - i int
    i = 42
    i += 1
    test.AssertI(43, i)

  ---
    // "-= 1" operator
    - i int
    i = 42
    i -= 1
    test.AssertI(41, i)

  ---
    // "&" operator
    - i int
    i = 0x3A9
    test.AssertI(0xA9, i & 0xFF)

  ---
    // "|" operator
    - i int
    i = 0x3A9
    test.AssertI(0xFFF, i | 0xFFF)

  ---
    // "~" operator
    - i int
    i = 0x8
    test.AssertI(0x7, ~i & 0xF)

  ---
    // "^" operator
    - i int
    i = 0x78
    test.AssertI(0x88, i ^ 0xF0)

  ---
    // "<<" operator
    - i int
    i = 0x8
    test.AssertI(0x40, i << 3)

  ---
    // ">>" operator
    - i int
    i = 0x8F
    test.AssertI(0x08, i >> 4)

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

  ---
    // "switch" statment
    - i int
    - j int
    i = 2
    j = 0
    switch i
    case 0
      j = 0
    case 1
      j = 23
    case 2
      j = 34
    case 3
      j = 77
    test.AssertI(34, j)

  ---
    // "default" statment
    - i int
    - j int
    i = 5
    j = 0
    switch i
    case 0
      j = 0
    case 1
      j = 23
    case 2
      j = 34
    case 3
      j = 77
    default
      j = 99
    test.AssertI(99, j)

  ---
    // local var init
    - i int = 42
    test.AssertI(42, i)

  ---
    // global var init
    test.AssertI(39, _g_)

  ---
    // local var type
    - i = 41
    test.AssertI(41, i)

  ---
    // local var type
    - i = 41
    - f = 3.1415
    - g float = 3.1
    if f == g + 0.0415
      i = 9
    test.AssertI(9, i)

  ---
    // "+=" operator
    - i = 42
    i += 4
    test.AssertI(46, i)

  ---
    // "-=" operator
    - i = 42
    i -= 4
    test.AssertI(38, i)

  ---
    // "*=" operator
    - i = 42
    i *= 4
    test.AssertI(168, i)

  ---
    // "/=" operator
    - i = 42
    i /= 4
    test.AssertI(10, i)

  ---
    // "%=" operator
    - i = 42
    i %= 4
    test.AssertI(2, i)

  ---
    // bool type
    - i = 42
    - b = true
    if b
      i = 19
    test.AssertI(19, i)

  ---
    // bool type
    - i = 42
    - b = true
    - c = false
    if b != !c
      i = 19
    test.AssertI(42, i)

  ---
    // nop statement
    - i int
    for j in 0..7
      nop
      i += 1
    test.AssertI(7, i)

  ---
    // block comment
    - i = 42   // int
    /*
      this is a block comment
      the first indent has to match.
    /*
        nested block comment.
    */
    this is another line in block comment.
    */
    test.AssertI(42, i)

  ---
    // char literal
    - i = 'a'
    test.AssertI(97, i)

  ---
    // char literal
    - i = '\n'
    test.AssertI(10, i)

  ---
    // slash at the end of string literal
    - i int
    - s = "Hello\\\\"
    if s == "Hello\\\\"
      i = 13
    test.AssertI(13, i)

  ---
    // nil return type
    - i = 11
    bar()
    test.AssertI(11, i)

  ---
    // scope statement
    - i = 17
    ---
      - i int
      i = 9
    test.AssertI(17, i)

  // C++ test
  // ==========================================================================

  // switch with multi case values
  ---
    - n = 6
    - x = 0

    switch n
    case 1
      x = 11
    case 2, 3, 4
      x = 22
    case 13, 55, 6
      x = 99
    default
      x = 255
    test.AssertI(99, x)

  // switch with multi case values
  ---
    - n = 3
    - x = 0

    switch n
    case 1
      x = 11
    case 2, 3, 4
      x = 22
    case 13, 55, 6
      x = 99
    default
      x = 255
    test.AssertI(22, x)

  // bool default
  ---
    - b bool
    - c = true
    test.AssertB(true, b == false)
    test.AssertB(true, c != false)

  // pointer variable
  ---
    - i int = 42
    - p *int
    p = &i
    test.AssertI(42, *p)

  // TODO disable for now. because deref store uses set_global()
  // the result is written in global area not local register
  // referenced by the pointer
  // pointer argument
  //---
  //  - i int = 13
  //  twice(&i)
  //  test.AssertI(26, i)

  // array variable
  ---
    - a []int = [0, 0, 0, 0, 0, 0, 0, 0]
    - i = 9
    a[2] = 87
    test.AssertI(87, a[2])
    a[5] = 92
    test.AssertI(92, a[5])
    i = a[2] + a[5]
    test.AssertI(179, i)

  // array variable with const expression
  ---
    - a []int = [0, 1, 2, 3, 4, 5, 6, 7, 8, 9]
    - i = 7
    a[2] = 8
    test.AssertI(8, a[2])
    a[5] = 2
    test.AssertI(2, a[5])
    i = a[2] + a[5]
    test.AssertI(10, i)

  // global array variable
  ---
    _array_[3] = 99
    test.AssertI(99, _array_[3])

  // enum
  ---
    - a = Color.R
    test.AssertB(true, a == Color.R)
    test.AssertB(true, a != Color.G)

  // module import
  ---
    test.AssertF(3.1415, my_calc._PI_)
    my_calc._PI_ = 1.23
    test.AssertF(1.23, my_calc._PI_)
    test.AssertI(42, my_calc.add(31, 11))

  // for zero times loop
  ---
    - a = 13
    for i in 0..0
      a *= 2
    test.AssertI(13, a)

  // global pointer to function
  ---
    test.AssertI(2222, _addfp_(19, 2203))

  // TODO disable for now. because deref store uses set_global()
  // the result is written in global area not local register
  // referenced by the pointer
  // local pointer to function
  //---
  //  - twicefp = twice
  //  - xxxxxxxxxxxxxxxxxxxxxxxxxxxx = -128
  //  twicefp(&xxxxxxxxxxxxxxxxxxxxxxxxxxxx)
  //  test.AssertI(-256, xxxxxxxxxxxxxxxxxxxxxxxxxxxx)

  // array initialization
  ---
    - a = [99, 11, 22, 33 + 9]
    test.AssertI(64, a[2] + a[3])

  // struct initialization
  ---
    - p = Point { x = 99, y = 11 }
    test.AssertI(99, p.x)
    test.AssertI(11, p.y)

    - q = Point { y = p.x - 49, x = p.y * 2 }
    test.AssertI(22, q.x)
    test.AssertI(50, q.y)

    - r = Point {
      x = q.x - 9,
      y = q.y / 2
    }
    test.AssertI(13, r.x)
    test.AssertI(25, r.y)

    - s = Point { y = 49 }
    test.AssertI(0, s.x)
    test.AssertI(49, s.y)

  // struct default initialization
  ---
    - p Point
    test.AssertI(0, p.x)
    test.AssertI(0, p.y)

  // struct initialization with another struct
  ---
    - p = Point { x = 31, y = 79 }
    - q = p
    test.AssertI(31, q.x)
    test.AssertI(79, q.y)

  // global struct initialization
  ---
    test.AssertI(0,   _center_.x)
    test.AssertI(117, _center_.y)

  // global struct initialization
  ---
    test.AssertI(1178, _Q_.x)
    test.AssertI(-238, _Q_.y)

  // struct assignment
  ---
    - p = Point { x = -3, y = 1090 }
    test.AssertI(-3, p.x)
    test.AssertI(1090, p.y)

    - q = Point { y = 613 }
    test.AssertI(0, q.x)
    test.AssertI(613, q.y)

    q = p
    test.AssertI(-3, q.x)
    test.AssertI(1090, q.y)

  // struct parameter
  ---
    - p = Point { x = 319, y = -123 }
    test.AssertI(319, p.x)
    test.AssertI(-123, p.y)
    test.AssertI(-123, get_point_y(p))

  // struct reference parameter
  ---
    - p = Point { x = 37, y = 79 }
    test.AssertI(37, p.x)
    test.AssertI(79, p.y)

    set_point_x(&p, 19)
    test.AssertI(19, p.x)
    set_point_y(&p, 4299)
    test.AssertI(4299, p.y)

  //------------------------------
  print(test._test_count_, "tests done.")

  return 0
