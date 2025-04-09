> time

# main() int
  for i in 1..100
    if i % 15 == 0
      print("FizzBuzz")
    elif i % 3 == 0
      print("Fizz")
    elif i % 5 == 0
      print("Buzz")
    else
      print(i)
  time.sleep(1.0)
  return 0
