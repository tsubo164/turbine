[test]

# main(args []string) int

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

  print(test._test_count_, "tests done.")

  return 0
