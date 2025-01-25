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
  - f = 1.1 + 3.2
  print(f)
  print(Color.R.val)
  return 42
