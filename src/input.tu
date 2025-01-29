//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
[math]

## Point struct
  - x float
  - y float

## Color enum
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

//# foo($caller_line) int
//  return $caller_line

# main(args []string) int
  //- a = 33
  //- f = foo
  return Color.G.val
