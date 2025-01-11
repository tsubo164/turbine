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
  m["bar"] = 1212
  m["baz"] = 284
  m["Go"] = 923
  m["Nim"] = 1736
  m["Zig"] = 4812
  m["C/C++"] = 361
  m["Bash"] = 5792
  m["Rust"] = 814
  m["Lua"] = 1453
  m["Markdown"] = 2678
  m["Toml"] = 3921
  m["Yaml"] = 837
  m["Java"] = 5293
  m["Kotlin"] = 1847
  m["Dart"] = 615
  m["Lisp"] = 7432
  m["Python"] = 4261
  m["Ruby"] = 519
  m["Perl"] = 682
  m["Turbine"] = 3574
  //print(m)
  print("------------>", m["foo"], m["bar"])

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
