# Ember Fuzzing Test Suite

This directory contains fuzzing tests for the Ember programming language implementation, designed to find crashes, hangs, and other robustness issues in the parser and VM components.

## Overview

The fuzzing suite includes:

- **Parser Fuzzing**: Tests the parser with malformed and random input to find parsing crashes
- **VM Fuzzing**: Tests the virtual machine with malformed bytecode sequences
- **Comprehensive Fuzzing**: Combines both parser and VM testing with various input types

## Files

- `fuzz_parser.c` - Standalone parser fuzzer
- `fuzz_vm.c` - Standalone VM fuzzer  
- `fuzz_comprehensive.c` - Combined fuzzer with multiple test modes
- `fuzz_common.h` / `fuzz_common.c` - Shared utilities and test data generators
- `README.md` - This documentation

## Building

Build all fuzzing targets:
```bash
make fuzz
```

This creates the following executables in the `build/` directory:
- `fuzz-parser` - Parser fuzzing
- `fuzz-vm` - VM fuzzing
- `fuzz-comprehensive` - Comprehensive fuzzing

## Usage

### Basic Fuzzing

Run comprehensive fuzzing with default settings (1000 iterations):
```bash
make fuzz-run
```

Run quick fuzzing (100 iterations):
```bash
make fuzz-quick
```

### Individual Components

Test only the parser:
```bash
make fuzz-parser
# or directly:
./build/fuzz-parser 500
```

Test only the VM:
```bash
make fuzz-vm
# or directly:
./build/fuzz-vm 500
```

### Advanced Options

The comprehensive fuzzer supports several modes:

```bash
# Test parser only
./build/fuzz-comprehensive -n 1000 --parser

# Test VM only  
./build/fuzz-comprehensive -n 1000 --vm

# Test integration (parser + VM execution)
./build/fuzz-comprehensive -n 1000 --integration

# Test both parser and VM separately (default)
./build/fuzz-comprehensive -n 1000 --both
```

### Memory Safety Testing

Run fuzzing with AddressSanitizer to catch memory errors:
```bash
make fuzz-asan
```

### Continuous Fuzzing

Use the fuzzing script for extended testing:

```bash
# Run for 10 minutes
./scripts/run_fuzzing.sh -d 600

# Run continuously until interrupted
./scripts/run_fuzzing.sh -c

# Run with AddressSanitizer
./scripts/run_fuzzing.sh -a -d 300

# Test specific component
./scripts/run_fuzzing.sh -t parser -n 500
```

## Understanding Results

### Exit Codes
- `0` - No crashes detected
- `1` - Crashes or significant issues found

### Output Interpretation

**Normal behavior:**
- Parse errors for malformed input (expected)
- Evaluation errors for invalid code (expected)
- Progress messages showing test completion

**Issues to investigate:**
- `CRASH detected` messages
- `TIMEOUT` messages  
- Segmentation faults or other signals
- Infinite loops or excessive output

### Example Normal Output
```
=== Parser Fuzzing Statistics ===
Total tests: 100
Iterations completed: 100
Successes: 15
Errors: 85
Crashes: 0
Timeouts: 0
Success rate: 15.00%
Crash rate: 0.00%
```

### Example Problem Output
```
CRASH detected at iteration 42 with input: 'malformed_input_here'
WARNING: 3 crashes detected during fuzzing!
```

## Test Data Generation

The fuzzing suite generates various types of test data:

1. **Random strings** - Completely random character sequences
2. **Ember keywords** - Valid language constructs in invalid combinations  
3. **Malformed syntax** - Common syntax error patterns
4. **Edge case numbers** - Extreme numeric values and invalid formats
5. **Unicode strings** - Non-ASCII characters and control sequences
6. **Valid code** - Correct programs for baseline testing

## Configuration

Key parameters in the fuzzing code:

- `FUZZ_MAX_INPUT_SIZE` (1024) - Maximum input size in bytes
- `FUZZ_ITERATIONS_DEFAULT` (1000) - Default number of test iterations
- `FUZZ_TIMEOUT_SECONDS` (5) - Timeout per individual test
- `FUZZ_MAX_BYTECODE_SIZE` (512) - Maximum VM bytecode size

## Troubleshooting

### Common Issues

**Build failures:**
- Ensure `make fuzz` builds successfully first
- Check that all dependencies are installed

**Excessive output:**
- Some inputs may cause verbose error reporting
- Use timeouts to prevent infinite loops
- Redirect output to files for analysis: `./build/fuzz-parser 100 > fuzz.log 2>&1`

**False positives:**
- Parse errors are expected for malformed input
- Only actual crashes (segfaults, assertions) indicate problems

### Performance Tips

- Start with small iteration counts (10-100) for testing
- Use AddressSanitizer builds for thorough memory checking
- Run longer tests (1000+ iterations) for comprehensive coverage
- Monitor system resources during extended fuzzing

## Integration with CI/CD

Example CI integration:

```bash
# Quick smoke test
make fuzz-quick

# Thorough testing with memory checking
make fuzz-asan

# Extended testing (time permitting)
timeout 300 ./scripts/run_fuzzing.sh -d 300
```

## Contributing

When adding new fuzzing capabilities:

1. Add test data generators to `fuzz_common.c`
2. Update statistics tracking for new test types
3. Document new command-line options
4. Add corresponding Makefile targets
5. Update this README with usage examples

## Security Considerations

Fuzzing may trigger:
- High CPU usage during testing
- Memory allocation stress
- File system access attempts
- Network requests (if testing network features)

Run fuzzing in isolated environments when testing untrusted code paths.