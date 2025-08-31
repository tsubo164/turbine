> gc

# main(args vec{string}) int
  /*
  - s = "foo"
  - m = map{"bar":"BAR", "zoo":"ZOO"}
  print(m)
  m["bar"] = s + "FOO"
  print(m)
  */

  //- m map{int}
  //- i = 3
  //- i = -1212
  //print(i)
  //print(m)
  //m["foo"] = 42
  //m["bar"] = -1212
  //print(m)
  //- i int
  - i = 0x3A9
  print(i & 0xFF)
  //print(i)

  /*
  gc.print_objects()

  gc.request()

  for i in 0..2
    nop

  m["bar"] = s + "FOO"

  print("==============")
  gc.print_objects()
  print(">>>>>>>>>>>>", gc.get_stats().total_collections)
  print(m["bar"])
  */

  return 0

/*
# main(args vec{string}) int
  - s = "foo"
  - v = vec{"bar"}

  gc.print_objects()

  gc.request()

  for i in 0..2
    nop

  v[0] = s + "bar"

  print("==============")
  gc.print_objects()
  print(">>>>>>>>>>>>", gc.get_stats().total_collections)
  print(v[0])

  return 0
*/
