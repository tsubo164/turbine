//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
[math]

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

:: Color
  | symbol | name    | val
  | ---    | ---     | ---
  | R      | "red"   | -42
  | G      | "green" | 99
  | B      | "blue"  | 4095
  | A      | "alpha" | 42

/*
:: Color
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

:: Color
  - symbol {name    , val}
  - R      {"red"   , 42}
  - G      {"green" , 99}
  - B      {"blue"  , 4095}
  - A      {"alpha" , 42}

:: Color
  * symbol , name    , num
  * R      , "red"   , 42
  * G      , "green" , 42

:: Color
  : symbol : name   : num
  : ---    : ---    : ---
  : R      : "red"  : 42

:: Color
  | symbol | name    | num
  : R      : "red"   : 42
  : G      : "green" : 42

:: Color
  | symbol | name   | num
  | ---    | ---    | ---
  | R      | "red"  | (42 | 0x0F)
*/

# main(args []string) int
  - a = 32
  - p Point
  //print(42, 3.14, "foo", Color.B)
  print(Color.B)
  print(p.x, math._PI_)
  //print(42, 3.14, "foo", Color.B.val)
  return 42
