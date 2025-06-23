#!/bin/bash

echo "Building minimal working Ember interpreter..."

# Create build directory
mkdir -p build

# Create stub header
mkdir -p build/include
cat > build/include/ember_native_functions.h << 'EOF'
#ifndef EMBER_NATIVE_FUNCTIONS_H
#define EMBER_NATIVE_FUNCTIONS_H
#include "../../include/ember.h"
// Stub header for native functions
#endif
EOF

# Create minimal builtins that don't reference external libraries
cat > build/builtins_minimal.c << 'EOF'
#include "../include/ember.h"
#include "../src/runtime/value/value.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

// Basic print function
ember_value ember_native_print(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    for (int i = 0; i < argc; i++) {
        if (i > 0) printf(" ");
        
        if (argv[i].type == EMBER_VAL_NUMBER) {
            printf("%g", argv[i].as.number_val);
        } else if (argv[i].type == EMBER_VAL_STRING) {
            if (argv[i].as.string_val) {
                printf("%s", argv[i].as.string_val);
            }
        } else if (argv[i].type == EMBER_VAL_BOOL) {
            printf("%s", argv[i].as.bool_val ? "true" : "false");
        } else if (argv[i].type == EMBER_VAL_NIL) {
            printf("nil");
        }
    }
    printf("\n");
    return ember_make_nil();
}

// Basic math functions
ember_value ember_native_abs(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(fabs(argv[0].as.number_val));
}

ember_value ember_native_sqrt(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc != 1 || argv[0].type != EMBER_VAL_NUMBER) {
        return ember_make_nil();
    }
    return ember_make_number(sqrt(argv[0].as.number_val));
}

ember_value ember_native_max(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc < 2) return ember_make_nil();
    
    double max_val = argv[0].as.number_val;
    for (int i = 1; i < argc; i++) {
        if (argv[i].type == EMBER_VAL_NUMBER && argv[i].as.number_val > max_val) {
            max_val = argv[i].as.number_val;
        }
    }
    return ember_make_number(max_val);
}

ember_value ember_native_min(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm;
    if (argc < 2) return ember_make_nil();
    
    double min_val = argv[0].as.number_val;
    for (int i = 1; i < argc; i++) {
        if (argv[i].type == EMBER_VAL_NUMBER && argv[i].as.number_val < min_val) {
            min_val = argv[i].as.number_val;
        }
    }
    return ember_make_number(min_val);
}

// Function to register built-in functions
void register_builtin_functions(ember_vm* vm) {
    if (!vm) return;
    
    ember_register_native_function(vm, "print", ember_native_print);
    ember_register_native_function(vm, "abs", ember_native_abs);
    ember_register_native_function(vm, "sqrt", ember_native_sqrt);
    ember_register_native_function(vm, "max", ember_native_max);
    ember_register_native_function(vm, "min", ember_native_min);
}

// Stub functions for missing dependencies
ember_value ember_json_parse(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("json_parse_stub");
}

ember_value ember_json_stringify(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("json_stringify_stub");
}

ember_value ember_json_validate(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_bool(1);
}

ember_value ember_native_sha256(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("sha256_stub");
}

ember_value ember_native_sha512(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("sha512_stub");
}

ember_value ember_native_hmac_sha256(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("hmac_sha256_stub");
}

ember_value ember_native_secure_random(ember_vm* vm, int argc, ember_value* argv) {
    (void)vm; (void)argc; (void)argv;
    return ember_make_string("secure_random_stub");
}

// GC function stub
ember_value ember_make_string_gc(ember_vm* vm, const char* str) {
    (void)vm;
    return ember_make_string(str);
}

// Function chunk tracking
void track_function_chunk(ember_vm* vm, ember_chunk* chunk) {
    (void)vm; (void)chunk;
    // Stub - in real implementation would track for cleanup
}

// Common string handling
const char* ember_get_string_value(ember_value val) {
    if (val.type == EMBER_VAL_STRING) {
        return val.as.string_val;
    }
    return NULL;
}
EOF

# Core source files without problematic dependencies
CORE_SOURCES="
src/frontend/lexer/lexer.c
src/frontend/parser/parser.c
src/frontend/parser/core.c
src/frontend/parser/statements.c
src/frontend/parser/oop.c
src/core/vm.c
src/core/vm_arithmetic.c
src/core/vm_comparison.c
src/core/vm_stack.c
src/core/bytecode.c
src/core/memory.c
src/core/error.c
src/runtime/value/value.c
src/runtime/crypto_simple.c
src/runtime/json_simple.c
src/runtime/io_simple.c
build/builtins_minimal.c
tools/ember/ember.c
"

# Try expressions.c if it exists and has content
if [ -s "src/frontend/parser/expressions.c" ]; then
    CORE_SOURCES="$CORE_SOURCES src/frontend/parser/expressions.c"
fi

# Compile with basic flags, ignoring warnings for now
echo "Compiling source files..."
gcc -std=c99 -Iinclude -Ibuild/include -DEMBER_VERSION_STRING=\"2.0.3\" \
    -DHAVE_READLINE -O0 -g \
    $CORE_SOURCES \
    -o build/ember_simple \
    -lm -lpthread 2>/dev/null

if [ $? -eq 0 ]; then
    echo "✅ Build successful!"
    echo "Executable: build/ember_simple"
    echo "Testing basic functionality..."
    echo 'print("Hello from Ember!")' | ./build/ember_simple
else
    echo "❌ Build failed, trying with minimal sources..."
    
    # Even more minimal build
    MINIMAL_SOURCES="
    src/frontend/lexer/lexer.c
    src/frontend/parser/parser.c
    src/frontend/parser/core.c
    src/frontend/parser/statements.c
    src/core/vm.c
    src/runtime/value/value.c
    build/builtins_minimal.c
    tools/ember/ember.c
    "
    
    gcc -std=c99 -Iinclude -Ibuild/include -DEMBER_VERSION_STRING=\"2.0.3\" \
        -O0 -g -w \
        $MINIMAL_SOURCES \
        -o build/ember_minimal \
        -lm 2>/dev/null
    
    if [ $? -eq 0 ]; then
        echo "✅ Minimal build successful!"
        echo "Executable: build/ember_minimal"
        echo 'print("Hello from Ember!")' | ./build/ember_minimal
    else
        echo "❌ Even minimal build failed"
        exit 1
    fi
fi