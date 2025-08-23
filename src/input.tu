> gc

# foo(s string)
  - v = vec{s + "bar"}

# main(args vec{string}) int
  foo("foo")

  gc.print_objects()

  gc.request()

  for i in 0..10
    nop

  print("==============")
  gc.print_objects()

  return 0
