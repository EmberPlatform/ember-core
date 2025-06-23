# Ember Platform Exception Handling Guide

## Overview

The Ember Platform provides a comprehensive exception handling system that supports modern error management patterns including try/catch/finally blocks, exception chaining, detailed stack traces, and built-in exception types.

## Table of Contents

1. [Basic Exception Handling](#basic-exception-handling)
2. [Built-in Exception Types](#built-in-exception-types)
3. [Exception Object Structure](#exception-object-structure)
4. [Advanced Features](#advanced-features)
5. [Best Practices](#best-practices)
6. [API Reference](#api-reference)
7. [Examples](#examples)

## Basic Exception Handling

### Try/Catch/Finally Syntax

```ember
try {
    // Code that might throw an exception
    throw "Something went wrong"
} catch (e) {
    // Handle the exception
    print("Caught: " + str(e))
} finally {
    // Cleanup code (always executed)
    print("Cleanup complete")
}
```

### Key Features

- **Try blocks**: Contain code that might throw exceptions
- **Catch blocks**: Handle exceptions with optional variable binding
- **Finally blocks**: Execute cleanup code regardless of exceptions
- **Exception propagation**: Unhandled exceptions bubble up the call stack
- **Stack unwinding**: Proper cleanup of resources during exception propagation

## Built-in Exception Types

The Ember Platform provides several built-in exception types for common error scenarios:

### Core Exception Types

| Type | Description | Use Cases |
|------|-------------|-----------|
| `Error` | Generic error | General error conditions |
| `TypeError` | Type-related errors | Invalid type operations, type mismatches |
| `RuntimeError` | Runtime execution errors | Execution failures, state errors |
| `SyntaxError` | Syntax errors | Parse-time syntax issues |
| `ReferenceError` | Reference errors | Undefined variables, invalid references |
| `RangeError` | Range/bounds errors | Array bounds, numeric ranges |
| `MemoryError` | Memory allocation errors | Out of memory conditions |
| `SecurityError` | Security violations | Permission denied, security constraints |
| `IOError` | Input/output errors | File operations, stream errors |
| `NetworkError` | Network-related errors | Connection failures, timeouts |
| `TimeoutError` | Timeout conditions | Operation timeouts |
| `AssertionError` | Assertion failures | Failed assertions, contract violations |

### Creating Specific Exception Types

```ember
// Using built-in exception creation functions
var type_err = TypeError("Expected number, got string")
var range_err = RangeError("Index 5 out of bounds for array of length 3")
var io_err = IOError("Failed to read file: permission denied")
```

## Exception Object Structure

Each exception object contains detailed information for debugging and error handling:

```ember
{
    type: "TypeError",              // Exception type name
    message: "Expected number",     // Error message
    file_name: "script.ember",      // Source file name
    line_number: 42,                // Line number where exception occurred
    column_number: 15,              // Column number
    stack_trace: [...],             // Array of stack frames
    cause: nil,                     // Caused-by exception (for chaining)
    data: nil,                      // Additional data attached to exception
    timestamp: 1640995200,          // When exception was created
    suppressed: []                  // Array of suppressed exceptions
}
```

### Stack Trace Information

Each stack frame contains:
- Function name (or `<script>` for top-level)
- File name and location
- Line and column numbers
- Local variables at the time of exception (debugging)

## Advanced Features

### Exception Chaining

Link exceptions to preserve error context:

```ember
try {
    // Low-level operation that fails
    database_connect()
} catch (db_error) {
    // Wrap with higher-level context
    var user_error = Error("Failed to load user data")
    user_error.cause = db_error
    throw user_error
}
```

### Multiple Catch Blocks (Future Feature)

Handle different exception types differently:

```ember
try {
    risky_operation()
} catch (TypeError as type_err) {
    // Handle type errors specifically
    print("Type error: " + type_err.message)
} catch (IOError as io_err) {
    // Handle I/O errors specifically
    print("I/O error: " + io_err.message)
} catch (e) {
    // Handle all other exceptions
    print("Unexpected error: " + str(e))
}
```

### Resource Management

Use finally blocks for guaranteed cleanup:

```ember
var file = nil
try {
    file = open("data.txt")
    process_file(file)
} catch (e) {
    print("Error processing file: " + str(e))
} finally {
    if file != nil {
        close(file)
    }
}
```

## Best Practices

### 1. Use Specific Exception Types

```ember
// Good: Specific exception type
if age < 0 {
    throw RangeError("Age cannot be negative: " + str(age))
}

// Avoid: Generic error
if age < 0 {
    throw "Invalid age"
}
```

### 2. Preserve Exception Context

```ember
// Good: Preserve original exception
try {
    low_level_operation()
} catch (original) {
    var wrapped = Error("High-level operation failed")
    wrapped.cause = original
    throw wrapped
}

// Avoid: Losing original context
try {
    low_level_operation()
} catch (original) {
    throw "Something went wrong"
}
```

### 3. Use Finally for Cleanup

```ember
// Good: Guaranteed cleanup
var resource = nil
try {
    resource = acquire_resource()
    use_resource(resource)
} finally {
    if resource != nil {
        release_resource(resource)
    }
}
```

### 4. Handle Exceptions at Appropriate Levels

```ember
// Good: Handle at the right abstraction level
function user_service_get_user(id) {
    try {
        return database_get_user(id)
    } catch (DatabaseError as db_err) {
        // Log technical error
        log_error("Database error: " + db_err.message)
        // Throw user-friendly error
        throw UserNotFoundError("User " + str(id) + " not found")
    }
}
```

### 5. Don't Catch and Ignore

```ember
// Avoid: Silent failure
try {
    important_operation()
} catch (e) {
    // Don't silently ignore exceptions
}

// Good: At least log the error
try {
    important_operation()
} catch (e) {
    log_error("Important operation failed: " + str(e))
    // Re-throw if you can't handle it
    throw e
}
```

## API Reference

### Exception Creation Functions

```ember
// Generic exception
Error(message)
ember_make_exception(vm, type, message)
ember_make_exception_detailed(vm, type, message, file, line, column)

// Specific exception types
TypeError(message)
RuntimeError(message)
SyntaxError(message)
ReferenceError(message)
RangeError(message)
MemoryError(message)
SecurityError(message)
IOError(message)
NetworkError(message)
TimeoutError(message)
AssertionError(message)
```

### Exception Manipulation

```ember
// Exception chaining
exception.set_cause(cause_exception)
exception.add_suppressed(suppressed_exception)

// Exception information
exception.get_stack_trace_string()
exception.matches_type(exception_type)
exception.get_root_cause()

// Exception wrapping
wrap_exception(original_exception, new_message)
```

### Utility Functions

```ember
// Type checking
is_exception(value)
exception_type_to_string(type)
value_type_to_string(type)

// Stack trace utilities
capture_stack_trace(exception)
print_exception_details(exception)
```

## Examples

### Basic Error Handling

```ember
function divide(a, b) {
    try {
        if b == 0 {
            throw RangeError("Division by zero")
        }
        return a / b
    } catch (e) {
        print("Math error: " + str(e))
        return nil
    }
}

var result = divide(10, 0)  // Prints: Math error: Division by zero
```

### File Processing with Resource Management

```ember
function process_file(filename) {
    var file = nil
    try {
        file = open(filename)
        if file == nil {
            throw IOError("Could not open file: " + filename)
        }
        
        var content = read_all(file)
        return process_content(content)
        
    } catch (IOError as io_err) {
        print("File error: " + io_err.message)
        return nil
    } catch (e) {
        print("Unexpected error: " + str(e))
        return nil
    } finally {
        if file != nil {
            close(file)
        }
    }
}
```

### Exception Chaining

```ember
function user_service_authenticate(username, password) {
    try {
        return database_verify_credentials(username, password)
    } catch (DatabaseConnectionError as db_err) {
        var auth_err = AuthenticationError("Authentication service unavailable")
        auth_err.cause = db_err
        throw auth_err
    } catch (InvalidCredentialsError as cred_err) {
        throw AuthenticationError("Invalid username or password")
    }
}
```

### Circuit Breaker Pattern

```ember
var circuit_state = "CLOSED"
var failure_count = 0

function resilient_service_call(request) {
    if circuit_state == "OPEN" {
        throw ServiceUnavailableError("Service circuit breaker is open")
    }
    
    try {
        var result = external_service_call(request)
        failure_count = 0
        if circuit_state == "HALF_OPEN" {
            circuit_state = "CLOSED"
        }
        return result
    } catch (e) {
        failure_count = failure_count + 1
        if failure_count >= 5 {
            circuit_state = "OPEN"
        }
        throw e
    }
}
```

## Performance Considerations

1. **Normal Execution**: Exception handling infrastructure has minimal impact on normal (non-throwing) code paths
2. **Stack Traces**: Stack trace capture has overhead but provides valuable debugging information
3. **Exception Objects**: Exception objects are garbage collected like other objects
4. **Finally Blocks**: Finally blocks execute even during exception propagation
5. **Type Checking**: Exception type matching is optimized for common cases

## Integration with Standard Library

The exception handling system integrates with Ember's standard library:

- **File I/O**: Throws `IOError` for file operation failures
- **Network**: Throws `NetworkError` for connection issues
- **JSON**: Throws `SyntaxError` for malformed JSON
- **Math**: Throws `RangeError` for domain errors
- **Crypto**: Throws `SecurityError` for cryptographic failures

## Debugging Tips

1. **Use detailed exception messages** with context information
2. **Examine stack traces** to understand error propagation
3. **Check exception chains** for root causes
4. **Use appropriate exception types** for better categorization
5. **Log exceptions** at appropriate levels for monitoring

## Migration from Simple Errors

If upgrading from simple string-based error handling:

```ember
// Old style
if error_condition {
    return "Error: something went wrong"
}

// New style
if error_condition {
    throw Error("something went wrong")
}
```

The enhanced exception system provides much richer error information and better error handling patterns for robust applications.