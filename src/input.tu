> math
> gc

//# foo()
//  nop

# main(args vec{string}) int
  - stat gc.Stat
  - s = "Hoge"

  // temp object creation
  if "Foo" + s == "HOGE"
    nop

  stat = gc.get_stats()
  print("before total_collections:", stat.total_collections)
  print(stat)

  gc.collect()

  // safepoint
  for i in 0..1
    nop

  stat = gc.get_stats()
  print("after  total_collections:", stat.total_collections)
  print(stat)

  gc.print_objects()
  gc.print_stats()
  //- f = foo()

  return 0
