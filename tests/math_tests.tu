> math
> test

# main(args vec{string}) int

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
    // abs  
    test.AssertB(true, math.isclose(5.0, math.abs(5.0)))
    test.AssertB(true, math.isclose(5.0, math.abs(-5.0)))
    test.AssertB(true, math.isclose(0.0, math.abs(0.0)))
  
    // floor  
    test.AssertB(true, math.isclose(5.0, math.floor(5.7)))
    test.AssertB(true, math.isclose(-6.0, math.floor(-5.7)))
    test.AssertB(true, math.isclose(0.0, math.floor(0.9)))
  
    // ceil  
    test.AssertB(true, math.isclose(6.0, math.ceil(5.2)))
    test.AssertB(true, math.isclose(-5.0, math.ceil(-5.8)))
    test.AssertB(true, math.isclose(1.0, math.ceil(0.1)))
  
    // round  
    test.AssertB(true, math.isclose(5.0, math.round(4.6)))
    test.AssertB(true, math.isclose(-6.0, math.round(-5.5)))
    test.AssertB(true, math.isclose(0.0, math.round(0.4)))

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
    // sinh
    test.AssertB(true, math.isclose(0.0, math.sinh(0.0)))
    test.AssertB(true, math.isclose(1.1752011936438014, math.sinh(1.0)))
    test.AssertB(true, math.isclose(-1.1752011936438014, math.sinh(-1.0)))
    test.AssertB(true, math.isclose(3.626860407847019, math.sinh(2.0)))

    // cosh
    test.AssertB(true, math.isclose(1.0, math.cosh(0.0)))
    test.AssertB(true, math.isclose(1.5430806348152437, math.cosh(1.0)))
    test.AssertB(true, math.isclose(3.7621956910836314, math.cosh(2.0)))

    // tanh
    test.AssertB(true, math.isclose(0.0, math.tanh(0.0)))
    test.AssertB(true, math.isclose(0.7615941559557649, math.tanh(1.0)))
    test.AssertB(true, math.isclose(-0.7615941559557649, math.tanh(-1.0)))
    test.AssertB(true, math.isclose(0.9640275800758169, math.tanh(2.0)))

    // asinh
    test.AssertB(true, math.isclose(0.0, math.asinh(0.0)))
    test.AssertB(true, math.isclose(1.0, math.asinh(math.sinh(1.0))))
    test.AssertB(true, math.isclose(-1.0, math.asinh(math.sinh(-1.0))))
    test.AssertB(true, math.isclose(2.0, math.asinh(math.sinh(2.0))))

    // acosh
    test.AssertB(true, math.isclose(0.0, math.acosh(1.0)))
    test.AssertB(true, math.isclose(1.3169578969248166, math.acosh(2.0)))
    test.AssertB(true, math.isclose(2.0634370688955608, math.acosh(4.0)))

    // atanh
    test.AssertB(true, math.isclose(0.0, math.atanh(0.0)))
    test.AssertB(true, math.isclose(0.5493061443340548, math.atanh(0.5)))
    test.AssertB(true, math.isclose(-0.5493061443340548, math.atanh(-0.5)))
    test.AssertB(true, math.isclose(0.8673005274441012, math.atanh(0.7)))

  ---
    // exp
    test.AssertB(true, math.isclose(1.0, math.exp(0.0)))
    test.AssertB(true, math.isclose(2.718281828459045, math.exp(1.0)))
    test.AssertB(true, math.isclose(7.38905609893065, math.exp(2.0)))
    test.AssertB(true, math.isclose(0.36787944117144233, math.exp(-1.0)))

    // log
    test.AssertB(true, math.isclose(0.0, math.log(1.0)))
    test.AssertB(true, math.isclose(1.0, math.log(math._E_)))
    test.AssertB(true, math.isclose(2.0, math.log(math._E_ * math._E_)))
    test.AssertB(true, math.isclose(-1.0, math.log(1.0 / math._E_)))

    // log10
    test.AssertB(true, math.isclose(0.0, math.log10(1.0)))
    test.AssertB(true, math.isclose(1.0, math.log10(10.0)))
    test.AssertB(true, math.isclose(2.0, math.log10(100.0)))
    test.AssertB(true, math.isclose(-1.0, math.log10(0.1)))

    // log2
    test.AssertB(true, math.isclose(0.0, math.log2(1.0)))
    test.AssertB(true, math.isclose(1.0, math.log2(2.0)))
    test.AssertB(true, math.isclose(3.0, math.log2(8.0)))
    test.AssertB(true, math.isclose(-1.0, math.log2(0.5)))

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

  ---
    // Type in module
    - v math.Vec3
    v.x = 11.1
    v.y = 22.2
    v.z = 33.3
    test.AssertF(11.1, v.x)
    test.AssertF(22.2, v.y)
    test.AssertF(33.3, v.z)

  print(test._test_count_, "tests done.")

  return 0
