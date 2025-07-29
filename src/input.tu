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

# main(args vec{string}) int
  - s = "Hoge"
  - p = Person{first="Foo", last="Bar", age=42}
  print(p)

  // temp object creation
  if p.last + s == "HOGE"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()

  // safepoint at loop back edge
  for i in 0..10
    print("Foo", i)

  print("after  ===============================")
  gc.print()

  print(p)

  return 0
