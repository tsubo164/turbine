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

## Shape enum
  : tag
  - Line
  - Circle
  - Rectangle
  - Triangle

# main(args vec{string}) int
  - sh = Shape.Line
  sh = Shape.Rectangle

  switch sh
  * Line
    print("* draw line")

  * Circle
    print("* draw circle")

  //* Foo
  //  print("* draw foo")

  //* others
  //  print("* others", sh)

  return 0
