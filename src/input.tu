> gc
> math

//## Name struct
//  - first string
//  - last string

# main(args vec{string}) int

  - x = "baz"
  - v = vec{"foo", "bar" + x}
  print(v)

  if v["foo"] + "baz" == "**"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(v)

  return 0
