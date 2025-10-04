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

# main(args vec{string}) int
  - ok bool
  - a = foo(12, &ok)

  print(a)

  return 0
