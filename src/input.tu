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

# foo()
  nop

# main(args []string) int
  //- a = 33
  //- f = foo

  - i = 42

  /*
  if i == 41
    return 3
  elif i == 99
    nop
    return 6
  else
    return i
  */
  while i < 50
    print(i, "Foo")
    i += 1
    //return i


  return i
