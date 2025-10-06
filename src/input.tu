/*
## MyError enum
  : tag,          message
  - None,         "no error"
  - FileNotFound, "file not found"

# main(args vec{string}) int
  - e = MyError.FileNotFound

  print(e.message)
  print(e)
  print(e.tag)

  return 0
*/

# foo(a int, &ok bool) int
  ok = true
  return 2 * a

# bar(&a int)
  a = 42
  a += 8
  print(a)
  a -= 10

# main(args vec{string}) int
  - ok bool

  print(ok)
  - a = foo(12, &ok)
  print(ok)

  print(a)
  bar(&a)
  print(a)


  return 0
