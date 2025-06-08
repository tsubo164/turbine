> gc

# main() int
  - s = "foo"
  //s = s + "oo"

  if s + "bar" == "foo"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  //print(s)

  return 0
