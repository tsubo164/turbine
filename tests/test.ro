- _test_count_ int

// asserts bool
# AssertB(expected bool, actual bool, $caller_line)
  _test_count_ += 1

  if expected != actual
    print(format("error:%d: expected: %t actual: %t", $caller_line, expected, actual))
    exit(1)

// asserts int
# AssertI(expected int, actual int, $caller_line)
  _test_count_ += 1

  if expected != actual
    print(format("error:%d: expected: %d actual: %d", $caller_line, expected, actual))
    exit(1)

// asserts float
# AssertF(expected float, actual float, $caller_line)
  _test_count_ += 1

  if expected != actual
    print(format("error:%d: expected: %g actual: %g", $caller_line, expected, actual))
    exit(1)

// asserts string
# AssertS(expected string, actual string, $caller_line)
  _test_count_ += 1

  if expected != actual
    print(format("error:%d: expected: %s actual: %s", $caller_line, expected, actual))
    exit(1)
