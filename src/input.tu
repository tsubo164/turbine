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

:: Color
  | symbol | name    | num
  | ---    | ---     | ---
  | R      | "red"   | 42
  | G      | "green" | 99
  | B      | "blue"  | 4095

/*
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
  //- q = Point{x=11, y=22}
  //- c Color
  - d = Color.B

  -s = d.name

  //- x = d == Color.G
  //- x = p.x

  // TODO
  //- i int = 3.1
  //- p Vec
  //- x = p.t

  //print(Color.B)

  return 42
