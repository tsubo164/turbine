> gc

# main(args vec{string}) int
  - s = "foo"
  - v = set{"bar"}

  gc.print_objects()

  gc.request()

  for i in 0..2
    nop

  setadd(v, s + "bar")

  print("==============")
  gc.print_objects()
  print(">>>>>>>>>>>>", gc.get_stats().total_collections)
  print(v)

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
