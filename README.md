# Turbine Programming Language

Turbine is a lightweight, high-performance scripting language designed for simplicity, clarity, and extensibility. It emphasizes ease of use while providing powerful features such as dynamic containers, flexible loops, and modularity.

## Features

- **Familiar, Markdown-like Syntax**

  Turbine’s syntax is designed to resemble Markdown and simple pseudocode, making it approachable even for those without a programming background. Indentation-based blocks and clean keywords make scripts easy to write and read.

- **Minimal but Expressive Standard Library**

  Includes built-in modules for `math`, `time`, containers (`vec`, `map`, `set`, `stack`, `queue`), and I/O — enough to solve real problems without bloat.

- **No Hidden Magic**

  All control flow, scoping, and operations are explicit and visible. There’s no implicit coercion or behind-the-scenes behaviors.

- **Designed for Simplicity**

  Instead of complex features like generics or ownership systems, Turbine offers direct tools to solve problems clearly and concisely.

- **Consistent Structure**

  Uniform syntax across data types and control structures reduces the learning curve and makes code easier to maintain.

- **Lightweight and Fast**

  Single-binary implementation written in C for speed and simplicity, with a small runtime footprint.

## Quick Look

A compact RPN (Reverse Polish Notation) calculator using `math` and `time` modules to demonstrate Turbine’s scripting style.

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
    elif token == "sqrt"
      - a = stackpop(stack)
      stackpush(stack, math.sqrt(a))  // Square root
    elif token == "cos"
      - a = stackpop(stack)
      stackpush(stack, math.cos(a))   // Cosine
    else
      - b = stackpop(stack)
      - a = stackpop(stack)
      - res = 0.0

      if token == "+"
        res = a + b
      elif token == "-"
        res = a - b
      elif token == "*"
        res = a * b
      else
        res = a / b

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

- [GitHub Issues](https://github.com/tsubo164/turbine/issues) for bug reports and feature requests.
- [Discussion Forum](https://github.com/tsubo164/turbine/discussions) for general discussion, help, and ideas.


## License

Turbine is licensed under the MIT License. See the LICENSE file for more information.