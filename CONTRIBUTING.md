# Contributing to Ember Core

Thank you for your interest in contributing to Ember Core! This document provides guidelines and information for contributors to the core interpreter.

## Table of Contents

- [Code of Conduct](#code-of-conduct)
- [Getting Started](#getting-started)
- [Development Setup](#development-setup)
- [Making Changes](#making-changes)
- [Testing](#testing)
- [Submitting Changes](#submitting-changes)
- [Code Style](#code-style)
- [Documentation](#documentation)
- [Community](#community)

## Code of Conduct

This project and everyone participating in it is governed by our [Code of Conduct](CODE_OF_CONDUCT.md). By participating, you are expected to uphold this code. Please report unacceptable behavior to conduct@emberplatform.org.

## Getting Started

### Types of Contributions

We welcome several types of contributions:

- **Bug reports**: Help us identify and fix interpreter issues
- **Feature requests**: Suggest new language features or built-ins
- **Code contributions**: Implement bug fixes or new features
- **Documentation**: Improve or add documentation
- **Testing**: Add or improve test coverage
- **Performance**: Optimize existing code

### Before You Start

1. **Check existing issues**: Look for existing issues or discussions related to your contribution
2. **Create an issue**: For substantial changes, create an issue to discuss your proposal first
3. **Fork the repository**: Create your own fork of the project
4. **Set up development environment**: Follow the development setup instructions

## Development Setup

### Prerequisites

```bash
# Ubuntu/Debian
sudo apt-get install build-essential libreadline-dev libssl-dev pkg-config valgrind

# Arch Linux
sudo pacman -S gcc readline openssl pkg-config valgrind

# macOS
brew install readline openssl pkg-config
```

### Environment Setup

```bash
# Clone your fork
git clone https://github.com/YOUR_USERNAME/ember-core.git
cd ember-core

# Add upstream remote
git remote add upstream https://github.com/emberplatform/ember-core.git

# Install dependencies and build
make clean && make

# Verify setup
echo 'print("Hello, Ember!")' | ./build/ember
```

### Core-Specific Setup

The Ember Core interpreter has these key directories:

- **`src/core/`** - Virtual machine and core execution engine
- **`src/frontend/`** - Lexer and parser components
- **`src/runtime/`** - Built-in functions and runtime services
- **`include/`** - Public header files
- **`tests/`** - Test suites (unit, integration, fuzz)

## Making Changes

### Development Workflow

1. **Create a branch**: Create a new branch for your changes
   ```bash
   git checkout -b feature/your-feature-name
   ```

2. **Make your changes**: Implement your bug fix or feature

3. **Test your changes**: Ensure all tests pass
   ```bash
   make test
   ```

4. **Commit your changes**: Write clear, descriptive commit messages
   ```bash
   git commit -m "feat: add new built-in function"
   ```

### Commit Message Guidelines

We follow the [Conventional Commits](https://conventionalcommits.org/) specification:

- `feat:` - New language features or built-ins
- `fix:` - Bug fixes in interpreter or runtime
- `docs:` - Documentation changes
- `style:` - Code style changes (formatting, etc.)
- `refactor:` - Code refactoring without functionality changes
- `test:` - Adding or updating tests
- `chore:` - Build system or maintenance tasks

Examples:
```
feat: add user-defined function support
fix: resolve continue bug in for loops
docs: update API documentation for crypto functions
test: add unit tests for array operations
```

### Branch Naming

Use descriptive branch names:
- `feature/user-functions`
- `fix/memory-leak-parser`
- `docs/api-documentation-update`
- `refactor/vm-optimization`

## Testing

### Running Tests

```bash
# Run all tests
make test

# Run basic functionality tests
./build/ember working_features_test.ember

# Run control flow tests
./build/ember test_control_flow_simple.ember

# Run with memory debugging
make ASAN=1 && ./build/ember test_memory.ember

# Run security tests
cd ../ember-tests && ./security_scripts/run_memory_safety_tests.sh
```

### Writing Tests

- **Unit tests**: Test individual functions and components in `tests/unit/`
- **Integration tests**: Test component interactions in `tests/integration/`
- **Security tests**: Test for security vulnerabilities in `tests/fuzz/`
- **Ember script tests**: Test language features with `.ember` files

Example test structure:
```c
// For C code - tests/unit/test_builtins.c
void test_print_function() {
    // Test the print() built-in function
    ember_value_t args[2];
    args[0] = ember_value_string("Hello");
    args[1] = ember_value_number(42);
    
    ember_value_t result = builtin_print(2, args);
    assert(ember_value_is_nil(result));
}
```

```javascript
// For Ember code - test_math_functions.ember
print("Testing math functions...")

// Test abs() function
result = abs(-15)
assert(result == 15, "abs(-15) should return 15")

// Test sqrt() function
result = sqrt(16)
assert(result == 4, "sqrt(16) should return 4")

print("Math function tests passed!")
```

### Test Requirements

- All new code must include appropriate tests
- Tests must pass on all supported platforms
- Security-sensitive code requires additional security tests
- Performance-critical code should include performance tests

## Submitting Changes

### Pull Request Process

1. **Update your branch**: Sync with upstream before submitting
   ```bash
   git fetch upstream
   git rebase upstream/main
   ```

2. **Push your branch**: Push to your fork
   ```bash
   git push origin feature/your-feature-name
   ```

3. **Create Pull Request**: Open a PR with a clear title and description

4. **Address feedback**: Respond to review comments and make necessary changes

### Pull Request Guidelines

#### Title Format
```
[type]: brief description

Examples:
feat: add user-defined function support
fix: resolve memory leak in parser
docs: update README with current capabilities
```

#### Description Template
```markdown
## Summary
Brief description of changes

## Changes Made
- List of specific changes
- Use bullet points for clarity

## Testing
- [ ] Unit tests added/updated
- [ ] Integration tests pass
- [ ] Manual testing with .ember scripts completed
- [ ] Memory safety tests pass

## Documentation
- [ ] README updated if needed
- [ ] API documentation updated
- [ ] Code comments added

## Breaking Changes
- List any breaking changes
- Explain migration path if applicable

## Related Issues
Closes #123
Relates to #456
```

### Review Process

1. **Automated checks**: CI/CD pipeline runs automatically
2. **Code review**: Project maintainers review your code
3. **Feedback**: Address any requested changes
4. **Approval**: Once approved, your PR will be merged

## Code Style

### C Code Style
- Use C99/C11 standard
- 4-space indentation (no tabs)
- Function names: `module_function_name` (e.g., `vm_execute`, `parser_parse_expression`)
- Variable names: `snake_case`
- Constants: `UPPER_CASE`
- Braces on same line for functions, new line for control structures

```c
// Function definition
int vm_execute(vm_t *vm, bytecode_t *code) {
    if (vm == NULL) {
        return -1;
    }
    
    for (int i = 0; i < code->length; i++) {
        // Process bytecode instruction
        if (vm_execute_instruction(vm, code->instructions[i]) != 0) {
            return -1;
        }
    }
    
    return 0;
}
```

### Header File Organization
```c
// Include guard
#ifndef EMBER_MODULE_H
#define EMBER_MODULE_H

// System includes first
#include <stdio.h>
#include <stdlib.h>

// Project includes
#include "ember.h"

// Function declarations
int module_function(int param);

#endif // EMBER_MODULE_H
```

### Error Handling
```c
// Always check return values
ember_value_t *result = vm_execute_function(vm, func);
if (result == NULL) {
    fprintf(stderr, "Error: Function execution failed\n");
    return EMBER_ERROR;
}

// Use consistent error codes
#define EMBER_SUCCESS 0
#define EMBER_ERROR   -1
#define EMBER_MEMORY_ERROR -2
```

### Documentation Style

- Use clear, concise language
- Include working examples for all public APIs
- Document all function parameters and return values
- Use Doxygen-style comments for public APIs

```c
/**
 * Execute a bytecode instruction in the virtual machine
 * 
 * @param vm Pointer to the virtual machine instance
 * @param instruction The bytecode instruction to execute
 * @return 0 on success, negative error code on failure
 */
int vm_execute_instruction(vm_t *vm, bytecode_instruction_t instruction);
```

## Security Considerations

### Security-Sensitive Changes

For changes involving:
- **Memory management** and allocation
- **Input parsing** and validation
- **Built-in functions** that handle user data
- **Cryptographic functions**
- **File system operations**

Additional requirements:
- Security review required
- Security tests must be added
- Memory safety validation with AddressSanitizer
- Buffer overflow protection verification

### Reporting Security Issues

**Do not open public issues for security vulnerabilities.**

Instead, email: security@emberplatform.org

Include:
- Description of the vulnerability
- Steps to reproduce
- Potential impact on interpreter security
- Suggested fix (if any)

## Community

### Communication Channels

- **GitHub Issues**: Bug reports and feature requests
- **GitHub Discussions**: General questions and design discussions
- **Pull Requests**: Code review and collaboration

### Getting Help

- **Documentation**: [Ember Platform Documentation](https://docs.emberplatform.org)
- **Examples**: Check working `.ember` files in the repository
- **Previous issues**: Search existing GitHub issues
- **Architecture docs**: See [CLAUDE.md](../CLAUDE.md) for project structure

### Code of Conduct

We are committed to providing a welcoming and inclusive environment. Please read and follow our [Code of Conduct](CODE_OF_CONDUCT.md).

## Release Process

### Versioning

We follow [Semantic Versioning](https://semver.org/):
- **MAJOR.MINOR.PATCH**
- **MAJOR**: Breaking language changes
- **MINOR**: New language features (backward compatible)
- **PATCH**: Bug fixes (backward compatible)

### Current Priorities

1. **User-defined functions** - Function definitions with parameters
2. **Logical operators** - &&, ||, ! operators  
3. **Enhanced error handling** - Better error messages and recovery
4. **Local variable scoping** - Function-local variables
5. **Performance optimizations** - Memory management and execution speed

---

## Thank You

Thank you for contributing to Ember Core! Your contributions help make the Ember programming language better for everyone.

For questions about contributing, please:
- Open a GitHub Discussion
- Check existing documentation
- Contact maintainers via GitHub

**Happy coding!** 🚀