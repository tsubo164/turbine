> test
> gc

// make an object
# make(arg string) int
  - s = arg + "???"
  return gc.get_object_id(s)

# main(args vec{string}) int

  ---
    - id = make("Hello")
    test.AssertB(true, gc.is_object_alive(id))

    // request collect
    gc.collect()

    // safepoint at loop back edge
    for i in 0..1
      nop
    test.AssertB(false, gc.is_object_alive(id))

  print(test._test_count_, "tests done.")

  return 0
