#!/usr/bin/env python3
import re
import sys

def fix_all_unused_variables(filename):
    """Add UNUSED() calls for all unused variables in test files"""
    
    with open(filename, 'r') as f:
        content = f.read()
    
    # Add UNUSED macro if not present
    if '#define UNUSED(x)' not in content:
        content = content.replace('#include "test_ember_internal.h"', 
                                '#include "test_ember_internal.h"\n\n// Macro to mark variables as intentionally unused\n#define UNUSED(x) ((void)(x))')
    
    # Pattern to find all variable declarations that might be unused
    # This is a broad pattern that matches most C variable declarations
    patterns = [
        r'(\s+)(ember_value\s+\w+\s*=\s*[^;]+;)',          # ember_value
        r'(\s+)(const\s+char\*\s+\w+\s*=\s*[^;]+;)',      # const char*
        r'(\s+)(ember_\w+\*\s+\w+\s*=\s*[^;]+;)',         # ember_*
        r'(\s+)(uint32_t\s+\w+\s*=\s*[^;]+;)',            # uint32_t
        r'(\s+)(bool\s+\w+\s*=\s*[^;]+;)',                # bool
        r'(\s+)(EmberPackage\*\s+\w+\s*=\s*[^;]+;)',      # EmberPackage*
        r'(\s+)(int\s+\w+\s*=\s*[^;]+;)',                 # int (simple cases)
    ]
    
    for pattern in patterns:
        def replacement(match):
            indent = match.group(1)
            assignment = match.group(2)
            
            # Extract variable name - look for the pattern: type name = ...
            var_match = re.search(r'(?:ember_\w+\*|const\s+char\*|uint32_t|bool|EmberPackage\*|int|ember_value)\s+(\w+)', assignment)
            if var_match:
                var_name = var_match.group(1)
                return f"{indent}{assignment}\n{indent}UNUSED({var_name});"
            return match.group(0)
        
        content = re.sub(pattern, replacement, content)
    
    with open(filename, 'w') as f:
        f.write(content)

if __name__ == "__main__":
    if len(sys.argv) != 2:
        print("Usage: python3 comprehensive_fix.py <filename>")
        sys.exit(1)
    
    fix_all_unused_variables(sys.argv[1])
    print(f"Fixed unused variables in {sys.argv[1]}")