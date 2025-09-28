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
