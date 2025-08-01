> file
> test

# main(args vec{string}) int

  ---
    - s = "Hello, World!\n"
    test.AssertB(true, file.write_text("foo.out", s))

    - t = file.read_text("foo.out")
    test.AssertS(s, t)

  ---
    - lines = vec{"foo\n", "bar\n", "baz"}
    file.write_lines("foo.out", lines)

    - v = file.read_lines("foo.out")
    for i, l in v
      test.AssertS(lines[i], l)

  print(test._test_count_, "tests done.")

  return 0
