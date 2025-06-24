> gc
> math


# foo() string
  - v = vec{"foo", "bar"}
  return v[0]
/*
*/

# main(args vec{string}) int
  /*
  - s = foo()
  - s = "foo"
  s = s + args[0]

  if s + "bar" == "baz"
    nop
  */

  /*
  - s = args[0]
  print(s)
  */

  if foo() == "baz"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  //print(s)

  return 0
