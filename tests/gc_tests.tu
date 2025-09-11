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
    for i in 0..10
      nop
    test.AssertB(false, gc.is_object_alive(id))

  ---
    // GC stats
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

  ---
    // GC log
    - s = "Bar"

    // temp object creation
    if "Foo" + s == "FooBar"
      nop

    // force collect
    gc.collect()

    - log = gc.get_log()
    - len = veclen(log)
    - last = log[len - 1]

    test.AssertB(true, last.used_bytes_after < last.used_bytes_before)
    test.AssertB(true, last.trigger_reason == gc.Reason.USER)

  ---
    // GC trigger by threshold
    for i in 0..100000
      - v = vec{i}

    - log = gc.get_log()
    - len = veclen(log)
    - last = log[len - 1]

    test.AssertB(true, last.used_bytes_after < last.used_bytes_before)
    test.AssertB(true, last.trigger_reason == gc.Reason.THRESHOLD)

  ---
    // write barrier for vec
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - v = vec{"bar"}

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    v[0] = s + "bar"
    - id = gc.get_object_id(v[0])

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)
    test.AssertB(true, gc.is_object_alive(id))

  ---
    // write barrier for vecpush
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - v = vec{"bar"}

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    vecpush(v, s + "bar")
    - id = gc.get_object_id(v[1])

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)
    test.AssertB(true, gc.is_object_alive(id))

  ---
    // write barrier for map
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - m = map{"bar":"BAR"}
    test.AssertS("BAR", m["bar"])

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    m["bar"] = s + "bar"
    - id = gc.get_object_id(m["bar"])

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)
    test.AssertB(true, gc.is_object_alive(id))
    test.AssertS("foobar", m["bar"])

  ---
    // write barrier for set
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - v = set{"bar"}

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    setadd(v, s + "bar")

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)

  ---
    // write barrier for stack
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - v = stack{"bar"}

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    stackpush(v, s + "bar")

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)

  ---
    // write barrier for queue
    - total_before = gc.get_stats().total_collections
    - s = "foo"
    - v = queue{"bar"}

    // request GC
    gc.request()

    // do some steps (finish root scans)
    for i in 0..2
      nop

    // write white ref to black obj
    queuepush(v, s + "bar")

    // finish GC
    for i in 0..2
      nop

    // make sure GC finished
    - total_after = gc.get_stats().total_collections
    test.AssertB(true, total_after > total_before)

  print(test._test_count_, "tests done.")

  return 0
