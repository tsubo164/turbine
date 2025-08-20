> gc

# foo(s string)
  - v = vec{s + "bar"}

# main(args vec{string}) int
  foo("foo")

  gc.print_objects()

  gc.collect()

  print("==============")
  gc.print_objects()

  /*
  - log = gc.get_log()
  if veclen(log) > 0
    - last = log[veclen(log) - 1]
    //print(last.duration_msec)
  */

  return 0
/*

# main(args vec{string}) int
  - stat gc.Stat
  stat = gc.get_stats()
  gc.collect()

  return 0
*/
