//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]
//> math

## Point struct
  - x int
  - y int

# main(args vec{string}) int
  /*
  - v = vec{11, 22, 33}
  - v = vec{vec{11, 22}, vec{33, 44}}
  - v vec{int}
  - v vec{int}

  - v = vec{
    Point{x=111, y=222},
    Point{x=333, y=444}
  }

  - w vec{int}
  - v = vec{w}
  */
  - v vec{vec{int}}

  print(v)

  return 42
