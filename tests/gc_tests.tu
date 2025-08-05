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
    gc.request()

    // safepoint at loop back edge
    for i in 0..1
      nop
    test.AssertB(false, gc.is_object_alive(id))

  ---
    - before gc.Stat
    - s = "Bar"

    // temp object creation
    if "Foo" + s == "FooBar"
      nop

    before = gc.get_stats()
    // force collect
    gc.collect()
    - after = gc.get_stats()

    test.AssertB(true, before.total_collections < after.total_collections)
    test.AssertB(true, before.used_bytes > after.used_bytes)

  print(test._test_count_, "tests done.")

  return 0
