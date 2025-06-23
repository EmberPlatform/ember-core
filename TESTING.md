# Ember Core Testing Framework

This document describes the comprehensive testing framework for the Ember programming language.

## Overview

The Ember testing framework provides automated testing capabilities at multiple levels:
- **Unit tests**: Test individual components (lexer, parser, VM, builtins)
- **Integration tests**: Test component interactions
- **Regression tests**: Prevent previously fixed bugs from returning
- **Ember language tests**: Test Ember code using Ember itself
- **Security tests**: Validate security features and protections
- **Performance tests**: Benchmark and stress testing

## Directory Structure

```
repos/ember-core/
├── scripts/
│   └── run_regression_tests.sh    # Main test runner script
├── tests/
│   ├── core/                      # Core language feature tests
│   ├── control_flow/              # If/else, loops, break/continue tests
│   ├── stdlib/                    # Standard library function tests
│   ├── regression/                # Regression tests for specific bugs
│   ├── unit/                      # C unit tests
│   ├── integration/               # C integration tests
│   └── fuzz/                      # Fuzzing tests
├── test_runner.ember              # Ember test framework
└── tests/example_test.ember       # Example test format
```

## Running Tests

### Quick Start

```bash
# Run all tests
make test

# Run only core functionality tests
make test-core

# Run security tests
make test-security

# Run quick smoke tests
make test-quick

# Run with specific build type
make BUILD_TYPE=asan test
```

### Test Runner Script

The main test runner script `scripts/run_regression_tests.sh` provides comprehensive test execution:

```bash
# Run all tests
./scripts/run_regression_tests.sh

# Run specific category
./scripts/run_regression_tests.sh --filter=core
./scripts/run_regression_tests.sh --filter=control_flow
./scripts/run_regression_tests.sh --filter=stdlib
./scripts/run_regression_tests.sh --filter=regression
./scripts/run_regression_tests.sh --filter=security
```

### Features of the Test Runner

1. **Automatic building**: Cleans and rebuilds ember-core before testing
2. **Categorized tests**: Organizes tests by functionality
3. **Expected output validation**: Compares actual output with expected results
4. **Timeout protection**: Prevents hanging tests from blocking the suite
5. **Detailed reporting**: Shows pass/fail statistics and saves failed test details
6. **Exit codes**: Returns non-zero on test failures for CI integration

## Writing Tests

### Ember Language Tests

Ember tests are written in Ember itself and can use inline expectations:

```ember
# Test description
# This test validates basic arithmetic

x = 5 + 3
print("5 + 3 = " + str(x))
# EXPECT: 5 + 3 = 8

# Test arrays
arr = [1, 2, 3]
print("Array length: " + str(len(arr)))
# EXPECT: Array length: 3
```

The test runner looks for `# EXPECT:` comments and validates the output matches.

### Using the Test Framework

For more complex tests, use the test framework functions:

```ember
# Include test runner functions (future: use import)
# source: test_runner.ember

describe("Math Functions")

test("abs function", null)
assert_equals(abs(-5), 5, "abs of negative number")
assert_equals(abs(5), 5, "abs of positive number")

test("max/min functions", null)
assert_equals(max(10, 20), 20, "max of two numbers")
assert_equals(min(10, 20), 10, "min of two numbers")

# Array tests
describe("Array Operations")

test("array equality", null)
arr1 = [1, 2, 3]
arr2 = [1, 2, 3]
assert_array_equals(arr1, arr2, "identical arrays")

# Print test summary
print_summary()
```

### Test Assertions

Available assertion functions:
- `assert(condition, message)` - Basic assertion
- `assert_equals(actual, expected, message)` - Equality check
- `assert_not_equals(actual, unexpected, message)` - Inequality check
- `assert_true(value, message)` - Check for true
- `assert_false(value, message)` - Check for false
- `assert_greater(actual, threshold, message)` - Greater than check
- `assert_less(actual, threshold, message)` - Less than check
- `assert_array_equals(actual, expected, message)` - Array comparison

### Expected Output Files

For complex output validation, create a `.expected` file:

```bash
# For test file: tests/core/my_test.ember
# Create expected output: tests/core/my_test.expected

echo "Expected output line 1" > tests/core/my_test.expected
echo "Expected output line 2" >> tests/core/my_test.expected
```

## Test Categories

### Core Tests (`tests/core/`)
- Basic language features (variables, expressions, operators)
- Function definitions and calls
- String operations
- Array operations
- Object-oriented features (if supported)

### Control Flow Tests (`tests/control_flow/`)
- If/else statements
- While loops
- For loops
- Break and continue statements
- Nested control structures
- Switch statements (if supported)

### Standard Library Tests (`tests/stdlib/`)
- Math functions (abs, min, max, pow, sqrt, etc.)
- String functions (len, substr, split, join, etc.)
- Crypto functions (sha256, sha512, hmac, etc.)
- JSON functions (parse, stringify, validate)
- File I/O functions (read_file, write_file, etc.)

### Regression Tests (`tests/regression/`)
- Tests for specific bugs that have been fixed
- Prevents regressions in future changes
- Named after the issue they test (e.g., `continue_bug_test.ember`)

## Continuous Integration

The project includes GitHub Actions workflow (`.github/workflows/test.yml`) that:

1. Runs tests on multiple build types (release, debug, asan)
2. Performs coverage analysis
3. Runs security tests
4. Checks code style
5. Uploads test results as artifacts

## Best Practices

1. **Write tests for new features**: Every new feature should have tests
2. **Add regression tests for bugs**: When fixing a bug, add a test to prevent regression
3. **Use descriptive names**: Test files and assertions should clearly indicate what they test
4. **Test edge cases**: Include tests for boundary conditions and error cases
5. **Keep tests focused**: Each test should validate one specific behavior
6. **Use appropriate timeouts**: Prevent infinite loops from hanging the test suite

## Debugging Failed Tests

When a test fails:

1. Check `failed_tests.txt` for details
2. Run the specific test manually:
   ```bash
   build/ember tests/core/failing_test.ember
   ```
3. Compare actual output with expected output
4. Use the debugger if needed:
   ```bash
   gdb build/ember
   run tests/core/failing_test.ember
   ```

## Security Testing

Security tests validate:
- Input validation
- Buffer overflow protection
- Memory safety
- Cryptographic function correctness
- File system access controls

Run security tests with:
```bash
make test-security
```

## Performance Testing

For performance testing:
- Use `memory_stress_test.ember` for memory stress testing
- Benchmark tests are in `repos/ember-tests/benchmarks/`
- Run with timing measurements

## Adding New Tests

1. Create test file in appropriate directory
2. Add inline expectations or use test framework
3. Ensure test runs successfully locally
4. Add to appropriate test category
5. Update this documentation if adding new test patterns

## Known Issues

- Continue statement in for loops has a known bug (tests in regression/)
- Logical operators (&&, ||) not yet implemented
- Some advanced features may not work as expected

## Future Enhancements

- Module system for importing test framework
- Parallel test execution
- Better test coverage reporting
- Visual test result dashboard
- Performance regression detection