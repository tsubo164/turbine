//> gc
//
//## Foo struct
//  - a int
//
////## Foo struct
////  - b int
//
////# print()
////  nop
//
//# main() int
//  - a = 42
//
//  gc.print()
//
//  - f = Foo{a = 9}
//
//  print(f)
//
//  return 0
# main() int
  - i = 42
  - f = 3.14
  //- s = "Hello"

  f = f + float(i)

  print(f)

  return 0
