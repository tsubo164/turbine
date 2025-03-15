> file
> test

# main() int

  ---
    - s = "Hello, World!\n"
    test.AssertB(true, file.write_text("foo.out", s))

    - t = file.read_text("foo.out")
    test.AssertS(s, t)

  print(test._test_count_, "tests done.")

  return 0
