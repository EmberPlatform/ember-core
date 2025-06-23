# Ember Module System Documentation

The Ember Module System provides ES6-style import/export functionality with additional CommonJS-style require support. It includes module resolution, caching, circular dependency detection, and a comprehensive standard library.

## Table of Contents

1. [Basic Usage](#basic-usage)
2. [Export Syntax](#export-syntax)
3. [Import Syntax](#import-syntax)
4. [Module Resolution](#module-resolution)
5. [Core Modules](#core-modules)
6. [Error Handling](#error-handling)
7. [Advanced Features](#advanced-features)
8. [Examples](#examples)

## Basic Usage

### Exporting from a Module

```ember
// math_utils.ember
export pi = 3.14159265359
export e = 2.71828182846

export function add(a, b) {
    return a + b
}

export function multiply(a, b) {
    return a * b
}

// Default export
export default function calculate(operation, a, b) {
    if operation == "add" {
        return add(a, b)
    } else if operation == "multiply" {
        return multiply(a, b)
    }
    return 0
}
```

### Importing in Another Module

```ember
// app.ember
import Calculator, { add, multiply, pi } from "./math_utils"
import * as MathUtils from "./math_utils"

result1 = add(5, 3)
result2 = Calculator("multiply", 4, 7)
print("PI value:", pi)
print("All math utils:", MathUtils)
```

## Export Syntax

### Named Exports

```ember
// Export variables
export name = "Ember"
export version = "1.0.0"

// Export functions
export function greet(name) {
    return "Hello, " + name
}

// Export multiple items
export { name, version, greet }

// Export with alias
export { greet as sayHello }
```

### Default Export

```ember
// Export default value
export default "This is the default export"

// Export default function
export default function main() {
    print("Main function called")
}

// Export default object
export default {
    name: "MyModule",
    version: "1.0.0",
    init: function() {
        print("Module initialized")
    }
}
```

### Re-exports

```ember
// Re-export named exports
export { add, multiply } from "./math_utils"

// Re-export all exports
export * from "./math_utils"

// Re-export default with new name
export { default as MathCalculator } from "./math_utils"
```

## Import Syntax

### Named Imports

```ember
// Import specific exports
import { add, multiply } from "./math_utils"

// Import with alias
import { add as sum, multiply as product } from "./math_utils"

// Import multiple items
import { pi, e, add, multiply } from "./math_utils"
```

### Default Import

```ember
// Import default export
import Calculator from "./calculator"

// Import default with custom name
import MyCalculator from "./calculator"
```

### Namespace Import

```ember
// Import all exports as namespace
import * as Math from "./math_utils"

// Usage
result = Math.add(5, 3)
print("PI:", Math.pi)
```

### Mixed Imports

```ember
// Import default and named exports
import Calculator, { version, pi } from "./math_utils"

// Import namespace and specific exports
import * as Math, { add } from "./math_utils"
```

### Side-effect Import

```ember
// Import module for side effects only
import "./initialization_module"
```

### Dynamic Import (Function-style)

```ember
// Dynamic import using import() function
math_module = import("math")
result = math_module["add"](5, 3)

// CommonJS-style require
utils = require("./utils")
```

## Module Resolution

The module system resolves modules in the following order:

### 1. Core Modules
Built-in modules are resolved first:
- `math`, `string`, `crypto`, `json`, `io`, `http`
- `path`, `fs`, `os`, `util`

```ember
import { PI } from "math"  // Resolves to core math module
```

### 2. Relative Paths
Paths starting with `./` or `../`:

```ember
import utils from "./utils"        // ./utils.ember
import config from "../config"     // ../config.ember
import data from "./data/users"    // ./data/users.ember
```

### 3. Absolute Paths
Paths starting with `/`:

```ember
import module from "/absolute/path/module"  // /absolute/path/module.ember
```

### 4. Package Resolution
For non-relative, non-absolute paths:

1. `./node_modules/package_name`
2. `./node_modules/package_name/index.ember`
3. `./lib/package_name.ember`
4. `~/.local/lib/ember/package_name.ember`
5. `/usr/lib/ember/package_name.ember`
6. `./package_name.ember`

## Core Modules

### Math Module
```ember
import { PI, E, LN2, LN10, SQRT2 } from "math"
```

### String Module
```ember
import * as String from "string"
```

### Crypto Module
```ember
import * as Crypto from "crypto"
```

### JSON Module
```ember
import * as JSON from "json"
```

### IO Module
```ember
import * as IO from "io"
```

### HTTP Module
```ember
import * as HTTP from "http"
```

### Path Module
```ember
import { sep, delimiter } from "path"
```

### FS Module
```ember
import * as FS from "fs"
```

### OS Module
```ember
import { platform } from "os"
```

### Util Module
```ember
import * as Util from "util"
```

## Error Handling

### Module Not Found
```ember
// Returns nil for non-existent modules
bad_module = import("non_existent")
if bad_module == nil {
    print("Module not found")
}
```

### Circular Dependencies
The module system detects circular dependencies and prevents infinite loops:

```ember
// module_a.ember
import "./module_b"  // This would create a circular dependency
export function funcA() { }

// module_b.ember  
import "./module_a"  // Circular dependency detected
export function funcB() { }
```

### Export Errors
```ember
// Error: export outside module context
export invalid = "This will cause an error in non-module context"
```

## Advanced Features

### Module Caching
Modules are cached after first load for performance:

```ember
mod1 = import("math")  // Loads and caches
mod2 = import("math")  // Returns cached version
// mod1 === mod2 (same object)
```

### Module Context
Each module has its own execution context:

```ember
// In module_a.ember
local_var = "This is local to module A"
export function getLocal() {
    return local_var
}

// In module_b.ember  
local_var = "This is local to module B"  // Different variable
import { getLocal } from "./module_a"
print(getLocal())  // Prints "This is local to module A"
```

### Module Loading States
Modules track their loading state to handle circular dependencies:
- `is_loading`: Currently being loaded
- `is_loaded`: Fully loaded and cached

### Security Features
- Input validation for module paths
- Prevention of path traversal attacks
- Sandboxed module execution

## Examples

### Complete Example: Calculator Module

**math_operations.ember**
```ember
export function add(a, b) { return a + b }
export function subtract(a, b) { return a - b }
export function multiply(a, b) { return a * b }
export function divide(a, b) { 
    if b == 0 {
        throw "Division by zero"
    }
    return a / b 
}

export pi = 3.14159265359
```

**calculator.ember**
```ember
import { add, subtract, multiply, divide } from "./math_operations"

export default function Calculator() {
    this.history = []
    
    this.calculate = function(op, a, b) {
        result = nil
        
        if op == "add" {
            result = add(a, b)
        } else if op == "subtract" {
            result = subtract(a, b)
        } else if op == "multiply" {
            result = multiply(a, b)
        } else if op == "divide" {
            result = divide(a, b)
        }
        
        if result != nil {
            this.history.push(str(a) + " " + op + " " + str(b) + " = " + str(result))
        }
        
        return result
    }
    
    this.getHistory = function() {
        return this.history
    }
    
    return this
}

export version = "1.0.0"
```

**app.ember**
```ember
import Calculator, { version } from "./calculator"
import { pi } from "./math_operations"
import * as MathOps from "./math_operations"

print("Calculator version:", version)
print("PI constant:", pi)

calc = Calculator()
result1 = calc.calculate("add", 10, 5)
result2 = calc.calculate("multiply", 7, 8)

print("Results:", calc.getHistory())

// Using namespace import
area = MathOps.multiply(MathOps.pi, MathOps.multiply(5, 5))
print("Circle area (r=5):", area)
```

### Core Module Usage Example

```ember
// Using multiple core modules
import { PI, E } from "math"
import { platform } from "os"
import { sep } from "path"
import * as JSON from "json"

print("Platform:", platform)
print("Path separator:", sep)
print("Math constants - PI:", PI, "E:", E)

data = {
    platform: platform,
    constants: {
        pi: PI,
        e: E
    }
}

json_string = JSON.stringify(data)
print("JSON:", json_string)

parsed = JSON.parse(json_string)
print("Parsed platform:", parsed["platform"])
```

## Best Practices

1. **Use descriptive export names**
2. **Prefer named exports for utilities**
3. **Use default exports for main functionality**
4. **Group related exports together**
5. **Document module interfaces**
6. **Handle errors gracefully**
7. **Use relative imports for local modules**
8. **Import only what you need**

## Performance Considerations

- Modules are cached after first load
- Circular dependency detection has minimal overhead
- Core modules are pre-loaded and optimized
- Use namespace imports sparingly for large modules

## Compatibility

The Ember module system is designed to be compatible with:
- ES6 module syntax
- CommonJS require() patterns
- Node.js module resolution algorithms
- Modern JavaScript module standards

## Troubleshooting

### Common Issues

1. **Module not found**: Check file paths and extensions
2. **Circular dependencies**: Refactor to break the cycle
3. **Export/import mismatches**: Verify export names
4. **Path resolution**: Use relative paths for local modules

### Debug Tips

- Use `print()` statements to trace module loading
- Check module return values for `nil`
- Verify file extensions (.ember)
- Test with simple modules first