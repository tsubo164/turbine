//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]
// > math

# main(args []string) int
  //- s = set{11, 22, 33}
  - s set{int}

  //setadd(s, 5)
  //setadd(s, 3)
  //setadd(s, 1)

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

  return 42
