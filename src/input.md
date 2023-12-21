# foo()
  print("foo!")
  return

# main() int
  print("Hello, World!", 42, 3.14, nil)
  foo()
  - i = 9
  print("body block i:", i)
  ---
    - i = 42
    print("nested block i:", i)
    nop
    ---
      - k = 7
      print(">>> nested block k:", k)

    ---
      - z = 8
      print(">>> nested block z:", z)

  print(i)
  - j = 99
  print(j)

  - k = 2.7
  print(k)

  return 42
