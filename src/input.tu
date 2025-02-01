//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]

## Color enum
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

//# foo($caller_line) int
//  return $caller_line

# foo()
  nop

# main(args []string) int
  //- a = 33
  //- f = foo

  - i = 42

  - c = Color.G

  switch c
  case Color.B
    print("blue")
  case Color.R
    print("foo")
  case Color.G
    print("boo")
  default
    print("baz")

  return i
