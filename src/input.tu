> gc
> math

/*
# main() int
  print(veclen(args))
*/
# main(args vec{string}) int
  - s = "foo"
  s = s + "oo"

  if s + "bar" == "baz"
    nop

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  print(s)
  print(veclen(args))

  /*
  - s = vec{1, 2, 3}
  s = vec{4, 5, 6}
  */

  return 0
