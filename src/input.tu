> gc
> math


//# func(a string)
//  - s = set{a + ", Smith"}
//  print("=>", s)

# main(args vec{string}) int

  - x = "baz"
  - s = set{"foo", "bar" + x}
  print(s)

  //if s["foo"] == "baz"
  //  nop

  //func("John")

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(s)

  return 0
