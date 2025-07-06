> gc

# main(args vec{string}) int

  - s = "hello"
  - t = stack{"foo", "bar"}
  stackpush(t, s + "-world")
  print(t)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(stacktop(t))

  return 0
