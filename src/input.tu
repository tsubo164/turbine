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
  for i in 0..10
    print(i, "Foo")
    //return i
    if i == 9
      return i
    else
      nop
      return i - 1


  //return i
