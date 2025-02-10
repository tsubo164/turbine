//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]
// > math

# main(args []string) int
  - m = {"Foo":13}
  - s = set{11, 22, 33}
  //- s set{int}

  //print(">>>>> len:", setlen(s))
  //print("constains:", setcontains(s, 11))
  //print("constains:", setcontains(s, 22))
  //print("constains:", setcontains(s, 33))
  for val in m
    print(val)

  - x = -99

  for val in s
    print(val)

  /*
  setadd(s, 5)
  setadd(s, 7)
  setadd(s, 9)
  setadd(s, 1)
  setadd(s, 3)
  setadd(s, 4)
  setadd(s, 10)
  setadd(s, 20)
  setadd(s, 30)
  setadd(s, 40)
  setadd(s, 50)
  setadd(s, 60)

  print(setcontains(s, 4))
  setremove(s, 4)
  print(setcontains(s, 4))
  setremove(s, 9)
  setremove(s, 10)
  setremove(s, 1)
  setremove(s, 5)
  print("------", setremove(s, 3))
  print("------", setremove(s, 3))
  print("------", setremove(s, 111))
  print("------", setremove(s, 40))
  */

  return 42
