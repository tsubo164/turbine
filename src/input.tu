> time

# main(args vec{string}) int

  - start = time.now()

  //for i in 0..10000000
  //  -j = 100 / 234
  - sec = 1.5

  print("sleeping", sec, "seconds...")
  time.sleep(sec)

  //- u = time.now()
  //print(u - t)
  print("elapsed ", time.elapsed(start), "seconds")


  return 42
