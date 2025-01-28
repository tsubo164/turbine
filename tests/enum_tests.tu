[test]

:: Color
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

# main(args []string) int

  ---
    - c = Color.A
    - s = c.name
    test.AssertS("alpha", s)
    test.AssertI(42, c.val)
    test.AssertI(42, Color.R.val)
    test.AssertI(4095, Color.B.val)

  print(test._test_count_, "tests done.")

  return 0
