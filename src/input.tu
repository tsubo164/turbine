//# foo(f int)
//  nop

# main(args vec{string}) int
  - v = stack{"bar"}

  //foo(1)


  //setadd(v, "foo")

  -s = stackpop(v)

  print(s)

  return 0
