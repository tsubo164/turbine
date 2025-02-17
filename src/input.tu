//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]
//> math

# main(args []string) int
  - q = queue{11, 22, 33}

  queuepush(q, 44)
  queuepush(q, 55)
  queuepush(q, 66)
  queuepush(q, 77)
  queuepush(q, 88)

  print("...", queuepop(q))
  print("...", queuepop(q))
  queuepush(q, -9223)

  for i, val in q
    print(i, val)

  print(q)

  return 42
