# Turbine Programming Language

Turbine is a lightweight, high-performance scripting language designed for simplicity, clarity, and extensibility. It emphasizes ease of use while providing powerful features such as dynamic containers, flexible loops, and modularity.

## Features

- Simple & Clear: Designed for readability and simplicity. The syntax is minimalistic and intuitive.
- Fast: Built for performance, making it suitable for both small scripts and larger projects.
- Extensible: Easily extendable with custom modules, functions, and built-in data structures such as vec, set, map, stack, and queue.
- Readable Syntax: Inspired by modern languages, it ensures that the code is clean and easy to maintain.

## Quick Look

Here's a simple example using Turbine to perform an advanced RPN (Reverse Polish Notation) calculation with time measurement.

```cpp
> math
> time

# main() int
  // This program simulates a simple calculator using Reverse Polish Notation (RPN).
  // It supports basic arithmetic operations (+, -, *, /) as well as advanced math functions like square root and cosine.
  // Additionally, it measures the time taken to perform the calculation using the time module.

  // Start the timer
  - start_time = time.now()

  // Define the expression
  - expr vec{string} = vec{"9", "sqrt", "+", "3", "cos", "*", "2", "/"}

  - stack stack{float} = stack{}

  // Process the RPN expression with advanced math functions
  for token in expr
    if math.is_number(token)
      stackpush(stack, math.to_float(token))
    else if token == "sqrt"
      - a = stackpop(stack)
      stackpush(stack, math.sqrt(a))  // Square root
    else if token == "cos"
      - a = stackpop(stack)
      stackpush(stack, math.cos(a))   // Cosine
    else
      - b = stackpop(stack)
      - a = stackpop(stack)
      - res float =
          if token == "+"
            a + b
          else if token == "-"
            a - b
          else if token == "*"
            a * b
          else
            a / b
      stackpush(stack, res)

  // Output the result and the time it took for computation
  print("Result:", stacktop(stack))
  print("Time taken:", time.elapsed(start_time), "seconds")
```

## Installation

To get started with Turbine, follow these steps:

### Clone the repository:

```bash
git clone https://github.com/tsubo164/turbine.git
```

### Build and install:

```bash
cd turbine
make install
```

### Run a script:

```bash
turbine your_script.tu
```

For additional installation instructions and configurations, check the official documentation.

## Documentation

For detailed documentation, including tutorials, API references, and guides, visit the [Turbine Documentation](https://tsubo164.github.io/turbine-docs/).

### Key Topics:

- Language Overview
- Syntax and Structure
- Standard Library
- Building and Extending Modules

## Community

Join the community of Turbine users and developers:

- GitHub Issues for bug reports and feature requests.
- Discussion Forum for general discussion, help, and ideas.

## License

Turbine is licensed under the MIT License. See the LICENSE file for more information.
