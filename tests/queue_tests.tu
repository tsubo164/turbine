[test]

# main(args []string) int

  ---
    - q queue{int}
    test.AssertI(0, queuelen(q))

    queuepush(q, 11)
    test.AssertI(1, queuelen(q))
    queuepush(q, 22)
    test.AssertI(2, queuelen(q))

  print(test._test_count_, "tests done.")

  return 0
