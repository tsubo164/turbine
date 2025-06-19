> gc
> math

- _S_ = "Hello"

# main(args vec{string}) int
  - f = math._PI_
  - x = _S_

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
  print(x)

  return 0
