> math
> test

# main() int

  ---
    test.AssertF(3.141592653589793, math._PI_)
    test.AssertF(2.718281828459045, math._E_)
 
  ---
    - f = 25.0
    test.AssertF(5.0, math.sqrt(f))
    - x = 16.0
    test.AssertF(4.0, math.sqrt(x))
    x = 9.0
    test.AssertF(3.0, math.sqrt(x))

  ---
    - x = 9.0
    test.AssertF(81.0, math.pow(x, 2.0))
 
  ---
    - x = 2.0
    - y = 3.0
    test.AssertF(8.0, math.pow(x, y))

  ---
    - f1 = 3.0
    - f2 = 4.0
    test.AssertF(5.0, math.sqrt(math.pow(f1, 2.0) + math.pow(f2, 2.0)))

  ---
    - r = math.sin(math._PI_ / 2.0)
    test.AssertF(1.0, r)

  ---
    test.AssertB(true, math.isclose(0.7071067811865476, math.sin(math.radians(45.0))))
    test.AssertB(true, math.isclose(0.0, math.cos(math._PI_ / 2.0)))
    test.AssertF(-1.0, math.cos(math._PI_))
    test.AssertB(true, math.isclose(0.0, math.sin(math.radians(0.0))))
    test.AssertB(true, math.isclose(1.0, math.cos(math.radians(0.0))))
    test.AssertB(true, math.isclose(1.0, math.sin(math.radians(90.0))))
    test.AssertB(true, math.isclose(0.0, math.cos(math.radians(90.0))))
    test.AssertB(true, math.isclose(0.0, math.sin(math.radians(180.0))))
    test.AssertF(-1.0, math.cos(math.radians(180.0)))
    test.AssertB(true, math.isclose(-1.0, math.sin(math.radians(270.0))))
    test.AssertB(true, math.isclose(0.0, math.cos(math.radians(270.0))))
    test.AssertB(true, math.isclose(0.0, math.sin(math.radians(360.0))))
    test.AssertB(true, math.isclose(1.0, math.cos(math.radians(360.0))))

  ---
    // asin
    test.AssertB(true, math.isclose(0.0, math.degrees(math.asin(0.0))))
    test.AssertB(true, math.isclose(30.0, math.degrees(math.asin(0.5))))
    test.AssertB(true, math.isclose(90.0, math.degrees(math.asin(1.0))))
    test.AssertB(true, math.isclose(-30.0, math.degrees(math.asin(-0.5))))
    test.AssertB(true, math.isclose(-90.0, math.degrees(math.asin(-1.0))))

    // acos
    test.AssertB(true, math.isclose(0.0, math.degrees(math.acos(1.0))))
    test.AssertB(true, math.isclose(60.0, math.degrees(math.acos(0.5))))
    test.AssertB(true, math.isclose(90.0, math.degrees(math.acos(0.0))))
    test.AssertB(true, math.isclose(120.0, math.degrees(math.acos(-0.5))))
    test.AssertB(true, math.isclose(180.0, math.degrees(math.acos(-1.0))))

    // atan
    test.AssertB(true, math.isclose(0.0, math.degrees(math.atan(0.0))))
    test.AssertB(true, math.isclose(45.0, math.degrees(math.atan(1.0))))
    test.AssertB(true, math.isclose(-45.0, math.degrees(math.atan(-1.0))))
    test.AssertB(true, math.isclose(90.0, math.degrees(math.atan(math._INF_))))
    test.AssertB(true, math.isclose(-90.0, math.degrees(math.atan(-math._INF_))))

    // atan2
    test.AssertB(true, math.isclose(0.0, math.degrees(math.atan2(0.0, 1.0))))
    test.AssertB(true, math.isclose(45.0, math.degrees(math.atan2(1.0, 1.0))))
    test.AssertB(true, math.isclose(90.0, math.degrees(math.atan2(1.0, 0.0))))
    test.AssertB(true, math.isclose(-90.0, math.degrees(math.atan2(-1.0, 0.0))))
    test.AssertB(true, math.isclose(-135.0, math.degrees(math.atan2(-1.0, -1.0))))
    test.AssertB(true, math.isclose(135.0, math.degrees(math.atan2(1.0, -1.0))))

  ---
    test.AssertB(true, math.isclose(30.0, math.degrees(math._PI_ / 6.0)))
    test.AssertB(true, math.isclose(45.0, math.degrees(math._PI_ / 4.0)))
    test.AssertB(true, math.isclose(60.0, math.degrees(math._PI_ / 3.0)))
    test.AssertB(true, math.isclose(90.0, math.degrees(math._PI_ / 2.0)))
    test.AssertB(true, math.isclose(180.0, math.degrees(math._PI_)))
    test.AssertB(true, math.isclose(360.0, math.degrees(2.0 * math._PI_)))

  ---
    - v = math.Vec3 { x = 1.1, y = 2.2, z = 3.3 }
    test.AssertF(1.1, v.x)
    test.AssertF(2.2, v.y)
    test.AssertF(3.3, v.z)

    - a = math.Vec3 { x = 4.4, y = 5.5, z = 6.6 }
    test.AssertF(4.4, a.x)
    test.AssertF(5.5, a.y)
    test.AssertF(6.6, a.z)
 
    - b = math.Vec3 { x = -1.0, y = 0.0, z = 1.0 }
    test.AssertF(-1.0, b.x)
    test.AssertF(0.0, b.y)
    test.AssertF(1.0, b.z)

    - c = math.Vec3 { x = 0.0, y = -2.5, z = 2.5 }
    test.AssertF(0.0, c.x)
    test.AssertF(-2.5, c.y)
    test.AssertF(2.5, c.z)

  print(test._test_count_, "tests done.")

  return 0
