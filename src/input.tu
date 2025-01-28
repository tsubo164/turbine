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

//:: Color
//  | symbol | name    | val
//  | ---    | ---     | ---
//  | R      | "red"   | -42
//  | G      | "green" | 99
//  | B      | "blue"  | 4095
//  | A      | "alpha" | 42

## Point //struct
  - x float
  - y float

//## Color //enum
:: Color //enum
  - symbol , name    , val
  - R      , "red"   , 42
  - G      , "green" , 99
  - B      , "blue"  , 4095
  - A      , "alpha" , 42

/*
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
//# foo($caller_line) int
//  return $caller_line

# main(args []string) int
  //- a = 33
  //- f = foo
  return 42
