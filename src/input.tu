//[os]
//[csv]
//[json]
//[opengl] (github.com/...)
//[my_calc]
//[math]
// > math

# main(args []string) int
  - s = set{"foo", "bar", "baz", "aaa"}

  for val in s
    print(val)

  print(s)

  setremove(s, "foo")
  setremove(s, "bar")
  setremove(s, "baz")
  setremove(s, "aaa")

  return 42
