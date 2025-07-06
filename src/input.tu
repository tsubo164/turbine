> gc

# main(args vec{string}) int

  - s = "hello"
  - t = set{"foo", "bar"}
  setadd(t, s + ", world!")
  print(setcontains(t, "hello, world!"))
  print(t)

  /*
  for i, val in vec{11, 22, 42}
    print(2 * val)

  for i in 0..10
    print(2 * i)
  */

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(setcontains(t, "hello, world!"))

  return 0
