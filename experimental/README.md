# Experimental Features

This directory contains advanced features that were implemented but are not yet integrated into the main Ember interpreter. These features were moved here to clarify the distinction between working functionality and aspirational code.

## Directory Structure

### `jit/`
Contains the Just-In-Time compiler implementation:
- Full x86-64 code generation
- Arithmetic optimization with SIMD
- Hot path detection and compilation
- **Status**: Complete but not integrated due to header conflicts and architectural issues

### `vm_pools/`
Contains advanced VM pool implementations:
- `vm_pool_lockfree.c` - Lock-free concurrent VM pool
- `work_stealing_pool.c` - Work-stealing scheduler for parallel execution
- `concurrent_vm_pool.c` - Thread-safe VM pool with advanced features
- **Status**: Sophisticated implementations that were never connected to the main VM

### `memory/`
Contains advanced memory management:
- `arena_allocator.c/h` - SIMD-accelerated arena allocator
- `numa_arena_allocator.c` - NUMA-aware memory management
- **Status**: Feature-rich but VM still uses standard malloc/free

## Why These Features Are Experimental

1. **Integration Complexity**: These features require significant architectural changes to properly integrate
2. **Circular Dependencies**: JIT compiler has circular dependencies with VM headers
3. **Single-threaded Reality**: VM pools and NUMA optimization are designed for multi-threaded execution, but Ember is currently single-threaded
4. **Premature Optimization**: These optimizations were built before establishing baseline performance

## Integration Requirements

To properly integrate these features:

1. **JIT Compiler**:
   - Resolve header dependency conflicts
   - Design proper VM-JIT interface
   - Add initialization and lifecycle management
   - Start with simple hot-path compilation

2. **VM Pools**:
   - First make interpreter thread-safe
   - Design proper work distribution model
   - Add synchronization primitives
   - Benchmark to ensure actual performance gains

3. **Memory Management**:
   - Replace malloc/free calls throughout codebase
   - Add proper initialization in VM startup
   - Design object allocation strategy
   - Ensure compatibility with garbage collection (if implemented)

## Current Priority

The main focus should be on:
1. Stabilizing the core interpreter
2. Fixing known bugs (continue in for loops, build warnings)
3. Improving basic performance
4. Only then considering these advanced features

These implementations represent significant engineering effort and contain valuable ideas, but they should not be activated until the foundational architecture can properly support them.