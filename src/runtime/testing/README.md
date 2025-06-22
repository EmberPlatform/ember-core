# Ember Testing Framework v2.0.3

A comprehensive testing framework for the Ember programming language featuring advanced assertions, test organization, performance benchmarking, and detailed reporting.

## Features

### 🚀 **Core Testing Capabilities**
- **Rich Assertion Library**: 10+ assertion types with detailed error messages
- **Test Organization**: Hierarchical test suites with automatic discovery
- **Performance Benchmarking**: Built-in timing and performance analysis
- **Colorized Output**: Clear, readable test results with status indicators
- **Error Handling**: Comprehensive error testing with exception validation

### 📊 **Advanced Features**
- **Memory Testing**: Integration with valgrind for memory leak detection
- **Coverage Analysis**: Code coverage reporting with gcov integration
- **Parallel Testing**: Support for concurrent test execution
- **Custom Matchers**: Extensible assertion framework
- **Test Filtering**: Run specific test suites or individual tests

## Quick Start

### Building the Framework

```bash
# Build the complete framework
make test-framework

# Run all tests
make test-all

# Run specific test suites
make test-collections
make test-regex
make test-core
```

### Writing Tests

```c
#include "testing_framework.h"

// Define a test function
TEST_DEFINE(my_test) {
    ember_value number = ember_make_number(42);
    TEST_ASSERT(number.type == EMBER_VAL_NUMBER, "Should create number");
    TEST_ASSERT_NUM_EQ(42.0, number.as.number_val, 0.001, "Should have correct value");
}

// Create a test suite
TEST_SUITE(my_suite) {
    ember_vm* vm = ember_new_vm();
    test_suite* suite = test_runner_add_suite(runner, "My Test Suite");
    
    test_suite_add_test(suite, "My Test", test_my_test, vm);
    
    ember_free_vm(vm);
}
```

## API Reference

### Core Functions

```c
// Test Runner Management
test_runner* test_runner_create(void);
void test_runner_free(test_runner* runner);
test_suite* test_runner_add_suite(test_runner* runner, const char* name);
void test_runner_run_all(test_runner* runner);
void test_runner_print_results(test_runner* runner);

// Test Suite Management
void test_suite_add_test(test_suite* suite, const char* name, 
                         test_function func, ember_vm* vm);
```

### Assertion Functions

```c
// Basic Assertions
void test_assert_true(ember_vm* vm, int condition, const char* message);
void test_assert_false(ember_vm* vm, int condition, const char* message);

// Value Assertions
void test_assert_equal(ember_vm* vm, ember_value expected, 
                       ember_value actual, const char* message);
void test_assert_not_equal(ember_vm* vm, ember_value expected, 
                           ember_value actual, const char* message);
void test_assert_null(ember_vm* vm, ember_value value, const char* message);
void test_assert_not_null(ember_vm* vm, ember_value value, const char* message);

// Type-Specific Assertions
void test_assert_string_equal(ember_vm* vm, const char* expected, 
                              const char* actual, const char* message);
void test_assert_number_equal(ember_vm* vm, double expected, double actual, 
                              double tolerance, const char* message);
void test_assert_array_equal(ember_vm* vm, ember_value expected, 
                             ember_value actual, const char* message);

// Exception Testing
void test_assert_throws(ember_vm* vm, void (*func)(ember_vm*), 
                        const char* expected_error, const char* message);
```

### Utility Macros

```c
#define TEST_ASSERT(condition, message)
#define TEST_ASSERT_FALSE(condition, message)
#define TEST_ASSERT_EQ(expected, actual, message)
#define TEST_ASSERT_NEQ(expected, actual, message)
#define TEST_ASSERT_NULL(value, message)
#define TEST_ASSERT_NOT_NULL(value, message)
#define TEST_ASSERT_STR_EQ(expected, actual, message)
#define TEST_ASSERT_NUM_EQ(expected, actual, tolerance, message)

#define TEST_DEFINE(test_name)
#define TEST_SUITE(suite_name)
```

## Test Suites

### Collections Framework Tests
- **Set Operations**: Creation, add, delete, has, clear, equality
- **Map Operations**: Creation, set, get, delete, has, clear, equality
- **Performance Tests**: Benchmarks for common operations
- **Error Handling**: Edge cases and error conditions

### Regular Expressions Tests
- **Pattern Matching**: Basic and advanced pattern testing
- **Flags Support**: Case insensitive, multiline, global, dotall
- **Operations**: Test, match, replace, split functionality
- **Performance**: Regex operation benchmarks
- **Error Cases**: Invalid patterns and null handling

### Core Language Tests
- **Value Creation**: All value types (number, string, boolean, nil)
- **Type System**: Type checking and conversion
- **Memory Management**: Object allocation and garbage collection
- **Array Operations**: Creation, manipulation, access
- **Error Handling**: Exception throwing and catching

## Performance Benchmarking

```c
// Benchmark a function
void test_benchmark(const char* name, void (*func)(ember_vm*), 
                    ember_vm* vm, int iterations);

// Example benchmark
void benchmark_array_operations(ember_vm* vm) {
    ember_value array = ember_make_array(vm, 1000);
    ember_array* arr = AS_ARRAY(array);
    
    for (int i = 0; i < 1000; i++) {
        array_push(arr, ember_make_number(i));
    }
}

TEST_DEFINE(performance_test) {
    test_benchmark("Array Operations", benchmark_array_operations, vm, 100);
}
```

## Advanced Usage

### Memory Testing

```bash
# Run with memory leak detection
make memtest

# Generate coverage report
make coverage
```

### Custom Test Configuration

```c
test_runner* runner = test_runner_create();
runner->verbose = 1;           // Enable verbose output
runner->stop_on_failure = 1;   // Stop on first failure
```

### Test Filtering

```bash
# Run specific test suite
./ember_test_runner --suite collections

# Run with benchmarks
./ember_test_runner --benchmark
```

## Output Format

```
=== Ember Testing Framework ===
Running 3 test suite(s)

[1/3] Core Functionality
  PASS VM Creation (0.12ms)
  PASS Value Creation (0.08ms)
  PASS Value Equality (0.15ms)
  PASS Array Operations (0.23ms)
  Suite Summary: 4/4 passed (0.58ms)

[2/3] Collections Framework
  PASS Set Creation (0.09ms)
  PASS Set Operations (0.18ms)
  PASS Map Creation (0.11ms)
  PASS Map Operations (0.22ms)
  Suite Summary: 4/4 passed (0.60ms)

[3/3] Regular Expressions
  PASS Regex Creation (0.13ms)
  PASS Regex Operations (0.25ms)
  PASS Regex Performance (1.45ms)
  Suite Summary: 3/3 passed (1.83ms)

=== Test Results ===
Total Tests: 11
Passed: 11
Total Duration: 3.01ms
Success Rate: 100.0%

✓ ALL TESTS PASSED
```

## Contributing

When adding new test suites:

1. Create test files in `src/runtime/testing/`
2. Follow naming convention: `test_<feature>.c`
3. Use provided macros and assertions
4. Update the main runner to include new suites
5. Add documentation for new assertion types

## Integration

The testing framework integrates with:
- **CI/CD Pipelines**: Return codes for automated testing
- **Memory Tools**: Valgrind, AddressSanitizer integration
- **Coverage Tools**: gcov, lcov compatibility
- **Build Systems**: Make, CMake, Bazel support