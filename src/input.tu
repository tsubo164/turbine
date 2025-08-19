> gc

/*
# foo(s string)
  - v = vec{s + "bar"}

# main(args vec{string}) int
  foo("foo")

  gc.print_objects()

  gc.collect()

  print("==============")
  gc.print_objects()

  return 0
*/

# main(args vec{string}) int
  - stat gc.Stat
  stat = gc.get_stats()
  gc.collect()

  return 0
