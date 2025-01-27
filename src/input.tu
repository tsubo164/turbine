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
# foo($caller_line) int
  return $caller_line

# main(args []string) int
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  foo()
  print(format("%d", 42.0))
  return 42
