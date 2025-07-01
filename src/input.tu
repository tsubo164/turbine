/*
> gc
> math

//## Name struct
//  - first string
//  - last string

# main(args vec{string}) int

  for i in vec{11, 22}
    print(i)

  print("before ===============================")
  gc.print()
  gc.collect()
  print("after  ===============================")
  gc.print()

  return 0

*/
/*
# add(a int, b int) int
  return a + b

# main(args vec{string}) int
  //- c = add(42, 13)
  return 0
*/

# seven() int
  return 7

# add(x int, y int) int
  return x + y

# main() int
  //return seven()// + add(30, 5)
  //return add(30, 5)
  return 42
