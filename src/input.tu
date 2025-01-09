//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]

//:: token
//  | enum  | name
//  | ---   | ---
//  | T_IF  | "if"
//  | T_FOR | "for"
//  | T_ELS | "or"
//  | T_BRK | "break"

## Point
  - x int
  - y int

## Circle
  - center Point
  - radius int
  - samples []int

# main(args []string) int
  - m {}int

  m["foo"] = 42
  //m["bar"] = 12
  print("m[\"foo\"] =>", m["foo"])
  //print(400 - 300)

  return 42
  //- a = 42
  //- fp = exit
  //- b = 242301
  //- c = 0x1234567890
  //- d = true
  //- f = 3.14
  //- s = "Hello!"
  //print(a, b, c, d, f, s)
  //print()
  //return a
