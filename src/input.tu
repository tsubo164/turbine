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

- _G_ = 42

# foo(a int, &ok bool) int
  ok = true
  return 2 * a

# main(args vec{string}) int
  - ok bool

  print(ok)
  - i = 12
  - a = foo(i, &ok)
  //- a = foo(i, ok)
  //- a = foo(&i, &ok)
  //print(ok)

  - b = foo(i, &ok)

  return 0
