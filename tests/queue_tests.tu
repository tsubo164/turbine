> test

# main(args vec{string}) int

  ---
    - q queue{int}
    test.AssertI(0, queuelen(q))
    test.AssertB(true, queueempty(q))

    queuepush(q, 11)
    test.AssertI(1, queuelen(q))
    queuepush(q, 22)
    test.AssertI(2, queuelen(q))
    queuepush(q, 33)
    test.AssertI(3, queuelen(q))

    test.AssertB(false, queueempty(q))

    test.AssertI(11, queuepop(q))
    test.AssertI(2,  queuelen(q))
    test.AssertI(22, queuepop(q))
    test.AssertI(1,  queuelen(q))
    test.AssertI(33, queuepop(q))
    test.AssertI(0,  queuelen(q))
    test.AssertI(0,  queuepop(q))
    test.AssertI(0,  queuelen(q))

  ---
    - q = queue{"foo", "bar", "baz"}
    test.AssertI(3, queuelen(q))

    queuepush(q, "hello")
    queuepush(q, "world")
    queuepush(q, "aaa")
    queuepush(q, "bbb")
    queuepush(q, "ccc")
    test.AssertI(8, queuelen(q))

    test.AssertS("foo", queuepop(q))
    test.AssertS("bar", queuepop(q))
    test.AssertI(6, queuelen(q))
    test.AssertS("baz", queuepop(q))
    test.AssertI(5, queuelen(q))

  ---
    - s = queue{"foo", "bar", "baz", "zzz"}
    - t string

    for val in s
      t = t + val
    test.AssertS("foobarbazzzz", t)
    test.AssertI(4, queuelen(s))

    - sum = 0
    - i = 0
    for val in s
      sum += i
      i += 1
    test.AssertI(6, sum)
    test.AssertI(4, queuelen(s))

  print(test._test_count_, "tests done.")

  return 0
