# Security Policy

## Supported Versions

We actively maintain security updates for the following versions of Ember Core:

| Version | Supported |
| ------- | --------- |
| 2.0.3   | ✅ |
| 2.0.x   | ✅ |
| 1.x.x   | ❌ |

### End-of-Life Policy

- **Major versions** are supported for 2 years after release
- **Security patches** are backported to supported versions
- **End-of-life dates** are announced 6 months in advance

## Reporting a Vulnerability

### Security Contact

**Please do not report security vulnerabilities through public GitHub issues.**

Instead, please report security issues via email to:

**security@emberplatform.org**

### What to Include

When reporting a security vulnerability in Ember Core, please include:

1. **Description**: A clear description of the vulnerability
2. **Component**: Which part of the interpreter is affected (parser, VM, built-ins, etc.)
3. **Version**: The version(s) where you discovered the issue
4. **Reproduction**: Step-by-step instructions to reproduce the issue
5. **Impact**: Your assessment of the potential impact
6. **Evidence**: Any proof-of-concept code or scripts (if applicable)
7. **Suggested fix**: If you have ideas for remediation

### Example Report Template

```
Subject: [SECURITY] Memory corruption in parser

Component: ember-core parser
Version: 2.0.3
Severity: High

Description:
Buffer overflow in the string literal parsing function when processing 
strings longer than 4096 characters.

Steps to Reproduce:
1. Create an Ember script with a string literal > 4096 characters
2. Run: ./build/ember malicious_script.ember
3. Observe segmentation fault

Impact:
Potential for arbitrary code execution through carefully crafted 
input scripts.

Evidence:
[Attached: proof_of_concept.ember]

Suggested Fix:
Add bounds checking in parse_string_literal() function

Contact: researcher@example.com
```

## Response Process

### Acknowledgment

- **24 hours**: We will acknowledge receipt of your report
- **Initial assessment**: Within 48 hours we will provide an initial assessment
- **Regular updates**: We will provide updates every 7 days until resolution

### Investigation Timeline

1. **Initial triage** (1-2 business days)
   - Validate the vulnerability
   - Assess severity and impact
   - Assign internal tracking number

2. **Investigation** (3-10 business days)
   - Detailed analysis of the issue
   - Develop and test fix
   - Coordinate with other maintainers if needed

3. **Resolution** (1-7 business days after fix development)
   - Release security patch
   - Public disclosure coordination
   - CVE assignment (if applicable)

### Communication

- **Private disclosure**: We will work with you on responsible disclosure
- **Credit**: We will credit you in security advisories (unless you prefer anonymity)
- **Updates**: Regular status updates via email

## Security Features

### Interpreter Security

#### Memory Safety
- **Buffer overflow protection**: Comprehensive bounds checking
- **Input validation**: All user inputs validated and sanitized
- **Stack protection**: Stack canaries and ASLR support
- **Heap protection**: Use of secure allocators where possible

#### Parser Security
- **Input validation**: Robust validation of source code syntax
- **Resource limits**: Protection against excessive memory usage
- **Error handling**: Secure error message handling without information leakage

#### Built-in Function Security
- **Parameter validation**: All built-ins validate their parameters
- **Safe operations**: File and crypto operations use secure implementations
- **Error propagation**: Secure error handling without sensitive data exposure

### Development Security

- **Code review**: All code changes require security review
- **Security testing**: Automated security testing in CI/CD
- **Static analysis**: Static code analysis for security issues
- **Fuzzing**: Regular fuzzing of parser and VM components

### Compiler Security Flags

The Ember Core interpreter is built with security hardening:

```bash
# Security compilation flags
CFLAGS += -fstack-protector-strong
CFLAGS += -D_FORTIFY_SOURCE=2
CFLAGS += -fPIE
LDFLAGS += -pie
LDFLAGS += -Wl,-z,relro
LDFLAGS += -Wl,-z,now
```

## Vulnerability Categories

### High Severity
- **Memory corruption** (buffer overflows, use-after-free)
- **Code injection** through parser vulnerabilities
- **Privilege escalation** in built-in functions
- **Information disclosure** of sensitive data

### Medium Severity
- **Denial of service** through resource exhaustion
- **Logic errors** in security-sensitive functions
- **Timing attacks** in cryptographic functions
- **Input validation bypass**

### Low Severity
- **Information leakage** (non-sensitive)
- **Minor logic errors** without security impact
- **Configuration issues**

## Security Best Practices

### For Users

#### Safe Script Execution
```bash
# Run scripts with limited privileges
./build/ember --safe script.ember

# Use read-only directories when possible
chmod -w script_directory/
./build/ember script_directory/script.ember
```

#### Input Validation
```javascript
// Always validate user inputs
user_input = read_file("user_data.txt")
if (len(user_input) > 1000) {
    print("Error: Input too large")
    exit(1)
}
```

### For Developers

#### Secure Coding Practices
```c
// Always check bounds
if (index >= array_length) {
    return EMBER_ERROR_BOUNDS;
}

// Validate all inputs
if (!is_valid_string(input) || strlen(input) > MAX_STRING_LENGTH) {
    return EMBER_ERROR_INVALID_INPUT;
}

// Use secure memory functions
memset_s(password, sizeof(password), 0, sizeof(password));
```

#### Testing Security
```bash
# Run with AddressSanitizer
make ASAN=1 && ./build/ember test_script.ember

# Memory safety testing
valgrind --tool=memcheck ./build/ember script.ember

# Fuzzing
cd tests/fuzz && make fuzz
```

## Known Security Considerations

### Current Security Status

As of version 2.0.3, Ember Core includes:

- ✅ **Memory safety improvements** - Buffer overflow protection
- ✅ **Input validation** - Comprehensive input sanitization  
- ✅ **Secure built-ins** - Cryptographic functions with secure implementations
- ✅ **Compiler hardening** - Security-hardened compilation flags
- ✅ **Error handling** - Secure error messages without information leakage

### Security Limitations

Current limitations to be aware of:

1. **Global scope only**: All variables are global (local scope planned)
2. **No sandboxing**: Scripts run with interpreter privileges
3. **File system access**: Built-in file functions have full filesystem access
4. **No privilege separation**: No built-in capability restrictions

### Planned Security Enhancements

- **Function-local scoping** - Reduce variable pollution
- **Capability-based security** - Restrict built-in function access
- **Script sandboxing** - Isolated execution environments
- **Enhanced input validation** - More comprehensive validation

## Security Updates

### Update Notifications

- **GitHub Security Advisories**: Automated notifications for repository watchers
- **Release notes**: Security fixes highlighted in release documentation
- **Email notifications**: Direct notifications for critical security updates

### Applying Updates

```bash
# Check current version
./build/ember --version

# Update from source
git fetch origin
git checkout v2.0.3  # Latest security release
make clean && make && make install
```

## Security Resources

### Documentation
- [Core Security Guide](https://docs.emberplatform.org/security/core)
- [Secure Coding Practices](https://docs.emberplatform.org/guide/security)
- [Built-in Function Security](https://docs.emberplatform.org/api/security)

### Tools
- [Security Testing Scripts](../ember-tests/security_scripts/)
- [Fuzzing Framework](tests/fuzz/)
- [Memory Safety Tests](tests/unit/memory/)

### External Resources
- [OWASP Secure Coding Practices](https://owasp.org/www-project-secure-coding-practices-quick-reference-guide/)
- [CWE Common Weakness Enumeration](https://cwe.mitre.org/)
- [CVE Database](https://cve.mitre.org/)

## Contact Information

- **Security Team**: security@emberplatform.org
- **General Issues**: https://github.com/emberplatform/ember-core/issues
- **Security Advisory**: https://github.com/emberplatform/ember-core/security/advisories

## Acknowledgments

We would like to thank the following individuals for responsibly disclosing security vulnerabilities:

- [Security researchers will be listed here as vulnerabilities are discovered and fixed]

---

**Thank you for helping keep Ember Core secure!**