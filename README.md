# Fent Programming Language

A statically-typed, compiled programming language that compiles directly to x86_64 NASM assembly. Fent is designed as a learning project to explore compiler construction, lexical analysis, parsing, and code generation.

## Table of Contents

- [Overview](#overview)
- [Features](#features)
- [Installation](#installation)
- [Usage](#usage)
- [Language Syntax](#language-syntax)
- [Architecture & Implementation](#architecture--implementation)
- [Examples](#examples)
- [Building & Testing](#building--testing)
- [Resources](#resources)

## Overview

Fent is a minimal, imperative programming language with C-like syntax. The compiler is written in C++17 and follows a traditional three-stage compilation pipeline:

1. **Lexical Analysis** - Tokenizes source code into a stream of tokens
2. **Syntax Analysis** - Parses tokens into an Abstract Syntax Tree (AST)
3. **Code Generation** - Generates x86_64 NASM assembly from the AST

The generated assembly can then be assembled with NASM and linked to create native executables.

## Features

### Language Features

- **Primitive Types**: Integers, Booleans, Strings
- **Variables**: Mutable variables using `var` keyword
- **Functions**: First-class functions with parameters and return values
- **Control Flow**: `if`/`else` statements and `while` loops
- **Expressions**: Binary operators (`+`, `-`, `*`, `/`, `%`, `==`, `<`, `>`), unary operators (`-`, `!`)
- **Function Calls**: Support for user-defined and built-in functions (like `print`)
- **String Operations**: Compile-time string concatenation
- **Recursion**: Full support for recursive function calls

### Compiler Features

- **Debug Output**: Generate token and AST dumps for debugging
- **Error Reporting**: Line number tracking for lexical and parse errors
- **Optimization**: Compile-time evaluation of constant expressions
- **Architecture**: Currently supports x86_64 Linux

## Installation

### Prerequisites

- C++17 compatible compiler (g++ or clang++)
- NASM assembler
- GNU ld linker
- Make build system

### Build from Source

```bash
# Clone the repository
git clone <repository-url>
cd fent

# Build the compiler
make

# Optionally install system-wide
sudo make install
```

The compiler binary will be built as `bin/x86_64/fentc`.

## Usage

### Basic Compilation

```bash
# Compile a Fent source file to assembly
./bin/x86_64/fentc program.fent

# This generates output.asm by default
# To specify output file:
./bin/x86_64/fentc program.fent -o myprogram.asm
```

### Assembling and Linking

```bash
# Assemble to object file
nasm -f elf64 output.asm -o output.o

# Link to executable
ld output.o -o output

# Run the program
./output
echo $?  # Print exit code (return value)
```

### Shortcut: Compile to Executable

```bash
# Compile, assemble, and link in one command
make compile FILE=myprogram.fent
```

### Debug Options

```bash
# Generate token listing
./bin/x86_64/fentc program.fent -l
# Creates tokens_program.fent.txt

# Generate AST dump
./bin/x86_64/fentc program.fent -a
# Creates ast_program.fent.txt

# Both debug outputs
./bin/x86_64/fentc program.fent -l -a
```

## Language Syntax

### Variables

Variables are declared with the `var` keyword and are mutable:

```fent
var x = 42;
var message = "Hello World";
var flag = true;
```

Variables can be reassigned using regular assignment:

```fent
var counter = 0;
counter = counter + 1;  // Reassignment works
```

### Functions

Functions are defined with the `define` keyword:

```fent
define add(a, b) {
    return a + b;
}

define greet(name) {
    print("Hello");
    print(name);
    return 0;
}

// Call functions
var result = add(10, 20);
greet("World");
```

All function parameters are immutable by default. To allow modification of parameters within a function, prefix them with the `var` keyword:

```fent
define increment(var x) {
    x = x + 1;
    return x;
}
```

### Control Flow

#### If/Else Statements

```fent
if (x > 0) {
    print("positive");
} else {
    print("non-positive");
}
```

#### While Loops

```fent
var i = 0;
while (i < 10) {
    print("loop");
    i = i + 1;
}
```

### Expressions

#### Arithmetic Operators

```fent
var sum = 10 + 5;
var diff = 10 - 5;
var product = 10 * 5;
var quotient = 10 / 5;
var remainder = 10 % 3;
```

#### Comparison Operators

```fent
var equal = (x == y);
var less = (x < y);
var greater = (x > y);
```

#### Unary Operators

```fent
var negative = -42;
var notflag = !true;
```

### Strings

String literals are enclosed in double quotes and support escape sequences:

```fent
var message = "Hello\nWorld";  // Newline
var quote = "He said \"Hi\"";  // Escaped quotes
var path = "C:\\Users";        // Escaped backslash
```

Strings can be concatenated at compile-time:

```fent
var greeting = "Hello" + " " + "World";
```

### Built-in Functions

- `print(str)` - Outputs a string to stdout

### Comments

Currently, Fent does not support comments in the language syntax.

## Architecture & Implementation

### Compilation Pipeline

```
Source Code (.fent)
    ↓
[Lexer] → Tokens
    ↓
[Parser] → Abstract Syntax Tree (AST)
    ↓
[Code Generator] → x86_64 Assembly (.asm)
    ↓
[NASM] → Object File (.o)
    ↓
[Linker] → Executable
```

### Project Structure

```
fent/
├── bin/x86_64/          # Compiled binaries
│   └── fentc            # Fent compiler
├── src/
│   ├── lexer.cpp/hpp    # Lexical analysis
│   ├── token.hpp        # Token definitions
│   ├── ast.cpp/hpp      # AST nodes and parser
│   ├── code_gen.hpp     # Code generation interface
│   └── CogeGen/
│       └── x86_64.cpp   # x86_64 assembly code generator
├── tests/               # Test programs
├── main.cpp             # Compiler entry point
└── Makefile             # Build configuration
```

### Lexer (`src/lexer.cpp`)

The lexer performs tokenization in a single pass:

- **Whitespace Handling**: Skips whitespace, tracks line numbers for error reporting
- **Token Recognition**: Identifies keywords, identifiers, literals, and operators
- **String Processing**: Handles escape sequences (`\n`, `\t`, `\r`, `\\`, `\"`, `\0`)
- **Number Parsing**: Supports integer literals (floating point parsing exists but isn't used)

**Token Types** (defined in `src/token.hpp`):
- Delimiters: `{`, `}`, `(`, `)`, `;`, `,`
- Keywords: `if`, `else`, `while`, `return`, `var`, `define`, `true`, `false`
- Operators: `+`, `-`, `*`, `/`, `%`, `=`, `==`, `<`, `>`, `!`
- Literals: Numbers, Strings, Booleans
- Identifiers

### Parser (`src/ast.cpp`)

The parser uses recursive descent parsing to build an AST:

**Expression Parsing** (operator precedence):
1. Equality: `==`
2. Comparison: `<`, `>`
3. Term: `+`, `-`
4. Factor: `*`, `/`, `%`
5. Unary: `-`, `!`
6. Primary: literals, identifiers, function calls, parenthesized expressions

**Statement Types** (defined in `src/ast.hpp`):
- `VarDeclStmt`: Variable declarations
- `AssignStmt`: Variable assignments
- `ExprStmt`: Expression statements
- `BlockStmt`: Code blocks
- `IfStmt`: Conditional branches
- `WhileStmt`: Loops
- `ReturnStmt`: Function returns
- `FunctionDef`: Function definitions

**Expression Types**:
- `LiteralExpr`: Integer, boolean, or string constants
- `IdentifierExpr`: Variable references
- `BinaryExpr`: Binary operations
- `UnaryExpr`: Unary operations
- `CallExpr`: Function calls

### Code Generator (`src/CogeGen/x86_64.cpp`)

Generates x86_64 NASM assembly using System V AMD64 ABI calling conventions:

**Key Components**:

1. **Variable Table**: Tracks local variables with stack offsets
   - Stores type information (INT, BOOL, STRING)
   - Manages stack frame layout (RBP-relative addressing)
   - Distinguishes between function parameters and local variables

2. **Function Table**: Tracks function definitions
   - Function names and assembly labels
   - Parameter types and counts
   - Return types

3. **Data Table**: Manages string literals
   - Assigns unique labels to each string
   - Stores in `.data` section with proper null-termination
   - Tracks compile-time concatenated strings

4. **Code Generation Strategy**:
   - **Expressions**: Evaluated to `rax` register
   - **Stack Management**: Push/pop for intermediate values
   - **Control Flow**: Label-based jumps for conditionals and loops
   - **Function Calls**: System V calling convention (args in `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`)
   - **System Calls**: Linux syscalls for `print` (write) and program exit

**Register Usage**:
- `rax`: Primary accumulator, expression results, syscall numbers
- `rbx`, `rcx`: Temporary storage for binary operations
- `rsp`: Stack pointer
- `rbp`: Base pointer (frame pointer)
- `rdi`, `rsi`, `rdx`, `rcx`, `r8`, `r9`: Function argument passing

**Assembly Structure**:
```nasm
section .data
    ; String literals
    str_0: db "Hello", 0

section .text
global _start

_start:
    ; Main program code
    ; ...
    mov rdi, rax    ; Exit code
    mov rax, 60     ; sys_exit
    syscall

func_name:
    ; Function implementation
    ; ...
    ret
```

### Type System

Fent has a simple static type system with three types:
- `INT`: 64-bit signed integers
- `BOOL`: Boolean values (stored as integers: 0=false, 1=true)
- `STRING`: Immutable string literals

Type checking is minimal - the code generator infers types from literals and variable declarations.

### Memory Management

- **Stack Allocation**: Local variables are allocated on the stack
- **No Heap**: Currently no dynamic memory allocation
- **String Storage**: String literals are stored in the `.data` section
- **No Garbage Collection**: All memory is stack-based and automatically freed

### Limitations & Design Decisions

1. **No Runtime String Concatenation**: String concatenation only works at compile-time
2. **No Type Annotations**: Types are inferred from literals
3. **Limited Standard Library**: Only `print()` function available
4. **Single File Compilation**: No module system or separate compilation
5. **No Comments**: Language doesn't support comment syntax
6. **Integer-Only Math**: Floating-point is not implemented despite lexer support

## Examples

### Hello World

```fent
var message = "Hello, World!";
print(message);
return 0;
```

### Factorial (Recursive)

```fent
define factorial(n) {
    if (n < 2) {
        return 1;
    } else {
        return n * factorial(n - 1);
    }
}

var result = factorial(5);
return result;  // Returns 120
```

### Fibonacci

```fent
define fib(n) {
    if (n < 2) {
        return n;
    } else {
        return fib(n - 1) + fib(n - 2);
    }
}

var result = fib(10);
return result;  // Returns 55
```

### Sum Loop

```fent
var sum = 0;
var i = 1;
while (i < 11) {
    sum = sum + i;
    i = i + 1;
}
return sum;  // Returns 55
```

## Building & Testing

### Build Targets

```bash
make              # Build compiler
make test         # Run all tests
make test-verbose # Run tests with detailed output
make clean        # Remove build artifacts
make debug        # Build with debug symbols
make info         # Show build configuration
```

### Test Single File

```bash
# Parse and show debug output
make test-single FILE=01_literals.fent

# Compile, assemble, link, and run
make test-compile FILE=09_recursion.fent
```

### Test Suite

The `tests/` directory contains 15 test files covering:

1. `01_literals.fent` - Integer, boolean, string literals
2. `02_arithmetic.fent` - Arithmetic operations
3. `03_comparison.fent` - Comparison operators
4. `04_unary.fent` - Unary minus and logical not
5. `05_variables.fent` - Variable declarations
6. `06_if_else.fent` - Conditional statements
7. `07_while_loop.fent` - While loops
8. `08_functions.fent` - Function definitions and calls
9. `09_recursion.fent` - Recursive functions
10. `10_nested_calls.fent` - Nested function calls
11. `11_blocks.fent` - Block scoping
12. `12_expressions.fent` - Complex expressions
13. `13_strings.fent` - String handling and print
14. `14_edge_cases.fent` - Edge case testing
15. `15_complex_program.fent` - Integration test

### Running Tests

```bash
# Run all tests
make test

# Example output:
#   [1/15] 01_literals.fent       ✓ PASS
#   [2/15] 02_arithmetic.fent     ✓ PASS
#   ...
#   Results: 15/15 passed, 0 failed
```

## Resources

This project was built using the following resources:

- [Basic x86_64 compiler example](https://github.com/jgarzik/xbasic64)
- [AMD64 Architecture Manual](https://docs.amd.com/v/u/en-US/40332-PUB_4.08)
- [x86_64 Assembly Tutorial (CMU)](https://www.cs.cmu.edu/~fp/courses/15213-s07/misc/asm64-handout.pdf)
- [Linux System Call Table for x86_64](https://blog.rchapman.org/posts/Linux_System_Call_Table_for_x86_64)

## License

This is a learning project. Feel free to use and modify as needed.

## Future Improvements

Potential enhancements for the language:

- [ ] Add support for comments (`//` or `/* */`)
- [ ] Implement runtime string concatenation
- [ ] Add arrays and data structures
- [ ] Implement `for` loops
- [ ] Add more comparison operators (`<=`, `>=`, `!=`)
- [ ] Boolean operators (`&&`, `||`)
- [ ] Type checking and type errors
- [ ] Better error messages with context
- [ ] Floating-point arithmetic
- [ ] Standard library functions
- [ ] Module system
- [ ] Optimization passes
- [ ] Support for other architectures (ARM, etc.)

---

**Note**: The exit code of compiled Fent programs is the return value from the main body (or 0 if no explicit return). Use `echo $?` to check the exit code after running a program.
