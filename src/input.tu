/*
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
*/

/*
# add(a int, b int) int
  return a + b

# main(args vec{string}) int
  - c = add(42, 13)
  return 0
*/

# main(args vec{string}) int
  //- v = vec{"foo", "bar"}
  ////print(v)
  //print(v["foo"])
  - v = map{"foo":"bar"}
  //print(v)
  print(v[2])
  return 0
