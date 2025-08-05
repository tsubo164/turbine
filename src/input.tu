> gc

# main(args vec{string}) int
  - stat gc.Stat
  - s = "Bar"

  // temp object creation
  if "Foo" + s == "FooBar"
    nop

  stat = gc.get_stats()
  print("before total_collections:", stat.total_collections)
  print(stat)

  gc.collect()
  //gc.request()

  stat = gc.get_stats()
  print("after  total_collections:", stat.total_collections)
  print(stat)

  gc.print_objects()
  gc.print_stats()

  return 0
