# foo()
  print("foo!")
  return

# main() int
  print("Hello, World!", 42, 3.14, nil)
  foo()
  - i = 9
  //print(i)
  ---
    - i = 42
    print(i)
    nop

  print(i)
  return 42
