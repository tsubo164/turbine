> time

# main(args vec{string}) int

  - start = time.now()
  - p = time.perf()

  //for i in 0..10000000
  //  -j = 100 / 234
  - sec = 0.85

  print("sleeping", sec, "seconds...")
  time.sleep(sec)

  //- u = time.now()
  //print(u - t)
  print("elapsed ", time.elapsed(start), "seconds")
  print("elapsed ", time.perf() - p, "seconds (perf)")


  return 42
