> gc
> math


//# func(a string)
//  - s = set{a + ", Smith"}
//  print("=>", s)
## Name struct
  - first string
  - last string

# main(args vec{string}) int

  - x = "baz"
  - s = Name{first="foo", last="bar" + x}
  print(s)

  //if s.first + x == "John"
  if s.last == "John"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  //print(s)

  return 0
