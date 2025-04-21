# Turbine Programming Language

Turbine is a lightweight, high-performance scripting language designed for simplicity, clarity, and extensibility. It emphasizes ease of use while providing powerful features such as dynamic containers, flexible loops, and modularity.

## Project Status

Turbine is currently an early-stage experimental project.
We welcome feedback, issue reports, and contributions from the community.
Expect breaking changes and active development.

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

# is_number(token string) bool
  - digits = vec{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"}
  for digit in digits
    if token == digit
      return true
  return false

# to_number(token string) int
  - digits = vec{"0", "1", "2", "3", "4", "5", "6", "7", "8", "9"}
  for i, digit in digits
    if token == digit
      return i
  return -1

# main() int
  // This program simulates a simple calculator using Reverse Polish Notation (RPN).
  // It supports basic arithmetic operations (+, -, *, /) as well as advanced math functions like square root and cosine.
  // Additionally, it measures the time taken to perform the calculation using the time module.

  // Start the timer
  - start_time = time.now()

  // Define the expression
  - expr = vec{"3", "3", "**", "6", "+", "7", "-"}
  - stk stack{int}

  // Process the RPN expression with advanced math functions
  for token in expr
    if is_number(token)
      stackpush(stk, to_number(token))
    else
      - b = stackpop(stk)
      - a = stackpop(stk)
      - res = 0

      if token == "+"
        res = a + b
      elif token == "-"
        res = a - b
      elif token == "*"
        res = a * b
      elif token == "/"
        res = a / b
      elif token == "**"
        res = int(math.pow(float(a), float(b)))

      stackpush(stk, res)

  // Output the result and the time it took for computation
  print("Result:", stacktop(stk))
  print("Time taken:", time.elapsed(start_time), "seconds")

  return 0
```

## Installation

## ️Requirements

To build and run Turbine, you will need:

- A C compiler that supports **C99** (e.g., `gcc`, `clang`)
- `make`
- A Unix-like environment (Linux, macOS, etc.)


### Clone the repository:

```bash
git clone https://github.com/tsubo164/turbine.git
```

### Build and install:

```bash
cd turbine
sudo make install
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

- [Discord server](https://discord.gg/Q6pABVW3) to chat, ask questions, or share feedback
- [GitHub Issues](https://github.com/tsubo164/turbine/issues) for bug reports and feature requests.
- [Discussion Forum](https://github.com/tsubo164/turbine/discussions) for general discussion, help, and ideas.

## License

Turbine is licensed under the MIT License. See the LICENSE file for more information.