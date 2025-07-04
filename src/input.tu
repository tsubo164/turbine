> gc

# main(args vec{string}) int

  for i, val in vec{11, 22, 42}
    print(2 * val)

  for i in 0..10
    print(2 * i)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  return 0
