# Ember Platform Development Standards

**Effective Date**: June 2025  
**Version**: 1.0  
**Authority**: Project Lead Developer

## Core Principles

1. **Code Quality Over Feature Quantity**: A working interpreter with 10 features is better than a broken one with 100
2. **Honest Documentation**: Documentation MUST reflect actual state, not aspirational features
3. **Test Before Merge**: All changes must pass automated tests before merging
4. **Security First**: All code must undergo security review before production use

## Development Workflow

### Branch Strategy

**MANDATORY**: All development must use feature branches. Direct commits to master/main are FORBIDDEN.

```bash
# Create feature branch
git checkout -b feature/description
git checkout -b fix/bug-description
git checkout -b refactor/component-description
git checkout -b docs/documentation-update

# Example branch names:
feature/add-string-methods
fix/logical-operator-stack-underflow
refactor/vm-memory-management
docs/update-working-features
```

### Pull Request Requirements

Every PR must include:

1. **Clear Description**: What changes and why
2. **Testing Evidence**: Output from test runs
3. **Documentation Updates**: Any docs affected by changes
4. **Security Review**: For any I/O, parsing, or memory operations
5. **Performance Impact**: For VM or parser changes

### PR Template

```markdown
## Description
Brief description of changes

## Type of Change
- [ ] Bug fix
- [ ] New feature
- [ ] Refactoring
- [ ] Documentation update

## Testing
- [ ] All existing tests pass
- [ ] New tests added for new functionality
- [ ] Manual testing completed

## Documentation
- [ ] CLAUDE.md updated if needed
- [ ] API documentation updated
- [ ] Examples updated

## Security Checklist
- [ ] No hardcoded credentials
- [ ] Input validation added
- [ ] Memory safety verified
- [ ] No buffer overflows

## Build Verification
```bash
# Paste output of:
make clean && make 2>&1 | grep -i warning | wc -l
# Should be 0 warnings
```
```

## Testing Requirements

### Before EVERY Commit

```bash
# 1. Build without warnings
cd repos/ember-core
make clean && make 2>&1 | tee build.log
grep -i warning build.log  # Should return nothing

# 2. Run core functionality tests
./build/ember test_control_flow_simple.ember
./build/ember working_features_test.ember

# 3. Run regression tests
./scripts/run_regression_tests.sh  # To be created

# 4. Check for memory leaks (if changed C code)
valgrind --leak-check=full ./build/ember test_script.ember
```

### Automated Testing

All repos must have:
- `make test` target that runs all tests
- `make test-security` for security-specific tests
- `make test-performance` for performance regression tests

### Test Coverage Requirements

- Core VM operations: 100% coverage required
- Parser functions: 95% coverage required
- Built-in functions: 90% coverage required
- Standard library: 80% coverage required

## Code Style Guidelines

### C Code
```c
// Function naming: module_action_target
void parser_parse_expression(parser* p);
void vm_execute_instruction(ember_vm* vm);

// Error handling: ALWAYS check returns
if (result != EMBER_OK) {
    ember_set_error(vm, "Operation failed: %s", error_msg);
    return EMBER_ERROR;
}

// Memory: ALWAYS free what you allocate
char* buffer = malloc(size);
if (!buffer) return EMBER_ERROR_OOM;
// ... use buffer ...
free(buffer);
```

### Ember Code
```ember
// Use descriptive names
function calculate_fibonacci(n) {
    if (n < 2) return n;
    return calculate_fibonacci(n-1) + calculate_fibonacci(n-2);
}

// Handle errors explicitly
try {
    result = risky_operation();
} catch (e) {
    print("Error:", e.message);
    return nil;
}
```

## Refactoring Guidelines

### When to Refactor
- Before adding new features to messy code
- When fixing bugs reveals design issues
- When performance profiling shows bottlenecks
- When code violates these standards

### Refactoring Process

1. **Document Current Behavior**: Write tests that pass with current code
2. **Create Refactoring Plan**: Document in PR what will change
3. **Refactor Incrementally**: Small, testable changes
4. **Verify No Regressions**: All tests must still pass
5. **Update Documentation**: Reflect new structure

### MANDATORY: Complete All Refactors

**NEVER** leave a refactoring partially done. If you start, you must:
- Complete all code changes
- Update all affected documentation
- Fix all broken tests
- Update examples

## Security Requirements

### Security Audit Checklist

Before merging any PR that touches:
- Parser/lexer: Check for input validation
- VM execution: Check for stack bounds
- Memory management: Check for leaks/overflows
- File I/O: Check for path traversal
- Native functions: Check for injection attacks

### Security Testing

```bash
# Run security test suite
cd repos/ember-tests
./security_scripts/run_security_audit.sh

# Fuzzing (for parser changes)
./security_scripts/run_fuzzing_tests.sh

# Memory safety (for C changes)
make ASAN=1 test-all
```

## Performance Guidelines

### Performance Regression Testing

Any PR affecting VM, parser, or core loops must:

```bash
# Run benchmarks before changes
./ember-bench --save-baseline before.json

# Make changes

# Run benchmarks after changes
./ember-bench --compare before.json

# Regression threshold: <5% slowdown acceptable
```

## Documentation Standards

### Keep Docs Current

**RULE**: If code changes, docs must change in the SAME PR.

Priority documentation:
1. `CLAUDE.md` - Development guide (keep concise)
2. `WORKING_FEATURES.md` - What actually works
3. `API_REFERENCE.md` - Function signatures
4. Examples in `examples/` - Working code

### Documentation Honesty

**FORBIDDEN**:
- Documenting unimplemented features as working
- Leaving outdated examples
- Aspirational documentation without "PLANNED:" prefix

## Communication with AI Assistants

When working with Claude (Opus, Sonnet, or Haiku):

1. **Be Explicit**: "Fix bug X in file Y, update tests and docs"
2. **Provide Context**: Share relevant code sections
3. **Verify Output**: Always test AI-generated code
4. **Document AI Use**: Note in PR if AI-assisted

### CLAUDE.md Guidelines for AI

Keep instructions:
- Under 500 lines
- Focused on current state
- With working examples
- Without aspirational features

## Build and Release Standards

### Build Health

**ZERO TOLERANCE** for:
- Compilation warnings in master branch
- Failing tests in master branch
- Broken examples in master branch

### Release Checklist

- [ ] All tests pass
- [ ] No compilation warnings
- [ ] Security audit complete
- [ ] Performance benchmarks acceptable
- [ ] Documentation current
- [ ] Examples working

## Enforcement

These standards are MANDATORY. PRs violating these standards will be:
1. Rejected with specific feedback
2. Required to fix issues before re-review
3. Blocked from merging until compliant

## Evolution

These standards will evolve. To propose changes:
1. Create issue describing problem
2. Propose specific solution
3. Get consensus from team
4. Update via PR with "standards" label

---

**Remember**: Good development practices prevent technical debt. Follow these standards to keep Ember maintainable and reliable.