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

/*
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

  // * Foo
  //   print("* draw foo")

  // * others
  //   print("* others", sh)

  return 0
*/
> my_calc

# main(args vec{string}) int
  print(my_calc.add(2, 3))
  print("my_math version:", my_calc.my_math._VERSION_)
  - x = 5.0
  print("square of", x, "is", my_calc.square(x))
  print("ok")
  return 0
