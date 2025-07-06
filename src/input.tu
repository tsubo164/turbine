> gc

# main(args vec{string}) int

  - s = "hello"
  - t = queue{"foo", "bar"}
  queuepush(t, s + "-world")
  print(t)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  //print(queuepop(t))
  print(t)

  return 0
