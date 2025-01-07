[test]
[math]

## Pos
  - x int
  - y int

## Point
  - x float
  - y float

# length(p Point) float
  return math.sqrt(p.x * p.x + p.y * p.y)

## Circle
  - center Pos
  - radius int
  - samples []int

# main(args []string) int

  ---
    // struct literal
    - p = Point { x = 3.0, y = 4.0}
    - l = length(p)
    test.AssertF(5.0, l)
    test.AssertF(5.0, length(Point { x = 3.0, y = 4.0}))

    - q = Point{}
    test.AssertF(0.0, q.x)
    test.AssertF(0.0, q.y)

    - r = Point{y=1.1, x=2.2}
    test.AssertF(2.2, r.x)
    test.AssertF(1.1, r.y)

  ---
    // field access
    - p = Point { x = 3.0, y = 4.0}
    p.x += 3.3
    test.AssertF(6.3, p.x)
    p.y *= 1.9
    test.AssertF(7.6, p.y)
    p.x /= 2.0
    test.AssertF(3.15, p.x)
    p.y -= 4.5
    //test.AssertF(3.1, p.y)
    p.x += 3.0 * p.x - 1.3 * p.y
    test.AssertF(8.57, p.x)

  ---
    // array of struct
    - positions = [Pos{}]
    test.AssertI(0, positions[0].x)
    test.AssertI(0, positions[0].y)
    positions[0].x = 123
    positions[0].y = -91
    test.AssertI(123, positions[0].x)
    test.AssertI(-91, positions[0].y)

  ---
    // nested struct default initializer
    - c Circle
    test.AssertI(0, c.center.x)
    test.AssertI(0, c.center.y)
    test.AssertI(0, c.radius)
    test.AssertI(0, len(c.samples))

  print(test._test_count_, "tests done.")

  return 0
