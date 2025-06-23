# Ember Core Language

A programming language interpreter implementation with basic expression evaluation and built-in functions.

## 🚧 Development Status: Early Implementation

Ember Core is currently in early development with basic language features implemented:
- ✅ **Basic expression evaluation** (arithmetic, string concatenation)
- ✅ **Variables and arrays** (global scope only)
- ✅ **Built-in functions** (print, type checking, math functions)
- ⚠️ **Control flow** (if/else, loops - not yet working)
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
- **Control Flow**: if/else statements, loops (for, while)
- **Functions**: User-defined functions and closures
- **Objects/Classes**: Object-oriented programming features
- **Module System**: Import/export functionality
- **Exception Handling**: try/catch blocks
- **Standard Library**: File I/O, networking, cryptography

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

> ⚠️ **Build Status**: The build system currently has dependency issues and may not compile successfully.

```bash
# Attempt to build (may fail due to missing dependencies)
make clean
make

# Note: The build requires stdlib components that are not properly integrated
```

### Running Ember Code

If the build succeeds, you can test basic functionality:

```bash
# Execute a script file (if build works)
./build/ember script.ember

# Note: Interactive REPL and compiler are not currently functional
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
version = "2.0.3"
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
```

**Note**: The following features are NOT yet implemented:
- Functions, classes, if/else statements, loops
- Standard library features (JSON, crypto, file I/O)
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
- **No control flow**: if/else, loops not working
- **No objects/classes**: OOP features not implemented
- **No modules**: import/export system not working
- **Build issues**: Dependencies not properly organized

## Development

### Building from Source

```bash
# Install dependencies (Ubuntu/Debian)
sudo apt-get install build-essential libreadline-dev libssl-dev

# Attempt to build (currently has issues)
make clean
make

# Note: Build currently fails due to missing stdlib.h and other dependency issues
```

### Current Development Status

- **Build system**: Needs dependency reorganization
- **Test suite**: Limited tests available in parent repository
- **Documentation**: Being updated to reflect actual implementation
- **Architecture**: Needs refactoring for clean component separation

## Contributing

This is an early-stage language implementation. Current priorities:

1. **Fix build system** - Resolve dependency issues between core and stdlib
2. **Implement control flow** - Add if/else statements and loops
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