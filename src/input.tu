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

//## Point
//  - x int
//  - y int
//
//:: Color
//  | symbol | name    | val
//  | ---    | ---     | ---
//  | R      | "red"   | -42
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

//# twice(a int) int
//  return 2 * a

# main(args []string) int
  //- b = !false
  //- b = false || true
  //- c = true && true
  //- d = false && true
  - d = 1 ^ 0
  - e = 3 < 1
  print(e)

  //print(b)
  //print(c)
  //print(d)
  //- a = 7
  return 42
  //return 42 + (3 * 5 + a * 2)
  //print(Color.R.val)
  //print(2 + 3 + 11 - a)
  //return twice(a + (3 * 9 + 11))
