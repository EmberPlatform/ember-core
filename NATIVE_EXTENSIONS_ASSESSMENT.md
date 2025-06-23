# Native Extensions Assessment

## Current Situation

The Ember platform has two parallel approaches for standard library functionality:

1. **ember-native repository**: Comprehensive C-based native extension modules with advanced features
2. **ember-core simple implementations**: Basic C implementations directly in ember-core (`crypto_simple.c`, `json_simple.c`, `io_simple.c`)

## Analysis

### Pros of Current Approach
- **Working functionality**: Basic crypto, JSON, and I/O functions work today
- **Simple integration**: Functions are directly compiled into ember-core
- **No dependency issues**: No need to link external libraries
- **Clear upgrade path**: Can enhance functions incrementally

### Cons of Current Approach
- **Limited functionality**: Only basic features implemented
- **Performance**: Simple implementations lack optimization
- **Maintenance burden**: Two codebases for similar functionality
- **Confusion**: Unclear which approach is canonical

## Recommendations

### Short-term (Keep Current Approach)
1. **Continue with simple implementations** in ember-core for now
2. **Document clearly** in CLAUDE.md that these are temporary
3. **Focus on stability** over advanced features
4. **Fix critical bugs** (stack underflow, build warnings) first

### Medium-term (3-6 months)
1. **Write proper Ember stdlib modules** that use the native functions
2. **Create clear API boundaries** between native and Ember code
3. **Gradually enhance** the simple implementations with real crypto (OpenSSL)
4. **Test thoroughly** before replacing simple implementations

### Long-term (6+ months)
1. **Pure Ember stdlib** for high-level functionality
2. **Minimal native layer** only for:
   - System calls (file I/O, networking)
   - Cryptography (security-critical)
   - Performance bottlenecks (after profiling)
3. **Clear documentation** on when to use native vs Ember

## Proposed Architecture

```
┌─────────────────────────────────┐
│     Ember User Code             │
├─────────────────────────────────┤
│     Ember Standard Library      │  ← Pure Ember modules
│  (collections, algorithms, etc) │
├─────────────────────────────────┤
│     Ember Built-ins             │  ← print(), len(), type()
│    (in ember-core/builtins.c)  │
├─────────────────────────────────┤
│     Native Extensions           │  ← Only for system/perf
│  (crypto, file I/O, networking) │
└─────────────────────────────────┘
```

## Specific Recommendations for You

Given your limited usage with Claude Opus:

1. **Keep the simple implementations** - they work and are maintainable
2. **Document the transition plan** in CLAUDE.md for future developers
3. **Focus on core interpreter bugs** rather than stdlib perfection
4. **Use the native extensions** as reference implementations only

## Example Transition Plan

### Phase 1: Current State (Now)
- crypto_simple.c provides sha256(), sha512(), etc.
- Works but uses simple hash algorithm

### Phase 2: Enhancement (When Time Permits)
```c
// In crypto_simple.c
#ifdef HAVE_OPENSSL
    // Use real SHA-256 from OpenSSL
    SHA256_CTX ctx;
    SHA256_Init(&ctx);
    SHA256_Update(&ctx, input, len);
    SHA256_Final(output, &ctx);
#else
    // Fall back to simple implementation
    simple_sha256(input, output);
#endif
```

### Phase 3: Ember Stdlib (Future)
```ember
// In ember/crypto.ember
import _native_sha256 from "__builtin__";

function sha256(data) {
    // Add validation, error handling, etc.
    if (!is_string(data) && !is_bytes(data)) {
        throw TypeError("sha256 expects string or bytes");
    }
    return _native_sha256(data);
}

// Higher-level functions built on primitives
function hash_file(path) {
    data = read_file(path);
    return sha256(data);
}
```

## Conclusion

The current approach with simple native implementations in ember-core is pragmatic and working. Keep it for now, fix the critical bugs, and plan for a gradual transition to a proper Ember stdlib when the core interpreter is more stable.

The ember-native repository can serve as a reference for what features to implement, but don't feel obligated to integrate it wholesale. It represents an aspirational goal that can guide future development.