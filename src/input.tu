> gc

## Person struct
  - first string
  - last string
  - age int
  - a int
  - b int
  - c int
  - d int
  - e int
  - f int

# foo()
  nop

# main(args vec{string}) int
  - s = "Hoge"
  - p = Person{first="Foo", last="Bar", age=42}
  print(p)

  // temp object creation
  if p.last + s == "HOGE"
    nop

  print("before ===============================")
  gc.print()
  // request gc
  gc.collect()

  // safepoint at loop back edge
  - i = 0
  while i < 10
    i += 1

  // safepoint at function entry point
  //foo()

  // safepoint at loop back edge
  //for i in 0..10000
  //  - v = vec{i}
  //  print(i, v)

  print("after  ===============================")
  gc.print()

  print(p)

  return 0
