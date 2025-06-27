> gc
> math


# main(args vec{string}) int

  //- v = vec{"foo", "bar"}
  //v[0] += "123"
  //print(v)

  - m = map{"foo":"bar", "hello":"world"}
  m["foo"] += "123"
  print(m["foo"])

  /*
  - s = foo()
  - s = "foo"
  s = s + args[0]

  if s + "bar" == "baz"
    nop
  */

  if m["foo"] == "baz"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  /*
  */
  print(m["foo"])

  return 0
