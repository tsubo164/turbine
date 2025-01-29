//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]

//## Color enum
//  - symbol , name    , val
//  - R      , "red"   , 42
//  - G      , "green" , 99
//  - B      , "blue"  , 4095
//  - A      , "alpha" , 42

//# foo($caller_line) int
//  return $caller_line

# main(args []string) int
  //- a = 33
  //- f = foo

  - i int
  if 13 == 12
    i = 42
  elif 12 == 12
    i = 77
  else
    i = 99

  return i
