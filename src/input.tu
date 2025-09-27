## MyError enum
  : tag,          message
  - None,         "no error"
  - FileNotFound, "file not found"

/*
# main(args vec{string}) int
  - e = MyError.FileNotFound

  print(e.message)
  print(e)
  print(e.tag)

  return 0
*/

## Color enum
  : symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

# foo(c Color)
  print(c)

# main(args vec{string}) int

  - c = Color.G
  print(c)
  print(c.name)
  foo(c)

  for c in Color
    print(">>", c.name)

  return 0
