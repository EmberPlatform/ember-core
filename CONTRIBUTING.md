# Contributing to Ember Core

Thank you for your interest in contributing to Ember Core! This document provides guidelines for contributing to the core language implementation.

## Getting Started

### Development Environment

1. **System Requirements**:
   - GCC 7+ or Clang 8+
   - Make 4.0+
   - Git 2.0+

2. **Dependencies**:
   ```bash
   # Ubuntu/Debian
   sudo apt-get install build-essential libreadline-dev libssl-dev libcurl4-openssl-dev pkg-config
   
   # Arch Linux
   sudo pacman -S gcc readline openssl curl pkg-config
   
   # macOS
   brew install readline openssl curl pkg-config
   ```

3. **Clone and Build**:
   ```bash
   git clone https://github.com/ember-lang/ember-core.git
   cd ember-core
   make all
   make check
   ```

### Code Organization

- **`src/core/`**: Virtual machine, memory management, optimization
- **`src/frontend/`**: Lexer, parser, AST generation
- **`src/runtime/`**: Built-in functions, value system, package management
- **`include/`**: Public API headers
- **`tools/`**: REPL, compiler, and other tools
- **`tests/`**: Unit tests, fuzzing tests

## Contributing Guidelines

### Code Style

1. **C Code Standards**:
   - Use C99/C11 standard
   - 4-space indentation (no tabs)
   - Function names: `module_function` pattern
   - Always check return values
   - Use `const` where appropriate

2. **Naming Conventions**:
   ```c
   // Functions
   ember_value_t vm_execute(ember_vm_t* vm);
   
   // Constants
   #define MAX_STACK_SIZE 1024
   
   // Types
   typedef struct ember_vm ember_vm_t;
   ```

3. **Memory Management**:
   - Use arena allocators for performance-critical code
   - Always pair malloc/free
   - Check for NULL returns
   - Use valgrind for testing

### Testing

1. **Before Submitting**:
   ```bash
   make clean
   make asan                    # AddressSanitizer build
   make check                   # Run all tests
   make fuzz && make fuzz-run   # Fuzzing tests
   ```

2. **Adding Tests**:
   - Add unit tests for new functions
   - Include negative test cases
   - Test memory safety with ASAN
   - Add performance benchmarks for core features

3. **Test Categories**:
   - **Unit tests**: `tests/unit/test_*.c`
   - **Fuzzing**: `tests/fuzz/fuzz_*.c`
   - **Integration**: Complex scenarios

### Performance Considerations

1. **Hot Paths**: VM execution, string operations, memory allocation
2. **Memory Usage**: Prefer stack allocation, use pools for frequent allocations
3. **Threading**: VM pool operations must be thread-safe
4. **Benchmarking**: Include performance tests for significant changes

### Security Guidelines

1. **Input Validation**: Validate all external input
2. **Buffer Safety**: Use safe string functions
3. **Integer Overflow**: Check for overflow in calculations
4. **Memory Safety**: No buffer overruns, use-after-free

## Development Workflow

### 1. Issue Discussion

- Check existing issues before creating new ones
- Use issue templates for bug reports and feature requests
- Discuss design approaches for significant changes

### 2. Implementation

```bash
# Create feature branch
git checkout -b feature/my-feature

# Make changes with frequent commits
git add .
git commit -m "Add initial implementation"

# Keep branch updated
git pull --rebase origin main
```

### 3. Testing

```bash
# Comprehensive testing
make clean && make asan
make check
make fuzz-run

# Memory safety
valgrind ./build/test-vm
```

### 4. Pull Request

- Create PR with clear description
- Include test results
- Reference related issues
- Ensure CI passes

## Areas for Contribution

### High Priority

1. **Performance Optimization**:
   - JIT compilation research
   - Memory layout optimization
   - String interning improvements

2. **Language Features**:
   - Type inference system
   - Pattern matching
   - Generator functions

3. **Developer Tools**:
   - Debugger improvements
   - Language server features
   - Better error messages

### Medium Priority

1. **Standard Library**:
   - Additional built-in functions
   - Data structure implementations
   - I/O improvements

2. **Platform Support**:
   - Windows compatibility
   - ARM optimization
   - Mobile platforms

### Documentation

1. **API Documentation**: Improve function documentation
2. **Examples**: Real-world usage examples
3. **Tutorials**: Learning materials
4. **Architecture**: Implementation details

## Code Review Process

### Reviewers Look For

1. **Correctness**: Does the code work as intended?
2. **Performance**: Are there performance implications?
3. **Security**: Any security vulnerabilities?
4. **Style**: Follows project conventions?
5. **Tests**: Adequate test coverage?

### Review Timeline

- **Small changes**: 1-2 days
- **Medium changes**: 3-5 days
- **Large changes**: 1-2 weeks

## Release Process

### Version Numbering

- **Major**: Breaking API changes
- **Minor**: New features, performance improvements
- **Patch**: Bug fixes, small improvements

### Release Criteria

1. All tests pass
2. No known security issues
3. Performance regressions resolved
4. Documentation updated

## Getting Help

### Community Channels

- **GitHub Issues**: Bug reports, feature requests
- **GitHub Discussions**: General questions, ideas
- **Email**: core-dev@ember-lang.org

### Debugging Tips

1. **Build Issues**: Check dependencies, use `make clean`
2. **Test Failures**: Run with `BUILD_TYPE=debug` for better error info
3. **Memory Issues**: Use AddressSanitizer and Valgrind
4. **Performance**: Use profiling tools, check hot paths

## Recognition

Contributors are recognized in:

- CONTRIBUTORS.md file
- Release notes
- Conference talks
- Community highlights

## License

By contributing to Ember Core, you agree that your contributions will be licensed under the same license as the project (MIT License).

---

Thank you for contributing to Ember Core! Your help makes the language better for everyone.