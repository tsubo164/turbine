> gc

# getv(x int) vec{int}
  return vec{x, x, x}

# main(args vec{string}) int

  print(getv(42))
  - v = getv(1)
  print(v)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  return 0
