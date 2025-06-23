# Ember Core Language

A programming language interpreter implementation with basic expression evaluation and built-in functions.

## 🚧 Development Status: Early Implementation

Ember Core is currently in early development with basic language features implemented:
- ✅ **Basic expression evaluation** (arithmetic, string concatenation)
- ✅ **Variables and arrays** (global scope only)
- ✅ **Built-in functions** (print, type checking, math functions)
- ✅ **Control flow** (if/else statements, while loops work; for loops work but avoid continue)
- ⚠️ **Functions** (user-defined functions - not yet working)
- ⚠️ **Objects/classes** (OOP features - not yet working)

## Overview

Ember Core is a basic programming language interpreter written in C. This repository contains the core language implementation including a virtual machine, parser, and runtime system with built-in functions.

> ⚠️ **Development Status**: This is an early-stage implementation with limited functionality. Many documented features are not yet working.

## Current Working Features

### ✅ Basic Language Support
- **Expression Evaluation**: Arithmetic operations (`+`, `-`, `*`, `/`) with operator precedence
- **Variables**: Global variable assignment and retrieval (no local scope)
- **Data Types**: Numbers, strings, booleans, arrays, and nil values
- **String Operations**: String concatenation with `+` operator
- **Array Support**: Array literals and indexing (0-based)

### ✅ Built-in Functions
- **Output**: `print()` function for displaying values
- **Type System**: `type()` function for runtime type checking
- **Type Conversion**: `str()`, `num()`, `int()`, `bool()` conversion functions
- **Math Functions**: `abs()`, `sqrt()`, `max()`, `min()`, `floor()`, `ceil()`
- **Boolean Logic**: `not()` function for boolean negation

### ⚠️ Not Yet Implemented
- **Functions**: User-defined functions and closures
- **Objects/Classes**: Object-oriented programming features
- **Module System**: Import/export functionality
- **Exception Handling**: try/catch blocks
- **Standard Library**: Advanced features (networking, etc.)

## 🏗️ Repository Structure

This repository is part of a larger project structure:

### Related Repositories
- **[ember-stdlib](../ember-stdlib)** - Standard library modules (in development)
- **[emberweb](../emberweb)** - Web server integration (in development)
- **[ember-tools](../ember-tools)** - Development tools (in development)
- **[ember-tests](../ember-tests)** - Test suite and examples
- **[ember-docs](../ember-docs)** - Platform documentation

## Quick Start

### Building Ember Core

> ⚠️ **Build Status**: ember-core builds successfully but with compilation warnings that need to be addressed.

```bash
# Build ember-core (succeeds but shows warnings)
make clean
make

# Note: Build succeeds and produces working interpreter, but compilation warnings should be fixed
```

### Running Ember Code

You can test basic functionality:

```bash
# Execute a script file
./build/ember script.ember

# Interactive REPL mode
./build/ember

# Note: Both executable mode and REPL work
```

### Example Code - Working Features

```ember
// Basic variables and arithmetic
x = 42
y = 10
z = x + y * 2  // Result: 62
print("z =", z)

// String operations
name = "Ember"
version = "early-dev"
message = "Welcome to " + name + " " + version
print(message)

// Arrays
numbers = [1, 2, 3, 4, 5]
print("First element:", numbers[0])
print("Array:", numbers)

// Type checking and conversion
print("Type of 42:", type(42))        // "number"
print("Type of 'hello':", type("hello"))  // "string"
number_str = str(123)                 // Convert to string
string_num = num("456")               // Convert to number

// Math functions
print("abs(-15):", abs(-15))          // 15
print("sqrt(25):", sqrt(25))          // 5
print("max(7, 3):", max(7, 3))        // 7

// Control flow (these work!)
if x > 30 {
    print("x is greater than 30")
} else {
    print("x is 30 or less")
}

// While loops work
i = 0
while i < 3 {
    print("Loop iteration:", i)
    i = i + 1
}

// For loops work (but avoid continue)
for j = 0; j < 3; j = j + 1 {
    print("For loop:", j)
}
```

**Note**: The following features are NOT yet implemented:
- Functions, classes
- Advanced standard library features (networking, advanced crypto)
- Module system and imports

## Architecture

### Core Components

- **Virtual Machine (`src/core/vm.c`)**: Basic stack-based bytecode interpreter
- **Parser (`src/frontend/parser/`)**: Recursive descent parser for expressions and statements  
- **Lexer (`src/frontend/lexer/`)**: Token scanner for source code
- **Runtime (`src/runtime/`)**: Built-in functions and basic runtime services
- **Memory Management (`src/core/memory.c`)**: Basic memory allocation

### Implementation Notes

- **Single-threaded**: No concurrency or parallel execution
- **Global scope**: All variables are global, no local scoping
- **Basic types**: Numbers, strings, booleans, arrays supported
- **Expression evaluation**: Arithmetic and string operations work
- **Limited error handling**: Basic error reporting without recovery

## API Reference

### Currently Working Functions

```ember
// Output
print(value1, value2, ...)     // Print values to stdout

// Type checking
type(value)                    // Get type: "number", "string", "bool", "array", "nil"

// Type conversion  
str(value)                     // Convert to string
num(string)                    // Convert string to number
int(number)                    // Convert to integer
bool(value)                    // Convert to boolean

// Math functions
abs(number)                    // Absolute value
sqrt(number)                   // Square root
max(a, b)                      // Maximum of two values
min(a, b)                      // Minimum of two values
floor(number)                  // Floor (round down)
ceil(number)                   // Ceiling (round up)

// Boolean logic
not(bool_value)                // Boolean negation
```

### Not Yet Implemented

The following functions exist in the codebase but are not working:
- File I/O functions (`file_read`, `file_write`)
- String manipulation (`string_length`, `string_split`)
- JSON functions (`json_parse`, `json_stringify`) 
- Cryptography functions (`crypto_hash`)
- Array functions (`array_length`)
- Network/HTTP functions

## Current Limitations

- **No exception handling**: try/catch blocks are not implemented
- **No user-defined functions**: Function definitions don't work
- **For loop continue bug**: Avoid using continue in for loops (causes infinite loops)
- **No objects/classes**: OOP features not implemented
- **No modules**: import/export system not working
- **Build issues**: Dependencies not properly organized

## Development

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libreadline-dev libssl-dev

# Build ember-core (succeeds but with warnings)
make clean
make

# Note: Build succeeds and creates working interpreter, warnings need to be fixed
```

### Current Development Status

- **Build system**: ember-core builds successfully but with warnings
- **Test suite**: Limited tests available in parent repository
- **Documentation**: Being updated to reflect actual implementation
- **Architecture**: Needs refactoring for clean component separation

## Contributing

This is an early-stage language implementation. Current priorities:

1. **Fix build system** - Resolve dependency issues between core and stdlib
2. **Fix continue bug** - Resolve infinite loop issue with continue in for loops
3. **Add function support** - Enable user-defined functions
4. **Improve error handling** - Better error messages and recovery
5. **Organize architecture** - Clean separation between components

### Code Style

- Use C99 standard
- 4-space indentation  
- Function names: `module_function` pattern
- Check return values for all operations

## License

Licensed under the MIT License. See LICENSE file for details.

---

**Ember Core** - A basic programming language interpreter in early development.