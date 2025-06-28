> gc
> math


# x(a string, b string)
  - m = map{a: b}
  print(a, "=>", b)

# main(args vec{string}) int

  //- v = vec{"foo", "bar"}
  //v[0] += "123"
  //print(v)

  - m = map{"foo":"bar", "hello":"world"}
  m["foo"] += "123"
  m["hello"] += "456"
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

  x("John", "Smith")

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  /*
  */
  print(m["foo"])

  return 0
