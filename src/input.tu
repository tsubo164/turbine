> gc

## Foo struct
  - id int

## Person struct
  - first string
  - last string
  - id int

# main(args vec{string}) int

  - t = Person{first="Foo", last="Bar", id=232423}
  t.last += "-Baz"
  print(t)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(t)

  return 0
