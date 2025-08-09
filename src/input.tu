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

  //==============
  - log = gc.get_log()

  print(veclen(log))

  - last = log[0] //=> seg fault => panic?

  print("last triggered_addr:", last.triggered_addr)
  print("last used_bytes_before:", last.used_bytes_before, "bytes")
  print("last used_bytes_after: ", last.used_bytes_after, "bytes")

  return 0
