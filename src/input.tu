//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]

//:: Token
//  | symbol   | name
//  | ---      | ---
//  | T_IF     | "if"
//  | T_FOR    | "for"
//  | T_ELS    | "or"
//  | T_BRK    | "break"

## Point
  - x int
  - y int

//:: Color
//  | symbol | name    | val
//  | ---    | ---     | ---
//  | R      | "red"   | 42
//  | G      | "green" | 99
//  | B      | "blue"  | 4095
//  | A      | "alpha" | 42

/*
:: Color
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

:: Color
  | symbol | name   | num
  | ---    | ---    | ---
  | R      | "red"  | (42 | 0x0F)

:: Color
  : symbol : name   : num
  : ---    : ---    : ---
  : R      : "red"  : 42

:: Color
  | symbol | name    | num
  : R      : "red"   : 42
  : G      : "green" : 42

:: Color
  * symbol , name    , num
  * R      , "red"   , 42
  * G      , "green" , 42
*/

# main(args []string) int
  //- p Point
  - q = Point{x=11, y=22}
  //- c Color
  //- d = Color.A

  //-s = d.name
  //- a = 0xFFFFFFFFFF
  //- h = "Hello!"

  //- x = d == Color.G
  //- x = p.x

  //print(s)
  //print(d.val)
  //print(Color.R.name)
  //print(Color.G)

  // TODO
  - x = q.t
  print(x)

  return 42
