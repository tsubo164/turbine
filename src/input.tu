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

//## Point
//  - x int
//  - y int
//
//## Circle
//  - center Point
//  - radius int
//  - samples []int

# main(args []string) int
  /*
  - m = {
    "foo":42,
    "bar":1212,
    "baz":284,
    "Go":923,
    "Nim":1736,
    "Zig":4812,
    "C/C++":361,
    "Bash":5792,
    "Rust":814,
    "Lua":1453,
    "Markdown":2678,
    "Toml":3921,
    "Yaml":837,
    "Java":5293,
    "Kotlin":1847,
    "Dart":615,
    "Lisp":7432,
    "Python":4261,
    "Ruby":519,
    "Perl":682,
    "PHP":-832,
    "JavaScript":7432,
    "Swift":3921,
    "Turbine":3574
  }
  print(m)
  print(maplen(m))

  for i, key, val in m
    print(format("%3d: %10s => %d", i, key, val))
  */

  //for i, key, val in m
  //for _, _, val in m
  //for , , val in m
  - keysum = ""
  print(keysum)
  keysum += "Hello"
  print(keysum)
  keysum += ", World!"
  print(keysum)

  return 42
